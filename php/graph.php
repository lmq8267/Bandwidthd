<?
error_reporting(E_ALL & ~E_DEPRECATED);
require("include.php");
define('FONT_PATH', __DIR__ . '/fonts/msyh.ttf');  
define('FONT_SIZE', 10);
setlocale(LC_TIME, 'zh_CN.UTF-8');  
date_default_timezone_set('Asia/Shanghai');
$weekdays_cn = array('日', '一', '二', '三', '四', '五', '六');  
$months_cn = array('', '1月', '2月', '3月', '4月', '5月', '6月',   
                   '7月', '8月', '9月', '10月', '11月', '12月');  
  
// 修改日期格式化函数  
function format_date_cn($timestamp) {  
    global $weekdays_cn, $months_cn;  
    $w = date('w', $timestamp);  
    $m = date('n', $timestamp);  
    $d = date('j', $timestamp);  
    return "{$months_cn[$m]}{$d}日-周{$weekdays_cn[$w]}";  
}  
  
function format_month_cn($timestamp) {  
    global $months_cn;  
    $m = date('n', $timestamp);  
    $y = date('Y', $timestamp);  
    return "{$y}年{$months_cn[$m]}";  
}
// 添加小时分钟格式化函数  
function format_time_cn($timestamp) {  
    return date('H:i', $timestamp);  
}  
  
function format_datetime_cn($timestamp) {  
    global $months_cn;  
    $y = date('Y', $timestamp);  
    $m = date('n', $timestamp);  
    $d = date('j', $timestamp);  
    $h = date('H:i', $timestamp);  
    return "{$y}年{$months_cn[$m]}{$d}日{$h}";  
}
// Returns x location of any given timestamp
function ts2x($ts)
	{
	global $timestamp, $width, $interval;
	return(($ts-$timestamp)*(($width-XOFFSET) / $interval) + XOFFSET);
	}

// If we have multiple IP's in a result set we need to total the average of each IP's samples
function AverageAndAccumulate()
	{
	global $Count, $total, $icmp, $udp, $tcp, $ftp, $http, $p2p, $YMax;
	global $a_total, $a_icmp, $a_udp, $a_tcp, $a_ftp, $a_http, $a_p2p;
	
	foreach ($Count as $key => $number)
    	{
	    $total[$key] /= $number;
    	$icmp[$key] /= $number;
    	$udp[$key] /= $number;
    	$tcp[$key] /= $number;
    	$ftp[$key] /= $number;
    	$http[$key] /= $number;
    	$p2p[$key] /= $number;
    	}

	foreach ($Count as $key => $number) 
	{
		if (!isset($a_total[$key])) {
			$a_total[$key] = 0;
		}
		if (!isset($a_icmp[$key])) {
			$a_icmp[$key] = 0;
		}
		if (!isset($a_udp[$key])) {
			$a_udp[$key] = 0;
		}
		if (!isset($a_tcp[$key])) {
			$a_tcp[$key] = 0;
		}
		if (!isset($a_ftp[$key])) {
			$a_ftp[$key] = 0;
		}
		if (!isset($a_http[$key])) {
			$a_http[$key] = 0;
		}
		if (!isset($a_p2p[$key])) {
			$a_p2p[$key] = 0;
		}
			
			
			
		$a_total[$key] += $total[$key];
		$a_icmp[$key] += $icmp[$key];
		$a_udp[$key] += $udp[$key];
		$a_tcp[$key] += $tcp[$key];
		$a_ftp[$key] += $ftp[$key];
		$a_http[$key] += $http[$key];
		$a_p2p[$key] += $p2p[$key];

		if ($a_total[$key] > $YMax)
			$YMax = $a_total[$key];
		}
	
	unset($GLOBALS['total'], $GLOBALS['icmp'], $GLOBALS['udp'], $GLOBALS['tcp'], $GLOBALS['ftp'], $GLOBALS['http'], $GLOBALS['p2p'], $GLOBALS['Count']);

	$total = array();
	$icmp = array();
	$udp = array();
	$tcp = array();
	$ftp = array();
	$http = array();
	$p2p = array();
	$Count = array();
	}
 

