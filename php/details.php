<?include("include.php");?>

<!DOCTYPE HTML>

<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>节点流量详情</title>
    <link rel="stylesheet" href="css/bootstrap.min.css">
    <!-- <link rel="stylesheet" href="https://unpkg.com/aos@2.3.4/dist/aos.css"> -->
    <link rel="stylesheet" href="css/aos.css">
    <style>
        body {
            background: #f5f7fa;
            font-family: "Microsoft YaHei", sans-serif;
            color: #333;
        }
        .content {
            margin-top: 30px;
            animation: fadeIn 0.8s ease-in-out;
        }
        .logo-container {
            text-align: center; /* 居中 logo */
        }
        .logo-container img {
            max-width: 100%; /* 自适应移动端宽度 */
            height: auto;
        }
        .card {
            border-radius: 15px;
            box-shadow: 0 4px 10px rgba(0,0,0,0.1);
            margin-bottom: 25px;
        }
        .card-header {
            background: linear-gradient(135deg, #007bff 0%, #00c9a7 100%);
            color: white;
            font-size: 18px;
            font-weight: 500;
            border-radius: 15px 15px 0 0;
            text-align: center; /* 居中每个图表标题 */
        }
        table {
            background: white;
            width: 100%;
        }
        th, td {
            text-align: center; /* 表头与表格内容都居中 */
            vertical-align: middle;
            white-space: nowrap; /* 防止换行错位 */
        }
        th {
            background: #e9ecef;
            cursor: pointer;
            font-weight: 600;
        }
        .table-responsive {
            overflow-x: auto;
            -webkit-overflow-scrolling: touch; /* 提升移动端滑动体验 */
        }
        .well {
            background: #ffffff;
            border-radius: 10px;
            box-shadow: inset 0 0 5px rgba(0,0,0,0.05);
            padding: 15px;
        }
        .well strong {
            display: block;
            text-align: center; /* 居中文字 */
            margin-bottom: 10px;
        }
        .well img {
            display: block;
            margin: 0 auto;
            max-width: 100%; /* 图片不超过容器宽度 */
            height: auto;
        }
        footer {
            color: #777;
            font-size: 14px;
            margin-top: 40px;
            text-align: center;
        }
        h3 {
            text-align: center; /* 居中标题 */
            word-wrap: break-word;
        }
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }
        /* 移动端优化 */
        @media (max-width: 768px) {
            h3 {
                font-size: 16px;
            }
            .card-header {
                font-size: 16px;
            }
            th, td {
                font-size: 12px;
                padding: 6px;
            }
        }
        /* 强制表格所有单元格文字水平垂直居中 */
        .table th, 
        .table td {
            text-align: center !important;
            vertical-align: middle !important;
        }
    </style>
</head>
<body>
<div class="container content" data-aos="fade-up">

<!-- 居中的 logo -->
<div class="logo-container mb-3">
    <img src="logo.gif" alt="logo">
</div>

<? 
$starttime = time();
trim_get();

if (isset($_GET['sensor_id'])) {
    $sensor_id = filter_var($_GET['sensor_id'], FILTER_SANITIZE_NUMBER_INT);
} else {
    echo "<div class='alert alert-danger mt-3 text-center'>请提供传感器 ID。</div>";
    exit(1);
}

if (isset($_GET['ip'])) {
    $ip = sanitize_ip($_GET['ip']);
} else {
    echo "<div class='alert alert-danger mt-3 text-center'>请提供 IP 地址。</div>";
    exit(1);
}

echo "<h3 class='mb-4 text-primary'>";
if (strpos($ip, "/") === FALSE) echo "$ip - ".gethostbyaddr($ip)."</h3>";
else echo "汇总视图 - $ip</h3>";

$db = ConnectDb();

if ($ip == "0.0.0.0/0") {
    $rxtable = "bd_rx_total_log";
    $txtable = "bd_tx_total_log";
} else {
    $rxtable = "bd_rx_log";
    $txtable = "bd_tx_log";
}

