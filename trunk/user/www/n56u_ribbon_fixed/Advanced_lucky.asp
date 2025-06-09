<!DOCTYPE html>
<html>
<head>
<title><#Web_Title#> - Lucky</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="Expires" content="-1">

<link rel="shortcut icon" href="images/favicon.ico">
<link rel="icon" href="images/favicon.png">
<link rel="stylesheet" type="text/css" href="/bootstrap/css/bootstrap.min.css">
<link rel="stylesheet" type="text/css" href="/bootstrap/css/main.css">
<link rel="stylesheet" type="text/css" href="/bootstrap/css/engage.itoggle.css">

<script type="text/javascript" src="/jquery.js"></script>
<script type="text/javascript" src="/bootstrap/js/bootstrap.min.js"></script>
<script type="text/javascript" src="/bootstrap/js/engage.itoggle.min.js"></script>
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/itoggle.js"></script>
<script type="text/javascript" src="/client_function.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script>
var $j = jQuery.noConflict();
<% lucky_status(); %>
<% login_state_hook(); %>
$j(document).ready(function() {

	init_itoggle('lucky_enable');
	$j("#tab_lucky_cfg, #tab_lucky_log").click(
	function () {
		var newHash = $j(this).attr('href').toLowerCase();
		showTab(newHash);
		return false;
	});

});

</script>
<script>

function initial(){

	show_banner(2);
	show_menu(5, 23, 0);
	show_footer();
	fill_status(lucky_status());
	
	if (!login_safe()){
		//textarea_scripts_enabled(0);
		$j('#btn_exec1').attr('disabled', 'disabled');
		$j('#btn_exec2').attr('disabled', 'disabled');
		$j('#btn_exec3').attr('disabled', 'disabled');
		$j('#btn_exec4').attr('disabled', 'disabled');
		$j('#btn_exec5').attr('disabled', 'disabled');
		$j('#btn_exec6').attr('disabled', 'disabled');
		$j('#LuckyCmd').attr('disabled', 'disabled');
	}else
		document.form.LuckyCmd.focus();

}

function fill_status(status_code){
	var stext = "Unknown";
	if (status_code == 0)
		stext = "<#Stopped#>";
	else if (status_code == 1)
		stext = "<#Running#>";
	$("lucky_status").innerHTML = '<span class="label label-' + (status_code != 0 ? 'success' : 'warning') + '">' + stext + '</span>';
}

var arrHashes = ["cfg","log"];
function showTab(curHash) {
	var obj = $('tab_lucky_' + curHash.slice(1));
	if (obj == null || obj.style.display == 'none')
	curHash = '#cfg';
	for (var i = 0; i < arrHashes.length; i++) {
		if (curHash == ('#' + arrHashes[i])) {
			$j('#tab_lucky_' + arrHashes[i]).parents('li').addClass('active');
			$j('#wnd_lucky_' + arrHashes[i]).show();
		} else {
			$j('#wnd_lucky_' + arrHashes[i]).hide();
			$j('#tab_lucky_' + arrHashes[i]).parents('li').removeClass('active');
			}
		}
	window.location.hash = curHash;
}

function applyRule(){
	showLoading();
	
	document.form.action_mode.value = " Apply ";
	document.form.current_page.value = "/Advanced_lucky.asp";
	document.form.next_page.value = "";
	
	document.form.submit();
}

function done_validating(action){
	refreshpage();
}

//function textarea_scripts_enabled(v){
    	//inputCtrl(document.form['scripts.lucky.conf'], v);
//}

function clearLog(){
	var $j = jQuery.noConflict();
	$j.post('/apply.cgi', {
		'action_mode': ' ClearluckyLog ',
		'next_host': 'Advanced_lucky.asp#log'
	}).always(function() {
		setTimeout(function() {
			location.reload(); 
		}, 3000);
	});
}

function button_restartlucky(){
    	var $j = jQuery.noConflict();
    	$j.post('/apply.cgi',
    	{
        		'action_mode': ' Restartlucky ',
    	});
}
function clearOut(){
	$j('#console_area').html('');
	$j('#LuckyCmd').val('');
}