$db = ConnectDb();

// Get parameters
trim_get ();

if (isset($_GET['width']))
    $width = filter_var($_GET['width'], FILTER_SANITIZE_NUMBER_INT); 
else
	$width = DFLT_WIDTH;

if (isset($_GET['height']))
    $height = filter_var($_GET['height'], FILTER_SANITIZE_NUMBER_INT);  
else
	$height = DFLT_HEIGHT;

if (isset($_GET['interval']))
    $interval = filter_var($_GET['interval'], FILTER_SANITIZE_NUMBER_INT); 
else
	$interval = DFLT_INTERVAL;

if (isset($_GET['ip']))
    $ip =  sanitize_ip ($_GET['ip']);
else
	exit(1);

if (isset($_GET['sensor_id']))
	$sensor_id = filter_var($_GET['sensor_id'], FILTER_SANITIZE_NUMBER_INT);  
else
	exit(1);

if (isset($_GET['timestamp']))
    $timestamp = filter_var($_GET['timestamp'], FILTER_SANITIZE_NUMBER_INT); 
else
	$timestamp = time() - $interval + (0.05*$interval);

//make sure just real tables can be accessed
if (isset($_GET['table']) 
	&& ($_GET['table'] == "bd_tx_total_log" || $_GET['table'] == "bd_tx_log" || $_GET['table'] == "bd_rx_total_log" || $_GET['table'] == "bd_rx_log"))
    $table = $_GET['table'];
else
	$table = "bd_rx_log";

if (isset($_GET['yscale']))
    $yscale = filter_var($_GET['yscale'], FILTER_SANITIZE_NUMBER_INT); 

$total = array();
$icmp = array();
$udp = array();
$tcp = array();
$ftp = array();
$http = array();
$p2p = array();
$Count = array();

// Accumulator
$a_total = array();
$a_icmp = array();
$a_udp = array();
$a_tcp = array();
$a_ftp = array();
$a_http = array();
$a_p2p = array();

$sql = "select *, extract(epoch from timestamp) as ts from sensors, $table where sensors.sensor_id = ".$table.".sensor_id and ip <<= '$ip' and sensors.sensor_id = '$sensor_id' and timestamp > to_timestamp($timestamp) and timestamp < to_timestamp(".($timestamp+$interval).") order by ip;";
//echo $sql."<br>"; exit(1);
$result = pg_query($db, $sql);

// The SQL statement pulls the data out of the database ordered by IP address, that way we can average each
// datapoint for each IP address to provide smoothing and then toss the smoothed value into the accumulator
// to provide accurate total traffic rate.

$last_ip = "";
$SentPeak = 0;
$TotalSent = 0;
while ($row = pg_fetch_array($result))
	{
	if ($row['ip'] != $last_ip)
		{
		AverageAndAccumulate();
		$last_ip = $row['ip'];
		}

	$x = ($row['ts']-$timestamp)*(($width-XOFFSET)/$interval)+XOFFSET;
	$xint = (int) $x;

	//echo "xint: ".$xint."<br>";
	if (!isset($Count[$xint])) {
		$Count[$xint] = 0;
	}
	$Count[$xint]++;

	if (!isset($total[$xint])) {
		$total[$xint] = 0;
	}
	if (!isset($icmp[$xint])) {
		$icmp[$xint] = 0;
	}
	if (!isset($udp[$xint])) {
		$udp[$xint] = 0;
	}
	if (!isset($tcp[$xint])) {
		$tcp[$xint] = 0;
	}		
	if (!isset($ftp[$xint])) {
		$ftp[$xint] = 0;
	}
	if (!isset($http[$xint])) {
		$http[$xint] = 0;
	}
	if (!isset($p2p[$xint])) {
		$p2p[$xint] = 0;
	}		
	
	
	if ($row['total']/$row['sample_duration'] > $SentPeak)
		$SentPeak = $row['total']/$row['sample_duration'];
	$TotalSent += $row['total'];
	$total[$xint] += $row['total']/$row['sample_duration'];
	$icmp[$xint] += $row['icmp']/$row['sample_duration'];
	$udp[$xint] += $row['udp']/$row['sample_duration'];
	$tcp[$xint] += $row['tcp']/$row['sample_duration'];
	$ftp[$xint] += $row['ftp']/$row['sample_duration'];
	$http[$xint] += $row['http']/$row['sample_duration'];
	$p2p[$xint] += $row['p2p']/$row['sample_duration'];                                                                                                                             
	}