$sql = "select rx.scale as rxscale, tx.scale as txscale, tx.total+rx.total as total, tx.total as sent,
rx.total as received, tx.tcp+rx.tcp as tcp, tx.udp+rx.udp as udp,
tx.icmp+rx.icmp as icmp, tx.http+rx.http as http,
tx.p2p+rx.p2p as p2p, tx.ftp+rx.ftp as ftp
from
(SELECT ip, max(total/sample_duration)*8 as scale, sum(total) as total, sum(tcp) as tcp, sum(udp) as udp, sum(icmp) as icmp,
sum(http) as http, sum(p2p) as p2p, sum(ftp) as ftp
from sensors, $txtable
where sensors.sensor_id = '$sensor_id'
and sensors.sensor_id = ".$txtable.".sensor_id
and ip <<= '$ip'
group by ip) as tx,
(SELECT ip, max(total/sample_duration)*8 as scale, sum(total) as total, sum(tcp) as tcp, sum(udp) as udp, sum(icmp) as icmp,
sum(http) as http, sum(p2p) as p2p, sum(ftp) as ftp
from sensors, $rxtable
where sensors.sensor_id = '$sensor_id'
and sensors.sensor_id = ".$rxtable.".sensor_id
and ip <<= '$ip'
group by ip) as rx
where tx.ip = rx.ip;";

$result = pg_query($db, $sql);
$r = pg_fetch_array($result);
?>

<div class="table-responsive" data-aos="zoom-in">
<table class="table table-hover table-bordered" style="font-size:14px;">
    <thead class="table-light">
        <tr>
            <th>IP 地址</th>
            <th>主机名</th>
            <th>总流量</th>
            <th>上传</th>
            <th>下载</th>
            <th>TCP</th>
            <th>UDP</th>
            <th>ICMP</th>
            <th>HTTP</th>
            <th>P2P</th>
            <th>FTP</th>
        </tr>
    </thead>
    <tbody>
        <tr>
        <? if (strpos($ip, "/") === FALSE) echo "<td>$ip</td><td>".gethostbyaddr($ip)."</td>";
           else echo "<td>汇总</td><td>$ip</td>"; ?>
        <?= fmtb($r['total']).fmtb($r['sent']).fmtb($r['received']).fmtb($r['tcp']).fmtb($r['udp']).fmtb($r['icmp']).fmtb($r['http']).fmtb($r['p2p']).fmtb($r['ftp']); ?>
        </tr>
    </tbody>
</table>
</div>

<? 
$scale = max($r['txscale'], $r['rxscale']);
$intervals = [
    "Daily" => "每日流量趋势图",
    "Weekly" => "每周流量趋势图",
    "Monthly" => "每月流量趋势图",
    "Yearly" => "每年流量趋势图"
];

foreach ($intervals as $key => $title) {
    $int_const = constant("INT_" . strtoupper($key));
    echo "<div class='card' data-aos='fade-up'>
            <div class='card-header'>$title</div>
            <div class='card-body'>
                <div class='well mb-3'>
                    <strong>上传流量：</strong>
                    <img class='img-fluid rounded shadow-sm' src='graph.php?interval=$int_const&amp;ip=$ip&amp;sensor_id=$sensor_id&amp;table=$txtable&amp;yscale=$scale' alt='上传图表'>
                    <img src='legend.gif' alt='图例' class='mt-2'>
                </div>
                <div class='well'>
                    <strong>下载流量：</strong>
                    <img class='img-fluid rounded shadow-sm' src='graph.php?interval=$int_const&amp;ip=$ip&amp;sensor_id=$sensor_id&amp;table=$rxtable&amp;yscale=$scale' alt='下载图表'>
                    <img src='legend.gif' alt='图例' class='mt-2'>
                </div>
            </div>
        </div>";
}
?>

<footer class="mt-5" data-aos="fade-up">
    <hr>
    <p>页面加载完成，用时 <span class="text-success fw-bold"><?= (time() - $starttime) ?></span> 秒</p>
    <p>© <?=date('Y')?> BandWidthd - 网络流量监控系统 </p>
</footer>
<!-- <script src="https://unpkg.com/aos@2.3.4/dist/aos.js"></script> -->
<script src="js/aos.js"></script>
<script>
AOS.init({ duration: 800, once: true });
</script>

</body>
</html>