function userPost(){
	if (!login_safe())
		return false;
	$j('#btn_exec1').attr('disabled', 'disabled');
	$j.ajax({
        type: "post",
        url: "/apply.cgi",
        data: {
		'action_mode': ' LuckyResetuser ',
		'current_page': 'Advanced_lucky.asp',
		'next_page': 'Advanced_lucky.asp',
		'LuckyCmd': $j('#LuckyCmd').val()
	},
        dataType: "json", 
        success: function(response) {
            if (response && typeof response === 'object') {
                const sys_result = response.sys_result; 
                const message = response.message;     
                
                
                alert(message);

                
                if (sys_result === 1) {
                    console.log("操作成功");
                } else {
                    console.error("操作失败");
                }
            } 
        },
        error: function() {
            alert("请求失败，请稍后再试！");
        },
        complete: function() {
            setTimeout(function() {
                $j('#btn_exec1').removeAttr('disabled');
            }, 3000);
        }
    });
}


function passPost(){
	if (!login_safe())
		return false;
	$j('#btn_exec2').attr('disabled', 'disabled');
	$j.ajax({
        type: "post",
        url: "/apply.cgi",
        data: {
		'action_mode': ' LuckyResetpass ',
		'current_page': 'Advanced_lucky.asp',
		'next_page': 'Advanced_lucky.asp',
		'LuckyCmd': $j('#LuckyCmd').val()
	},
        dataType: "json", 
        success: function(response) {
            if (response && typeof response === 'object') {
                const sys_result = response.sys_result; 
                const message = response.message;     
                
                
                alert(message);

                
                if (sys_result === 1) {
                    console.log("操作成功");
                } else {
                    console.error("操作失败");
                }
            } 
        },
        error: function() {
            alert("请求失败，请稍后再试！");
        },
        complete: function() {
            setTimeout(function() {
                $j('#btn_exec2').removeAttr('disabled');
            }, 3000);
        }
    });
}

function portPost(){
	if (!login_safe())
		return false;
	$j('#btn_exec3').attr('disabled', 'disabled');
	$j.ajax({
        type: "post",
        url: "/apply.cgi",
        data: {
		'action_mode': ' LuckyResetport ',
		'current_page': 'Advanced_lucky.asp',
		'next_page': 'Advanced_lucky.asp',
		'LuckyCmd': $j('#LuckyCmd').val()
	},
        dataType: "json", 
        success: function(response) {
            if (response && typeof response === 'object') {
                const sys_result = response.sys_result; 
                const message = response.message;     
                
                
                alert(message);

                
                if (sys_result === 1) {
                    console.log("操作成功");
                } else {
                    console.error("操作失败");
                }
            } 
        },
        error: function() {
            alert("请求失败，请稍后再试！");
        },
        complete: function() {
            setTimeout(function() {
                $j('#btn_exec3').removeAttr('disabled');
            }, 3000);
        }
    });
}

function safePost(){
	if (!login_safe())
		return false;
	$j('#btn_exec4').attr('disabled', 'disabled');
	$j.ajax({
        type: "post",
        url: "/apply.cgi",
        data: {
		'action_mode': ' LuckyResetsafe ',
		'current_page': 'Advanced_lucky.asp',
		'next_page': 'Advanced_lucky.asp',
		'LuckyCmd': $j('#LuckyCmd').val()
	},
        dataType: "json", 
        success: function(response) {
            if (response && typeof response === 'object') {
                const sys_result = response.sys_result; 
                const message = response.message;     
                
                
                alert(message);

                
                if (sys_result === 1) {
                    console.log("操作成功");
                } else {
                    console.error("操作失败");
                }
            } 
        },
        error: function() {
            alert("请求失败，请稍后再试！");
        },
        complete: function() {
            setTimeout(function() {
                $j('#btn_exec4').removeAttr('disabled');
            }, 3000);
        }
    });
}

