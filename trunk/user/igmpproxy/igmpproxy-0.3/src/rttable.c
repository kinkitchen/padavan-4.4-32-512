/*
**  igmpproxy - IGMP proxy based multicast router
**  Copyright (C) 2005 Johnny Egeland <johnny@rlo.org>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**----------------------------------------------------------------------------
**
**  This software is derived work from the following software. The original
**  source code has been modified from it's original state by the author
**  of igmpproxy.
**
**  smcroute 0.92 - Copyright (C) 2001 Carsten Schill <carsten@cschill.de>
**  - Licensed under the GNU General Public License, either version 2 or
**    any later version.
**
**  mrouted 3.9-beta3 - Copyright (C) 2002 by The Board of Trustees of
**  Leland Stanford Junior University.
**  - Licensed under the 3-clause BSD license, see Stanford.txt file.
**
*/
/**
*   rttable.c
*
*   Updates the routingtable according to
*     received request.
*/

#include "igmpproxy.h"

#define MAX_ORIGINS 4

/**
*   Routing table structure definition. Double linked list...
*/
struct RouteTable {
    struct RouteTable   *nextroute;     // Pointer to the next group in line.
    struct RouteTable   *prevroute;     // Pointer to the previous group in line.
    uint32_t            group;          // The group to route
    uint32_t            originAddrs[MAX_ORIGINS]; // The origin adresses (only set on activated routes)
    uint32_t            vifBits;        // Bits representing recieving VIFs.

    // Keeps the upstream membership state...
    short               upstrState;     // Upstream membership state.
    int                 upstrVif;       // Upstream Vif Index.

    // These parameters contain aging details.
    uint32_t            ageVifBits;     // Bits representing aging VIFs.
    int                 ageValue;       // Downcounter for death.
    int                 ageActivity;    // Records any acitivity that notes there are still listeners.

    // Keeps downstream hosts information
    uint32_t            downstreamHostsHashSeed;
    uint8_t             downstreamHostsHashTable[];
};


// Keeper for the routing table...
static struct RouteTable   *routing_table;

// Prototypes
void logRouteTable(const char *header);
int internAgeRoute(struct RouteTable *croute);
int internUpdateKernelRoute(struct RouteTable *route, int activate);


/**
*   Functions for downstream hosts hash table
*/