// One more time for the last IP
AverageAndAccumulate();

// Pull the data out of Accumulator
$total = $a_total;
$icmp = $a_icmp;
$udp = $a_udp;
$tcp = $a_tcp;
$ftp = $a_ftp;
$http = $a_http;
$p2p = $a_p2p;

$YMax += $YMax*0.05;    // Add an extra 5%

// if a y scale was specified override YMax
if (isset($yscale))
    $YMax = $yscale/8;

// Plot the data

header("Content-type: image/png");

$im = imagecreate($width, $height);
$white = imagecolorallocate($im, 255, 255, 255);
$yellow = ImageColorAllocate($im, 255, 255, 0);
$purple = ImageColorAllocate($im, 255, 0, 255);
$green  = ImageColorAllocate($im, 0, 255, 0);
$blue   = ImageColorAllocate($im, 0, 0, 255);
$lblue  = ImageColorAllocate($im, 128, 128, 255);
$brown  = ImageColorAllocate($im, 128, 0, 0);
$red    = ImageColorAllocate($im, 255, 0, 0);
$black  = ImageColorAllocate($im, 0, 0, 0); 
  
// 计算每个柱子应该覆盖的时间范围  
$barTimeSpan = 5*60; // 默认5分钟  
if ($interval <= 60*60) {  
    $barTimeSpan = 5*60; // 5分钟  
} else if ($interval <= 3*60*60) {  
    $barTimeSpan = 10*60; // 10分钟  
} else if ($interval <= 6*60*60) {  
    $barTimeSpan = 30*60; // 30分钟  
} else if ($interval <= 24*60*60) {  
    $barTimeSpan = 60*60; // 1小时  
} else {  
    $barTimeSpan = $interval / (($width-XOFFSET) / 10); // 动态计算  
}  
  
// 计算柱子宽度(像素)  
$barWidth = max(1, floor(($barTimeSpan * ($width-XOFFSET)) / $interval));  
  