function Internettrue(){
	if (!login_safe())
		return false;
	$j('#btn_exec5').attr('disabled', 'disabled');
	$j.ajax({
        type: "post",
        url: "/apply.cgi",
        data: {
		'action_mode': ' Luckynettrue ',
		'current_page': 'Advanced_lucky.asp',
		'next_page': 'Advanced_lucky.asp',
	},
        dataType: "json", 
        success: function(response) {
            if (response && typeof response === 'object') {
                const sys_result = response.sys_result; 
                const message = response.message;     
                
                
                alert(message);

                
                if (sys_result === 1) {
                    console.log("操作成功");
                } else {
                    console.error("操作失败");
                }
            } 
        },
        error: function() {
            alert("请求失败，请稍后再试！");
        },
        complete: function() {
            setTimeout(function() {
                $j('#btn_exec5').removeAttr('disabled');
            }, 3000);
        }
    });
}

function Internetfalse(){
	if (!login_safe())
		return false;
	$j('#btn_exec6').attr('disabled', 'disabled');
	$j.ajax({
        type: "post",
        url: "/apply.cgi",
        data: {
		'action_mode': ' Luckynetfalse ',
		'current_page': 'Advanced_lucky.asp',
		'next_page': 'Advanced_lucky.asp',
	},
        dataType: "json", 
        success: function(response) {
            if (response && typeof response === 'object') {
                const sys_result = response.sys_result; 
                const message = response.message;     
                
                
                alert(message);

                
                if (sys_result === 1) {
                    console.log("操作成功");
                } else {
                    console.error("操作失败");
                }
            } 
        },
        error: function() {
            alert("请求失败，请稍后再试！");
        },
        complete: function() {
            setTimeout(function() {
                $j('#btn_exec6').removeAttr('disabled');
            }, 3000);
        }
    });
}

</script>
</head>

<body onload="initial();" onunLoad="return unload_body();">

