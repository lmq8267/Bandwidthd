<?php
// 定义时间常量（秒数）
define("INT_DAILY", 60*60*24*2);   // 每日间隔，2天
define("INT_WEEKLY", 60*60*24*8);  // 每周间隔，8天
define("INT_MONTHLY", 60*60*24*35); // 每月间隔，35天
define("INT_YEARLY", 60*60*24*400); // 每年间隔，400天

// 坐标偏移量
define("XOFFSET", 90);
define("YOFFSET", 45);

$config_conf_key = 1;

require("config.php");

/**
 * 修剪所有 $_GET 输入
 */
function trim_get () 
{
	// 遍历 $_GET 所有键值，去掉前后空格
	foreach ($_GET as $key => $value) {
		$_GET[$key] = trim($_GET[$key]);
	}
}

/**
 * 清理并验证 IP 地址
 * @param string $given_ip 输入的 IP
 * @return string 清理后的 IP 或默认值
 */
function sanitize_ip ($given_ip) 
{
	// 使用正则匹配合法的 IPv4 地址，可带 CIDR 后缀
	if (preg_match("/(1?[1-9]?[0-9]|2?(?:[0-4]?[0-9]|5[0-5]))\.(1?[1-9]?[0-9]|2?(?:[0-4]?[0-9]|5[0-5]))\.(1?[1-9]?[0-9]|2?(?:[0-4]?[0-9]|5[0-5]))\.(1?[1-9]?[0-9]|2?(?:[0-4]?[0-9]|5[0-5]))(\/[0-9]{1,2})?\b/", $given_ip,$ip))
	{
		return $ip[0]; // 返回匹配的 IP
	}
	else
	{
		return "0.0.0.0/0"; // 默认 IP
	}
}

/**
 * 连接数据库
 */
function ConnectDb()
{
	global $db_connect_string;

    $db = pg_pconnect($db_connect_string); // 建立 PostgreSQL 持久连接
    if (!$db)
    {
        printf("⚠️ 错误：无法连接数据库，请检查数据库配置文件！"); // 连接失败提示
        exit(1);
     }
    return($db);
}

/**
 * 格式化字节大小为 K/M/G/T 显示
 * @param int $kbytes 字节数（KB）
 * @return string HTML 表格单元格
 */
function fmtb($kbytes)
{
	$Max = 1024;
	$Output = $kbytes;
	$Suffix = 'K'; // 默认单位 KB

	if ($Output > $Max)
	{
		$Output /= 1024;
		$Suffix = 'M'; // MB
	}

	if ($Output > $Max)
	{
		$Output /= 1024;
		$Suffix = 'G'; // GB
	}

	if ($Output > $Max)
	{
		$Output /= 1024;
		$Suffix = 'T'; // TB
	}
		
	// 返回带格式的 HTML 表格单元格，右对齐，显示单位
	return(sprintf("<td class='%d' style='text-align:right;'>%.1f%s</td>",$kbytes, $Output, $Suffix));
}

// TODO: 检查所需函数和扩展，例如 imagecreate / PHP5_GD

$starttime = time();       // 脚本开始时间
set_time_limit(300);       // 设置脚本最大执行时间为 300 秒
?>