// 按时间分组绘制柱状图  
for($timeOffset = 0; $timeOffset < $interval; $timeOffset += $barTimeSpan) {  
    $barStartTime = $timestamp + $timeOffset;  
    $barEndTime = $barStartTime + $barTimeSpan;  
      
    // 计算这个柱子的X坐标  
    $barX = ts2x($barStartTime);  
      
    if ($barX < XOFFSET || $barX >= $width) continue;  
      
    // 收集这个时间段内的所有数据点  
    $barTotal = 0;  
    $barIcmp = 0;  
    $barUdp = 0;  
    $barTcp = 0;  
    $barFtp = 0;  
    $barHttp = 0;  
    $barP2p = 0;  
    $dataCount = 0;  
      
    for($x = floor($barX); $x < floor($barX) + $barWidth && $x < $width; $x++) {  
        if (isset($total[$x])) {  
            $barTotal += $total[$x];  
            $barIcmp += $icmp[$x];  
            $barUdp += $udp[$x];  
            $barTcp += $tcp[$x];  
            $barFtp += $ftp[$x];  
            $barHttp += $http[$x];  
            $barP2p += $p2p[$x];  
            $dataCount++;  
        }  
    }  
      
    // 如果这个时间段有数据,计算平均值并绘制  
    if ($dataCount > 0) {  
        $barTotal /= $dataCount;  
        $barIcmp /= $dataCount;  
        $barUdp /= $dataCount;  
        $barTcp /= $dataCount;  
        $barFtp /= $dataCount;  
        $barHttp /= $dataCount;  
        $barP2p /= $dataCount;  
          
        // 转换为Y坐标  
        if ($YMax != 0) {  
            $barTotal = ($barTotal*($height-YOFFSET))/$YMax;  
            $barIcmp = ($barIcmp*($height-YOFFSET))/$YMax;  
            $barUdp = ($barUdp*($height-YOFFSET))/$YMax;  
            $barTcp = ($barTcp*($height-YOFFSET))/$YMax;  
            $barFtp = ($barFtp*($height-YOFFSET))/$YMax;  
            $barHttp = ($barHttp*($height-YOFFSET))/$YMax;  
            $barP2p = ($barP2p*($height-YOFFSET))/$YMax;  
        }  
          
        // 堆叠逻辑  
        $barUdp += $barIcmp;  
        $barTcp += $barUdp;  
        $barP2p += $barTcp;  
        $barHttp += $barP2p;  
        $barFtp += $barHttp;  
          
        // 绘制填充矩形  
        imagefilledrectangle($im,   
            floor($barX), ($height-YOFFSET) - $barTotal,   
            floor($barX) + $barWidth, $height-YOFFSET,   
            $yellow);  
          
        if ($barIcmp > 0) {  
            imagefilledrectangle($im,   
                floor($barX), ($height-YOFFSET) - $barIcmp,   
                floor($barX) + $barWidth, $height-YOFFSET,   
                $red);  
        }  
          
        if ($barUdp > $barIcmp) {  
            imagefilledrectangle($im,   
                floor($barX), ($height-YOFFSET) - $barUdp,   
                floor($barX) + $barWidth, ($height-YOFFSET) - $barIcmp,   
                $brown);  
        }  
          
        if ($barTcp > $barUdp) {  
            imagefilledrectangle($im,   
                floor($barX), ($height-YOFFSET) - $barTcp,   
                floor($barX) + $barWidth, ($height-YOFFSET) - $barUdp,   
                $green);  
        }  
          
        if ($barP2p > $barTcp) {  
            imagefilledrectangle($im,   
                floor($barX), ($height-YOFFSET) - $barP2p,   
                floor($barX) + $barWidth, ($height-YOFFSET) - $barTcp,   
                $purple);  
        }  
          
        if ($barHttp > $barP2p) {  
            imagefilledrectangle($im,   
                floor($barX), ($height-YOFFSET) - $barHttp,   
                floor($barX) + $barWidth, ($height-YOFFSET) - $barP2p,   
                $blue);  
        }  
          
        if ($barFtp > $barHttp) {  
            imagefilledrectangle($im,   
                floor($barX), ($height-YOFFSET) - $barFtp,   
                floor($barX) + $barWidth, ($height-YOFFSET) - $barHttp,   
                $lblue);  
        }  
    }  
}                                                                                                                    

// Margin Text
if ($table == "bd_tx_total_log" || $table == "bd_tx_log")
	$tx_rx_text = "上传";
else 
	$tx_rx_text = "下载";

	//|| $_GET['table'] == "bd_rx_total_log" || $_GET['table'] == "bd_rx_log"))
	
	
	
if ($SentPeak < 1024/8)
	$txtPeakSendRate = sprintf("%s峰值速率: %.1fK/秒", $tx_rx_text, $SentPeak*8);
else if ($SentPeak < (1024*1024)/8)
    $txtPeakSendRate = sprintf("%s峰值速率: %.1fM/秒", $tx_rx_text, ($SentPeak*8.0)/1024.0);
else 
	$txtPeakSendRate = sprintf("%s峰值速率: %.1fG/秒", $tx_rx_text, ($SentPeak*8.0)/(1024.0*1024.0));
                                                                                                                             