<div class="wrapper">
	<div class="container-fluid" style="padding-right: 0px">
	<div class="row-fluid">
	<div class="span3"><center><div id="logo"></div></center></div>
	<div class="span9" >
	<div id="TopBanner"></div>
	</div>
	</div>
	</div>

	<div id="Loading" class="popup_bg"></div>

	<iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>

	<form method="post" name="form" id="ruleForm" action="/start_apply.htm" target="hidden_frame">

	<input type="hidden" name="current_page" value="Advanced_lucky.asp">
	<input type="hidden" name="next_page" value="">
	<input type="hidden" name="next_host" value="">
	<input type="hidden" name="sid_list" value="LUCKY;LANHostConfig;General;">
	<input type="hidden" name="group_id" value="">
	<input type="hidden" name="action_mode" value="">
	<input type="hidden" name="action_script" value="">

	<div class="container-fluid">
	<div class="row-fluid">
	<div class="span3">
	<!--Sidebar content-->
	<!--=====Beginning of Main Menu=====-->
	<div class="well sidebar-nav side_nav" style="padding: 0px;">
	<ul id="mainMenu" class="clearfix"></ul>
	<ul class="clearfix">
	<li>
	<div id="subMenu" class="accordion"></div>
	</li>
	</ul>
	</div>
	</div>
	<div class="span9">
	<!--Body content-->
	<div class="row-fluid">
	<div class="span12">
	<div class="box well grad_colour_dark_blue">
	<h2 class="box_head round_top">Lucky</h2>
	<div class="round_bottom">
	<div>
	<ul class="nav nav-tabs" style="margin-bottom: 10px;">
	<li class="active"><a id="tab_lucky_cfg" href="#cfg">基本设置</a></li>
	<li><a id="tab_lucky_log" href="#log">运行日志</a></li>
	</ul>
	</div>
	<div class="row-fluid">
	<div id="tabMenu" class="submenuBlock"></div>
	<div id="wnd_lucky_cfg">
	<div class="alert alert-info" style="margin: 10px;">
	lucky是一款软硬路由公网神器,集成了多种工具，ipv6/ipv4 端口转发,反向代理,DDNS,WOL,ipv4 stun内网穿透,cron,acme,阿里云盘,ftp,webdav,filebrowser<br>官网：<a href="https://lucky666.cn" target="blank">https://lucky666.cn</a><br><br>由于Padavan固件防火墙容易初始化，若lucky里设置自动放行防火墙失效建议手动放行。
	<br><div>当前版本:【<span style="color: #FFFF00;"><% nvram_get_x("", "lucky_ver"); %></span>】&nbsp;&nbsp;最新版本:【<span style="color: #FD0187;"><% nvram_get_x("", "lucky_ver_n"); %></span>】 </div>
	</div>
	<table width="100%" cellpadding="4" cellspacing="0" class="table">
	<tr>
	<th><#running_status#>
	</th>
	<td colspan="4" id="lucky_status">
	</tr><td colspan="4"></td>
	<tr>
	<th width="30%" style="border-top: 0 none;">启用lucky</th>
	<td style="border-top: 0 none;">
	<div class="main_itoggle">
	<div id="lucky_enable_on_of">
	<input type="checkbox" id="lucky_enable_fake" <% nvram_match_x("", "lucky_enable", "1", "value=1 checked"); %><% nvram_match_x("", "lucky_enable", "0", "value=0"); %>  />
	</div>
	</div>
	<div style="position: absolute; margin-left: -10000px;">
	<input type="radio" value="1" name="lucky_enable" id="lucky_enable_1" class="input" value="1" <% nvram_match_x("", "lucky_enable", "1", "checked"); %> /><#checkbox_Yes#>
	<input type="radio" value="0" name="lucky_enable" id="lucky_enable_0" class="input" value="0" <% nvram_match_x("", "lucky_enable", "0", "checked"); %> /><#checkbox_No#>
	</div>
	</td>
	<td colspan="4" style="border-top: 0 none;">
	<input class="btn btn-success" style="width:150px" type="button" name="restartlucky" value="重启" onclick="button_restartlucky()" />
	</td>
	</tr><td colspan="4"></td>
	<tr>
	<th width="30%" style="border-top: 0 none;">配置文件目录</th>
	<td style="border-top: 0 none;" colspan="3">
	<div class="input-append">
	<input name="lucky_cmd" type="text" class="input" id="lucky_cmd" placeholder="/etc/storage/lucky" onkeypress="return is_string(this,event);" value="<% nvram_get_x("","lucky_cmd"); %>" size="32" maxlength="128" />
	</div><br><span style="color:#888;">⚠️来自官方的友好提醒：<br>经常在软件后台备份配置文件，防止升级或意外丢失配置！</span>
	</td>
	</tr><td colspan="4"></td>
	<tr>
	<th width="30%" style="border-top: 0 none;">程序路径</th>
	<td style="border-top: 0 none;" colspan="3">
	<div class="input-append">
	<input name="lucky_bin" type="text" class="input" id="lucky_bin" placeholder="/tmp/lucky" onkeypress="return is_string(this,event);" value="<% nvram_get_x("","lucky_bin"); %>" size="32" maxlength="128" />
	</div><br><span style="color:#888;">自定义程序的存放路径，填写完整的路径和程序名称</span>
	</td>
	</tr><td colspan="4"></td>
	<tr>
	<th width="30%" style="border-top: 0 none;">程序选择</th>
	<td style="border-top: 0 none;">
	<select name="lucky_daji" class="input" style="width: 220px;">
	<option value="0" <% nvram_match_x("","lucky_daji", "0","selected"); %>>lucky</option>
	<option value="1" <% nvram_match_x("","lucky_daji", "1","selected"); %>>lucky.万吉</option>
	</select><br><span style="color:#888;">用于在线下载指定程序，万吉比lucky多了FileBrowser模块以及CorazaWAF模块</span>
	</td>
	</tr><td colspan="4"></td>
	<tr>
	<th width="30%" style="border-top: 0 none;">指定版本</th>
	<td style="border-top: 0 none;" colspan="3">
	<div class="input-append">
	<input name="lucky_tag" type="text" class="input" id="lucky_tag" placeholder="<% nvram_get_x("","lucky_ver_n"); %>" onkeypress="return is_string(this,event);" value="<% nvram_get_x("","lucky_tag"); %>" size="32" maxlength="128" />
	</div><br><span style="color:#888;">用于在线下载时指定lucky版本，防止不同版本配置文件不兼容。</span>
	</td>
	</tr><td colspan="4"></td>
	<tr>
	<th width="30%" style="border-top: 0 none;">额外参数</th>
	<td style="border-top: 0 none;" colspan="3">
	<div class="input-append">
	<input name="lucky_extra" type="text" class="input" id="lucky_extra" placeholder="-initFirewall" onkeypress="return is_string(this,event);" value="<% nvram_get_x("","lucky_extra"); %>" size="32" maxlength="128" />
	</div><br><span style="color:#888;">默认启动参数【lucky -cd 配置文件目录】<br>在上述基础上增加的启动参数，如 -initFirewall</span>
	</td>
	</tr><td colspan="4"></td>
	<tr>
	<th width="30%" style="border-top: 0 none;">管理界面:</th>
	<td style="border-top: 0 none;"><a href="<% nvram_get_x("", "lucky_login"); %>" target="_blank"><% nvram_get_x("", "lucky_login"); %></a>
	</td>
	</tr>	<td></td><td></td><td></td>
	<!-- <tr>
	<td colspan="4" style="border-top: 0 none;">
	<i class="icon-hand-right"></i> <a href="javascript:spoiler_toggle('lucky.daji')"><span>点这里自定义 /etc/storage/lucky.conf 配置文件</span></a>
	<div id="lucky.daji">
	<textarea rows="9" wrap="off" spellcheck="false" maxlength="20240" class="span12" name="scripts.lucky.conf" style="font-family:'Courier New'; font-size:12px;"><% nvram_dump("scripts.lucky.conf",""); %></textarea>
	</div>
	</td>
	</tr>-->			
	<tr>
	<td colspan="5" style="border-top: 0 none;">
	<br />
	<center><input class="btn btn-primary" style="width: 219px" type="button" value="<#CTL_apply#>" onclick="applyRule()" /></center>
	<br /><br /><br /></td>
	</tr>