// MurmurHash3 32bit hash function by Austin Appleby, public domain
static uint32_t murmurhash3(uint32_t x) {
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

static inline void setDownstreamHost(struct Config *conf, struct RouteTable *croute, uint32_t src) {
    uint32_t hash = murmurhash3(src ^ croute->downstreamHostsHashSeed) % (conf->downstreamHostsHashTableSize*8);
    BIT_SET(croute->downstreamHostsHashTable[hash/8], hash%8);
}

static inline void clearDownstreamHost(struct Config *conf, struct RouteTable *croute, uint32_t src) {
    uint32_t hash = murmurhash3(src ^ croute->downstreamHostsHashSeed) % (conf->downstreamHostsHashTableSize*8);
    BIT_CLR(croute->downstreamHostsHashTable[hash/8], hash%8);
}

static inline void zeroDownstreamHosts(struct Config *conf, struct RouteTable *croute) {
    croute->downstreamHostsHashSeed = ((uint32_t)rand() << 16) | (uint32_t)rand();
    memset(croute->downstreamHostsHashTable, 0, conf->downstreamHostsHashTableSize);
}

static inline int testNoDownstreamHost(struct Config *conf, struct RouteTable *croute) {
    for (size_t i = 0; i < conf->downstreamHostsHashTableSize; i++) {
        if (croute->downstreamHostsHashTable[i])
            return 0;
    }
    return 1;
}

/**
*   Initializes the routing table.
*/
void initRouteTable(void) {
    unsigned Ix;
    struct IfDesc *Dp;

    // Clear routing table...
    routing_table = NULL;

    // Join the all routers group on downstream vifs...
    for ( Ix = 0; (Dp = getIfByIx(Ix)); Ix++ ) {
        // If this is a downstream vif, we should join the All routers group...
        if( Dp->InAdr.s_addr && ! (Dp->Flags & IFF_LOOPBACK) && Dp->state == IF_STATE_DOWNSTREAM) {
            my_log(LOG_DEBUG, 0, "Joining all-routers group %s on vif %s",
                         inetFmt(allrouters_group,s1),inetFmt(Dp->InAdr.s_addr,s2));

            k_join(Dp, allrouters_group);

            my_log(LOG_DEBUG, 0, "Joining all igmpv3 multicast routers group %s on vif %s",
                         inetFmt(alligmp3_group,s1),inetFmt(Dp->InAdr.s_addr,s2));
            k_join(Dp, alligmp3_group);
        }
    }
}

/**
*   Internal function to send join or leave requests for
*   a specified route upstream...
*/
static void sendJoinLeaveUpstream(struct RouteTable* route, int join) {
    struct IfDesc*      upstrIf;
    int i;

    for(i=0; i<MAX_UPS_VIFS; i++)
    {
        if (-1 != upStreamIfIdx[i])
        {
            // Get the upstream IF...
            upstrIf = getIfByIx( upStreamIfIdx[i] );
            if(upstrIf == NULL) {
                my_log(LOG_ERR, 0 ,"FATAL: Unable to get Upstream IF.");
            }

            // Check if there is a white list for the upstram VIF
            if (upstrIf->allowedgroups != NULL) {
              uint32_t           group = route->group;
                struct SubnetList* sn;

                // Check if this Request is legit to be forwarded to upstream
                for(sn = upstrIf->allowedgroups; sn != NULL; sn = sn->next)
                    if((group & sn->subnet_mask) == sn->subnet_addr)
                        // Forward is OK...
                        break;

                if (sn == NULL) {
                    my_log(LOG_INFO, 0, "The group address %s may not be forwarded upstream. Ignoring.", inetFmt(group, s1));
                    return;
                }
            }

            // Send join or leave request...
            if(join) {
                // Only join a group if there are listeners downstream...
                if(route->vifBits > 0) {
                    my_log(LOG_DEBUG, 0, "Joining group %s upstream on IF address %s",
                                 inetFmt(route->group, s1),
                                 inetFmt(upstrIf->InAdr.s_addr, s2));

                    k_join(upstrIf, route->group);

                    route->upstrState = ROUTESTATE_JOINED;
                } else {
                    my_log(LOG_DEBUG, 0, "No downstream listeners for group %s. No join sent.",
                        inetFmt(route->group, s1));
                }
            } else {
                // Only leave if group is not left already...
                if(route->upstrState != ROUTESTATE_NOTJOINED) {
                    my_log(LOG_DEBUG, 0, "Leaving group %s upstream on IF address %s",
                                 inetFmt(route->group, s1),
                                 inetFmt(upstrIf->InAdr.s_addr, s2));

                    k_leave(upstrIf, route->group);

                    route->upstrState = ROUTESTATE_NOTJOINED;
                }
            }
        }
        else
        {
            i = MAX_UPS_VIFS;
        }
    }
}

/**
*   Clear all routes from routing table, and alerts Leaves upstream.
*/
void clearAllRoutes(void) {
    struct RouteTable   *croute, *remainroute;

    // Loop through all routes...
    for(croute = routing_table; croute; croute = remainroute) {

        remainroute = croute->nextroute;

        // Log the cleanup in debugmode...
        my_log(LOG_DEBUG, 0, "Removing route entry for %s",
                     inetFmt(croute->group, s1));

        // Uninstall current route
        if(!internUpdateKernelRoute(croute, 0)) {
            my_log(LOG_WARNING, 0, "The removal from Kernel failed.");
        }

        // Send Leave message upstream.
        sendJoinLeaveUpstream(croute, 0);

        // Clear memory, and set pointer to next route...
        free(croute);
    }
    routing_table = NULL;

    // Send a notice that the routing table is empty...
    my_log(LOG_NOTICE, 0, "All routes removed. Routing table is empty.");
}

/**
*   Private access function to find a route from a given
*   Route Descriptor.
*/
static struct RouteTable *findRoute(uint32_t group) {
    struct RouteTable*  croute;

    for(croute = routing_table; croute; croute = croute->nextroute) {
        if(croute->group == group) {
            return croute;
        }
    }

    return NULL;
}

/**
*   Adds a specified route to the routingtable.
*   If the route already exists, the existing route
*   is updated...
*/
int insertRoute(uint32_t group, int ifx, uint32_t src) {

    struct Config *conf = getCommonConfig();
    struct RouteTable*  croute;

    // Sanitycheck the group adress...
    if( ! IN_MULTICAST( ntohl(group) )) {
       if(group==htonl(INADDR_BROADCAST)) { 
       return 0;
    }
       my_log(LOG_WARNING, 0, "The group address %s is not a valid Multicast group. Table insert failed.",
           inetFmt(group, s1));
        return 0;
    }
    
    // Santiycheck the VIF index...
    if(ifx >= MAXVIFS) {
        my_log(LOG_WARNING, 0, "The VIF Ix %d is out of range (0-%d). Table insert failed.", ifx, MAXVIFS-1);
        return 0;
    }

    // Try to find an existing route for this group...
    croute = findRoute(group);
    if(croute==NULL) {
        struct RouteTable*  newroute;

        my_log(LOG_DEBUG, 0, "No existing route for %s. Create new.",
                     inetFmt(group, s1));


        // Create and initialize the new route table entry..
        newroute = (struct RouteTable*)malloc(sizeof(struct RouteTable) + (conf->fastUpstreamLeave ? conf->downstreamHostsHashTableSize : 0));
        // Insert the route desc and clear all pointers...
        newroute->group      = group;
        memset(newroute->originAddrs, 0, MAX_ORIGINS * sizeof(newroute->originAddrs[0]));
        newroute->nextroute  = NULL;
        newroute->prevroute  = NULL;
        newroute->upstrVif   = -1;

        if(conf->fastUpstreamLeave) {
            // Init downstream hosts bit hash table
            zeroDownstreamHosts(conf, newroute);

            // Add downstream host
            setDownstreamHost(conf, newroute, src);
        }

        // The group is not joined initially.
        newroute->upstrState = ROUTESTATE_NOTJOINED;

        // The route is not active yet, so the age is unimportant.
        newroute->ageValue    = conf->robustnessValue;
        newroute->ageActivity = 0;

        BIT_ZERO(newroute->ageVifBits);     // Initially we assume no listeners.

        // Set the listener flag...
        BIT_ZERO(newroute->vifBits);    // Initially no listeners...
        if(ifx >= 0) {
            BIT_SET(newroute->vifBits, ifx);
        }

        // Check if there is a table already....
        if(routing_table == NULL) {
            // No location set, so insert in on the table top.
            routing_table = newroute;
            my_log(LOG_DEBUG, 0, "No routes in table. Insert at beginning.");
        } else {

            my_log(LOG_DEBUG, 0, "Found existing routes. Find insert location.");

            // Check if the route could be inserted at the beginning...
            if(routing_table->group > group) {
                my_log(LOG_DEBUG, 0, "Inserting at beginning, before route %s",inetFmt(routing_table->group,s1));

                // Insert at beginning...
                newroute->nextroute = routing_table;
                newroute->prevroute = NULL;
                routing_table = newroute;

                // If the route has a next node, the previous pointer must be updated.
                if(newroute->nextroute != NULL) {
                    newroute->nextroute->prevroute = newroute;
                }

            } else {

                // Find the location which is closest to the route.
                for( croute = routing_table; croute->nextroute != NULL; croute = croute->nextroute ) {
                    // Find insert position.
                    if(croute->nextroute->group > group) {
                        break;
                    }
                }

                my_log(LOG_DEBUG, 0, "Inserting after route %s",inetFmt(croute->group,s1));

                // Insert after current...
                newroute->nextroute = croute->nextroute;
                newroute->prevroute = croute;
                if(croute->nextroute != NULL) {
                    croute->nextroute->prevroute = newroute;
                }
                croute->nextroute = newroute;
            }
        }

        // Set the new route as the current...
        croute = newroute;

        // Log the cleanup in debugmode...
        my_log(LOG_INFO, 0, "Inserted route table entry for %s on VIF #%d",
            inetFmt(croute->group, s1),ifx);

    } else if(ifx >= 0) {

        // The route exists already, so just update it.
        BIT_SET(croute->vifBits, ifx);

        // Register the VIF activity for the aging routine
        BIT_SET(croute->ageVifBits, ifx);

        // Register dwnstrHosts for host tracking if fastleave is enabled
        if(conf->fastUpstreamLeave) {
            setDownstreamHost(conf, croute, src);
        }

        // Log the cleanup in debugmode...
        my_log(LOG_INFO, 0, "Updated route entry for %s on VIF #%d",
            inetFmt(croute->group, s1), ifx);

        // Update route in kernel...
        if(!internUpdateKernelRoute(croute, 1)) {
            my_log(LOG_WARNING, 0, "The insertion into Kernel failed.");
            return 0;
        }
    }

    // Send join message upstream, if the route has no joined flag...
    if(croute->upstrState != ROUTESTATE_JOINED) {
        // Send Join request upstream
        sendJoinLeaveUpstream(croute, 1);
    }

    logRouteTable("Insert Route");

    return 1;
}

/**
*   Activates a passive group. If the group is already
*   activated, it's reinstalled in the kernel. If
*   the route is activated, no originAddr is needed.
*/
int activateRoute(uint32_t group, uint32_t originAddr, int upstrVif) {
    struct RouteTable*  croute;
    int result = 0;

    // Find the requested route.
    croute = findRoute(group);
    if(croute == NULL) {
        my_log(LOG_DEBUG, 0,
            "No table entry for %s [From: %s]. Inserting route.",
            inetFmt(group, s1),inetFmt(originAddr, s2));

        // Insert route, but no interfaces have yet requested it downstream.
        insertRoute(group, -1, 0);

        // Retrieve the route from table...
        croute = findRoute(group);
    }

    if(croute != NULL) {
        // If the origin address is set, update the route data.
        if(originAddr > 0) {
            // find this origin, or an unused slot
            int i;
            for (i = 0; i < MAX_ORIGINS; i++) {
                // unused slots are at the bottom, so we can't miss this origin
                if (croute->originAddrs[i] == originAddr || croute->originAddrs[i] == 0) {
                    break;
                }
            }

            if (i == MAX_ORIGINS) {
                i = MAX_ORIGINS - 1;

                my_log(LOG_WARNING, 0, "Too many origins for route %s; replacing %s with %s",
                    inetFmt(croute->group, s1),
                    inetFmt(croute->originAddrs[i], s2),
                    inetFmt(originAddr, s3));
            }

            // set origin
            croute->originAddrs[i] = originAddr;

            // move it to the top
            while (i > 0) {
                uint32_t t = croute->originAddrs[i - 1];
                croute->originAddrs[i - 1] = croute->originAddrs[i];
                croute->originAddrs[i] = t;
                i--;
            }
        }
        croute->upstrVif = upstrVif;

        // Only update kernel table if there are listeners !
        if(croute->vifBits > 0) {
            result = internUpdateKernelRoute(croute, 1);
        }
    }
    logRouteTable("Activate Route");

    return result;
}


/**
*   This function loops through all routes, and updates the age
*   of any active routes.
*/
void ageActiveRoutes(void) {
    struct RouteTable   *croute, *nroute;

    my_log(LOG_DEBUG, 0, "Aging routes in table.");

    // Scan all routes...
    for( croute = routing_table; croute != NULL; croute = nroute ) {

        // Keep the next route (since current route may be removed)...
        nroute = croute->nextroute;

        // Run the aging round algorithm.
        if(croute->upstrState != ROUTESTATE_CHECK_LAST_MEMBER) {
            // Only age routes if Last member probe is not active...
            internAgeRoute(croute);
        }
    }
    logRouteTable("Age active routes");
}

/**
*   Counts the number of interfaces a given route is active on
*/
int numberOfInterfaces(struct RouteTable *croute) {
    int Ix;
    struct IfDesc *Dp;
    int result = 0;
    // Loop through all interfaces
    for ( Ix = 0; (Dp = getIfByIx(Ix)); Ix++ ) {
        // If the interface is used by the route, increase counter
        if(BIT_TST(croute->vifBits, Dp->index)) {
            result++;
        }
    }
    my_log(LOG_DEBUG, 0, "counted %d interfaces", result);
    return result;
}

/**
*   Should be called when a leave message is received, to
*   mark a route for the last member probe state.
*/
void setRouteLastMemberMode(uint32_t group, uint32_t src) {
    struct Config       *conf = getCommonConfig();
    struct RouteTable   *croute;
    int                 routeStateCheck = 1;

    croute = findRoute(group);
    if(!croute)
        return;

    // Check for fast leave mode...
    if(conf->fastUpstreamLeave) {
        if(croute->upstrState == ROUTESTATE_JOINED) {
            // Remove downstream host from route
            clearDownstreamHost(conf, croute, src);
        }

        // Do route state check if there is no downstream host in hash table
        // This host does not have to been the last downstream host if hash collision occurred
        routeStateCheck = testNoDownstreamHost(conf, croute);

        if(croute->upstrState == ROUTESTATE_JOINED) {
            // Send a leave message right away but only when the route is not active anymore on any downstream host
            // It is possible that there are still some interfaces active but no downstream host in hash table due to hash collision
            if (routeStateCheck && numberOfInterfaces(croute) <= 1) {
                my_log(LOG_DEBUG, 0, "quickleave is enabled and this was the last downstream host, leaving group %s now", inetFmt(croute->group, s1));
                sendJoinLeaveUpstream(croute, 0);
            } else {
                my_log(LOG_DEBUG, 0, "quickleave is enabled but there are still some downstream hosts left, not leaving group %s", inetFmt(croute->group, s1));
            }
        }
    }

    // Set the routingstate to last member check if we have no known downstream host left or if fast leave mode is disabled...
    if(routeStateCheck) {
        croute->upstrState = ROUTESTATE_CHECK_LAST_MEMBER;

        // Set the count value for expiring... (-1 since first aging)
        croute->ageValue = conf->lastMemberQueryCount;
    }
}


/**
*   Ages groups in the last member check state. If the
*   route is not found, or not in this state, 0 is returned.
*/
int lastMemberGroupAge(uint32_t group) {
    struct RouteTable   *croute;

    croute = findRoute(group);
    if(croute!=NULL) {
        if(croute->upstrState == ROUTESTATE_CHECK_LAST_MEMBER) {
            return !internAgeRoute(croute);
        } else {
            return 0;
        }
    }
    return 0;
}

/**
*   Remove a specified route. Returns 1 on success,
*   and 0 if route was not found.
*/
static int removeRoute(struct RouteTable*  croute) {
    struct Config       *conf = getCommonConfig();
    int result = 1;

    // If croute is null, no routes was found.
    if(croute==NULL) {
        return 0;
    }

    // Log the cleanup in debugmode...
    my_log(LOG_DEBUG, 0, "Removed route entry for %s from table.",
                 inetFmt(croute->group, s1));

    //BIT_ZERO(croute->vifBits);

    // Uninstall current route from kernel
    if(!internUpdateKernelRoute(croute, 0)) {
        my_log(LOG_WARNING, 0, "The removal from Kernel failed.");
        result = 0;
    }

    // Send Leave request upstream if group is joined
    if(croute->upstrState == ROUTESTATE_JOINED || 
       (croute->upstrState == ROUTESTATE_CHECK_LAST_MEMBER && !conf->fastUpstreamLeave)) 
    {
        sendJoinLeaveUpstream(croute, 0);
    }

    // Update pointers...
    if(croute->prevroute == NULL) {
        // Topmost node...
        if(croute->nextroute != NULL) {
            croute->nextroute->prevroute = NULL;
        }
        routing_table = croute->nextroute;

    } else {
        croute->prevroute->nextroute = croute->nextroute;
        if(croute->nextroute != NULL) {
            croute->nextroute->prevroute = croute->prevroute;
        }
    }
    // Free the memory, and set the route to NULL...
    free(croute);
    croute = NULL;

    logRouteTable("Remove route");

    return result;
}


/**
*   Ages a specific route
*/
int internAgeRoute(struct RouteTable*  croute) {
    struct Config *conf = getCommonConfig();
    int result = 0;

    // Drop age by 1.
    croute->ageValue--;

    // Check if there has been any activity...
    if( croute->ageVifBits > 0 && croute->ageActivity == 0 ) {
        // There was some activity, check if all registered vifs responded.
        if(croute->vifBits == croute->ageVifBits) {
            // Everything is in perfect order, so we just update the route age.
            croute->ageValue = conf->robustnessValue;
            //croute->ageActivity = 0;
        } else {
            // One or more VIF has not gotten any response.
            croute->ageActivity++;

            // Update the actual bits for the route...
            croute->vifBits = croute->ageVifBits;
        }
    }
    // Check if there have been activity in aging process...
    else if( croute->ageActivity > 0 ) {

        // If the bits are different in this round, we must
        if(croute->vifBits != croute->ageVifBits) {
            // Or the bits together to insure we don't lose any listeners.
            croute->vifBits |= croute->ageVifBits;

            // Register changes in this round as well..
            croute->ageActivity++;
        }
    }

    // If the aging counter has reached zero, its time for updating...
    if(croute->ageValue == 0) {
        // Check for activity in the aging process,
        if(croute->ageActivity>0) {

            my_log(LOG_DEBUG, 0, "Updating route after aging : %s",
                         inetFmt(croute->group,s1));

            // Just update the routing settings in kernel...
            internUpdateKernelRoute(croute, 1);

            // We append the activity counter to the age, and continue...
            croute->ageValue = croute->ageActivity;
            croute->ageActivity = 0;
        } else {

            my_log(LOG_DEBUG, 0, "Removing group %s. Died of old age.",
                         inetFmt(croute->group,s1));

            // No activity was registered within the timelimit, so remove the route.
            removeRoute(croute);
        }
        // Tell that the route was updated...
        result = 1;
    }

    // The aging vif bits must be reset for each round...
    BIT_ZERO(croute->ageVifBits);

    return result;
}

/**
*   Updates the Kernel routing table. If activate is 1, the route
*   is (re-)activated. If activate is false, the route is removed.
*/
int internUpdateKernelRoute(struct RouteTable *route, int activate) {
    struct   MRouteDesc mrDesc;
    struct   IfDesc     *Dp;
    unsigned            Ix;
    int i;

    for (i = 0; i < MAX_ORIGINS; i++) {
        if (route->originAddrs[i] == 0 || route->upstrVif == -1) {
            continue;
        }

        // Build route descriptor from table entry...
        // Set the source address and group address...
        mrDesc.McAdr.s_addr     = route->group;
        mrDesc.OriginAdr.s_addr = route->originAddrs[i];

        // clear output interfaces
        memset( mrDesc.TtlVc, 0, sizeof( mrDesc.TtlVc ) );

        my_log(LOG_DEBUG, 0, "Vif bits : 0x%08x", route->vifBits);

        mrDesc.InVif = route->upstrVif;

        // Set the TTL's for the route descriptor...
        for ( Ix = 0; (Dp = getIfByIx(Ix)); Ix++ ) {
            if(Dp->state == IF_STATE_UPSTREAM) {
                continue;
            }
            else if(BIT_TST(route->vifBits, Dp->index)) {
                my_log(LOG_DEBUG, 0, "Setting TTL for Vif %d to %d", Dp->index, Dp->threshold);
                mrDesc.TtlVc[ Dp->index ] = Dp->threshold;
            }
        }

        // Do the actual Kernel route update...
        if(activate) {
            // Add route in kernel...
            addMRoute( &mrDesc );
        } else {
            // Delete the route from Kernel...
            delMRoute( &mrDesc );
        }
    }

    return 1;
}

/**
*   Debug function that writes the routing table entries
*   to the log.
*/
void logRouteTable(const char *header) {
        struct Config       *conf = getCommonConfig();
        struct RouteTable   *croute = routing_table;
        unsigned            rcount = 0;

        my_log(LOG_DEBUG, 0, "");
        my_log(LOG_DEBUG, 0, "Current routing table (%s):", header);
        my_log(LOG_DEBUG, 0, "-----------------------------------------------------");
        if(croute==NULL) {
            my_log(LOG_DEBUG, 0, "No routes in table...");
        } else {
            do {
                char st = 'I';
                char src[MAX_ORIGINS * 30 + 1];
                src[0] = '\0';
                int i;

                for (i = 0; i < MAX_ORIGINS; i++) {
                    if (croute->originAddrs[i] == 0) {
                        continue;
                    }
                    st = 'A';
                    sprintf(src + strlen(src), "Src%d: %s, ", i, inetFmt(croute->originAddrs[i], s1));
                }

                my_log(LOG_DEBUG, 0, "#%d: %sDst: %s, Age:%d, St: %c, OutVifs: 0x%08x, dHosts: %s",
                    rcount, src, inetFmt(croute->group, s2),
                    croute->ageValue, st,
                    croute->vifBits,
                    !conf->fastUpstreamLeave ? "not tracked" : testNoDownstreamHost(conf, croute) ? "no" : "yes");

                croute = croute->nextroute;

                rcount++;
            } while ( croute != NULL );
        }

        my_log(LOG_DEBUG, 0, "-----------------------------------------------------");
}

/**
*   Returns true when the given group belongs to the given interface
*/
int interfaceInRoute(int32_t group, int Ix) {
    struct RouteTable*  croute;
    croute = findRoute(group);
    if (croute != NULL) {
        my_log(LOG_DEBUG, 0, "Interface id %d is in group $d", Ix, group);
        return BIT_TST(croute->vifBits, Ix);
    } else {
        return 0;
    }
}