if ($TotalSent < 1024)
	$txtTotalSent = sprintf("%s用量: %.1fK", $tx_rx_text, $TotalSent);
else if ($TotalSent < 1024*1024)
	$txtTotalSent = sprintf("%s用量: %.1fM", $tx_rx_text, $TotalSent/1024.0);
else 
	$txtTotalSent = sprintf("%s用量: %.1fG", $tx_rx_text, $TotalSent/(1024.0*1024.0));
                                                                                                                             
imagettftext($im, FONT_SIZE, 0, XOFFSET+5, $height-20+FONT_SIZE, $black, FONT_PATH, $txtTotalSent);  
imagettftext($im, FONT_SIZE, 0, $width/2+XOFFSET/2, $height-20+FONT_SIZE, $black, FONT_PATH, $txtPeakSendRate);  

// Draw X Axis

ImageLine($im, 0, $height-YOFFSET, $width, $height-YOFFSET, $black);

// Day/Month Seperator bars

// 对于小于24小时的时间间隔,使用小时:分钟格式标注  
if ($interval < 24*60*60) {  
    // 计算合适的标注间隔  
    $labelInterval = 60*60; // 默认1小时  
    if ($interval <= 1*60*60) {  
        $labelInterval = 10*60; // 1小时:每10分钟一个标注 
	} else if ($interval <= 3*60*60) {  
        $labelInterval = 30*60; // 2-3小时:每半小时一个标注
    } else if ($interval <= 6*60*60) {  
        $labelInterval = 60*60; // 3-6小时:每1小时一个标注
	} else if ($interval <= 16*60*60) {  
        $labelInterval = 2*60*60; // 6-16小时:每2小时一个标注
    } else {  
        $labelInterval = 4*60*60; // 6-24小时:每4小时一个标注  
    }  
      
    $ts = getdate($timestamp);  
    // 对齐到标注间隔的整数倍  
    $minutes = floor($ts['minutes'] / ($labelInterval/60)) * ($labelInterval/60);  
    $MarkTime = mktime($ts['hours'], $minutes, 0, $ts['mon'], $ts['mday'], $ts['year']);  
      
    $x = ts2x($MarkTime);  
    while ($x < XOFFSET) {  
        $MarkTime += $labelInterval;  
        $x = ts2x($MarkTime);  
    }  
      
    while ($x < ($width-10)) {  
        if ($x > XOFFSET) {  
            // 绘制分隔线  
            ImageLine($im, $x, 0, $x, $height-YOFFSET, $black);  
              
            // 格式化时间标签:月日 时:分  
            $txtTime = date('m月d日 H:i', $MarkTime);  
            // 调整标签位置,避免重叠:向左偏移40像素  
            imagettftext($im, FONT_SIZE, 0, $x-40, $height-YOFFSET+10+FONT_SIZE, $black, FONT_PATH, $txtTime);  
        }  
          
        $MarkTime += $labelInterval;  
        $x = ts2x($MarkTime);  
    }  
} else if ((24*60*60*($width-XOFFSET))/$interval > ($width-XOFFSET)/10) {  
	$ts = getdate($timestamp);
	$MarkTime = mktime(0, 0, 0, $ts['mon'], $ts['mday'], $ts['year']);
	
    $x = ts2x($MarkTime);
    while ($x < XOFFSET)
    	{
        $MarkTime += (24*60*60);
	    $x = ts2x($MarkTime);
        }
                                                                                                                             
    while ($x < ($width-10))
    	{
        // Day Lines
        ImageLine($im, $x, 0, $x, $height-YOFFSET, $black);
        ImageLine($im, $x+1, 0, $x+1, $height-YOFFSET, $black);
                                                                                                                             
        $txtDate = format_date_cn($MarkTime);
        imagettftext($im, FONT_SIZE, 0, $x-30, $height-YOFFSET+10+FONT_SIZE, $black, FONT_PATH, $txtDate);  
                                                                                                                             
        // Calculate Next x
        $MarkTime += (24*60*60);
	    $x = ts2x($MarkTime);
        }
	}