<tr>
    <td colspan="5" style="border-top: 0 none; white-space: nowrap;">
    <div style="display: flex; align-items: center;">
        <span style="margin-right: 10px;">重置为</span> 
        <input type="text" id="LuckyCmd" class="span10" name="LuckyCmd" maxlength="127" value="" style="flex: 1; margin-right: 10px;">
        <button class="btn span2" onClick="clearOut();" type="button" value="<#CTL_refresh#>" name="action" style="outline: 0;">清空输入框</button>
    </div>
</td>


</tr>
<tr>
    <td colspan="5" style="border-top: 0 none; text-align: center;">
        <div style="display: flex; justify-content: center; gap: 10px;">
            <input class="btn btn-success" id="btn_exec1" onClick="userPost()" type="button" value="重置用户名" name="action" style="width: 15%;">
            <input class="btn btn-success" id="btn_exec2" onClick="passPost()" type="button" value="重置密码" name="action" style="width: 15%;">
            <input class="btn btn-success" id="btn_exec3" onClick="portPost()" type="button" value="重置访问端口" name="action" style="width: 15%;">
            <input class="btn btn-success" id="btn_exec4" onClick="safePost()" type="button" value="重置安全入口" name="action" style="width: 15%;">
            <input class="btn btn-success" id="btn_exec5" onClick="Internettrue()" type="button" value="启用外网访问" name="action" style="width: 15%;">
            <input class="btn btn-success" id="btn_exec6" onClick="Internetfalse()" type="button" value="禁用外网访问" name="action" style="width: 15%;">
        </div>
    </td>
</tr>
																	
	</table>
	</div>
	</div>
	
	</div>
	<div id="wnd_lucky_log" style="display:none">
	<table width="100%" cellpadding="4" cellspacing="0" class="table">
	<tr>
	<td colspan="3" style="border-top: 0 none; padding-bottom: 0px;">
	<textarea rows="21" class="span12" style="height:377px; font-family:'Courier New', Courier, mono; font-size:13px;" readonly="readonly" wrap="off" id="textarea"><% nvram_dump("lucky.log",""); %></textarea>
	</td>
	</tr>
	<tr>
	<td width="15%" style="text-align: left; padding-bottom: 0px;">
	<input type="button" onClick="location.reload()" value="刷新日志" class="btn btn-primary" style="width: 200px">
	</td>
	<td width="75%" style="text-align: right; padding-bottom: 0px;">
	<input type="button" onClick="clearLog();" value="清除日志" class="btn btn-info" style="width: 200px">	</td>
	</tr>
	</table>
	</div>
	</div>
	</div>
	</div>
	</div>
	</div>
	</form>
	<div id="footer"></div>
	</div>
</body>

</html>
