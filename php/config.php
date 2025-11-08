<?
if ($config_conf_key == 1) {

	define("DFLT_WIDTH", 900);
	define("DFLT_HEIGHT", 256);
	define("DFLT_INTERVAL", INT_DAILY);
	
	$db_connect_string = "user = 用户名 password = 密码 dbname = 数据库名 host = pg数据库地址:端口";
}
else {
	die("Error");
}

?>