else if ((24*60*60*30*($width-XOFFSET))/$interval > ($width-XOFFSET)/10)
	{
	// Monthly Bars
	$ts = getdate($timestamp);
	$month = $ts['mon'];
	$MarkTime = mktime(0, 0, 0, $month, 1, $ts['year']);
	
    $x = ts2x($MarkTime);
    while ($x < XOFFSET)
    	{
		$month++;
        $MarkTime = mktime(0, 0, 0, $month, 1, $ts['year']);
	    $x = ts2x($MarkTime);
        }
                                                                                                                             
    while ($x < ($width-10))
    	{
        // Day Lines
        ImageLine($im, $x, 0, $x, $height-YOFFSET, $black);
        ImageLine($im, $x+1, 0, $x+1, $height-YOFFSET, $black);
                                                                                                                             
        $txtDate = format_month_cn($MarkTime);
        imagettftext($im, FONT_SIZE, 0, $x-25, $height-YOFFSET+10+FONT_SIZE, $black, FONT_PATH, $txtDate);  
                                                                                                                             
        // Calculate Next x
		$month++;
        $MarkTime = mktime(0, 0, 0, $month, 1, $ts['year']);
	    $x = ts2x($MarkTime);
        }
	}
else
	{
	// Year Bars
    $ts = getdate($timestamp);
    $year = $ts['year'];
    $MarkTime = mktime(0, 0, 0, 1, 1, $year);
                                                                                                                             
    $x = ts2x($MarkTime);
    while ($x < XOFFSET)
        {
        $year++;
        $MarkTime = mktime(0, 0, 0, 1, 1, $year);
        $x = ts2x($MarkTime);
        }
                                                                                                                             
    while ($x < ($width-10))
        {
        // Day Lines
        ImageLine($im, $x, 0, $x, $height-YOFFSET, $black);
        ImageLine($im, $x+1, 0, $x+1, $height-YOFFSET, $black);
                                                                                                                             
        $txtDate = format_month_cn($MarkTime);
        imagettftext($im, FONT_SIZE, 0, $x-25, $height-YOFFSET+10+FONT_SIZE, $black, FONT_PATH, $txtDate);  
                                                                                                                             
        // Calculate Next x
        $year++;
        $MarkTime = mktime(0, 0, 0, 1, 1, $year);
        $x = ts2x($MarkTime);
        }	
	}

// Draw Major Tick Marks
if ((6*60*60*($width-XOFFSET))/$interval > 10) // pixels per 6 hours is more than 2
	$MarkTimeStep = 6*60*60; // Major ticks are 6 hours
else if ((24*60*60*($width-XOFFSET))/$interval > 10)
	$MarkTimeStep = 24*60*60; // Major ticks are 24 hours;
else if ((24*60*60*30*($width-XOFFSET))/$interval > 10)
	{
	// Major tick marks are months
	$MarkTimeStep = 0; // Skip the standard way of drawing major tick marks below

    $ts = getdate($timestamp);
    $month = $ts['mon'];
    $MarkTime = mktime(0, 0, 0, $month, 1, $ts['year']);
                                                                                                                             
    $x = ts2x($MarkTime);
    while ($x < XOFFSET)
        {
        $month++;
        $MarkTime = mktime(0, 0, 0, $month, 1, $ts['year']);
        $x = ts2x($MarkTime);
        }
                                                                                                                             
    while ($x < ($width-10))
        {
        // Day Lines
		$date = getdate($MarkTime);
		if ($date['mon'] != 1)
			{
	        ImageLine($im, $x, $height-YOFFSET-5, $x, $height-YOFFSET+5, $black);                                                                                                                      
    	    $txtDate = $months_cn[date('n', $MarkTime)];
        	imagettftext($im, FONT_SIZE, 0, $x-5, $height-YOFFSET+10+FONT_SIZE, $black, FONT_PATH, $txtDate);  
          	}
                                                                                                                   
        // Calculate Next x
        $month++;
        $MarkTime = mktime(0, 0, 0, $month, 1, $ts['year']);
        $x = ts2x($MarkTime);
        }
	}
else
	$MarkTimeStep = 0; // Skip Major Tick Marks

if ($MarkTimeStep)
	{
	$ts = getdate($timestamp);
	$MarkTime = mktime(0, 0, 0, $ts['mon'], $ts['mday'], $ts['year']);
	$x = ts2x($MarkTime);

	while ($x < ($width-10))
		{
    	if ($x > XOFFSET) 
			{
    	    ImageLine($im, $x, $height-YOFFSET-5, $x, $height-YOFFSET+5, $black);
	        }
		$MarkTime += $MarkTimeStep;
	    $x = ts2x($MarkTime);
		}
	}

// Draw Minor Tick marks  
if ($interval <= 60*60) {  
    // 1小时及以下:5分钟刻度  
    $MarkTimeStep = 5*60;  
} else if ($interval <= 3*60*60) {  
    // 1-3小时:10分钟刻度  
    $MarkTimeStep = 10*60;  
} else if ($interval <= 6*60*60) {  
    // 3-6小时:30分钟刻度  
    $MarkTimeStep = 30*60;  
} else if ($interval <= 24*60*60) {  
    // 6-24小时:1小时刻度  
    $MarkTimeStep = 60*60;  
} else if ((60*60*($width-XOFFSET))/$interval > 4) {  
    $MarkTimeStep = 60*60;  
} else if ((6*60*60*($width-XOFFSET))/$interval > 4) {  
    $MarkTimeStep = 6*60*60;  
} else if ((24*60*60*($width-XOFFSET))/$interval > 4) {  
    $MarkTimeStep = 24*60*60;  
} else {  
    $MarkTimeStep = 0;  
}  
  
if ($MarkTimeStep) {  
    $ts = getdate($timestamp);  
    $MarkTime = mktime($ts['hours'], floor($ts['minutes']/($MarkTimeStep/60))*($MarkTimeStep/60), 0, $ts['mon'], $ts['mday'], $ts['year']);  
    $x = ts2x($MarkTime);  
  
    while ($x < ($width-10)) {  
        if ($x > XOFFSET) {  
            ImageLine($im, $x, $height-YOFFSET, $x, $height-YOFFSET+5, $black);  
        }  
        $MarkTime += $MarkTimeStep;  
        $x = ts2x($MarkTime);  
    }  
}

// Draw Y Axis
ImageLine($im, XOFFSET, 0, XOFFSET, $height, $black);

$YLegend = 'k';
$Divisor = 1;
if ($YMax*8 > 1024*2)
	{
    $Divisor = 1024;    // Display in m
    $YLegend = 'M';
    }

if ($YMax*8 > 1024*1024*2)
	{
    $Divisor = 1024*1024; // Display in g
    $YLegend = 'G';
	}

if ($YMax*8 > 1024*1024*1024*2)
	{
    $Divisor = 1024*1024*1024; // Display in t
    $YLegend = 'T';
    }
                                                                                                                             
$YStep = $YMax/10;
if ($YStep < 1)
	$YStep=1;
$YTic=$YStep;
                                                                                                                             
while ($YTic <= ($YMax - $YMax/10))
	{
    $y = ($height-YOFFSET)-(($YTic*($height-YOFFSET))/$YMax);
	ImageLine($im, XOFFSET, $y, $width, $y, $black);
    $txtYLegend = sprintf("%4.1f%s/秒", (8.0*$YTic)/$Divisor, $YLegend);
    imagettftext($im, FONT_SIZE, 0, 3, $y-7+FONT_SIZE, $black, FONT_PATH, $txtYLegend);
	$YTic += $YStep;
	}

imagepng($im); 
imagedestroy($im);
