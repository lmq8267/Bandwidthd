<?php include("include.php"); ?>

<!DOCTYPE HTML>

<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>BandWidthd - 网络带宽监控系统</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<!-- <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet"> -->
<link href="css/bootstrap.min.css" rel="stylesheet">
<link href="https://cdnjs.cloudflare.com/ajax/libs/animate.css/4.1.1/animate.min.css" rel="stylesheet">
<!-- <script src="https://code.jquery.com/jquery-3.7.1.min.js"></script> -->
<script src="js/jquery-3.7.1.min.js"></script>
<!-- <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js"></script> -->
<script src="js/bootstrap.bundle.min.js"></script>

<style>
body { background: linear-gradient(135deg, #f8f9fa, #e9f3ff); font-family: "Microsoft YaHei", sans-serif; }
#logo { width: 400px; animation: fadeInDown 1s ease; }
.navbar { box-shadow: 0 2px 10px rgba(0,0,0,0.05); }
h2,h3,h4 { color: #0d6efd; }
.form-select, .form-control { border-radius: 8px; }
.btn-success { transition: all 0.3s ease; }
.btn-success:hover { transform: scale(1.05); }
.table th { background-color: #0d6efd; color: white; text-align: center; cursor: pointer; user-select: none; position: relative; }
.table td { text-align: center; vertical-align: middle; }
.table-striped tbody tr:nth-of-type(odd) { background-color: #f2f9ff; }
.panel { background-color: #fff; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.05); margin-bottom: 25px; overflow: hidden; }
.panel-heading { background-color: #e9f3ff; padding: 12px 20px; }
.panel-title a { text-decoration: none; color: #0d6efd; font-weight: bold; }
.panel-body { padding: 20px; }
.well { background: #f8f9fa; border-radius: 10px; padding: 15px; margin-bottom: 15px; box-shadow: inset 0 0 6px rgba(0,0,0,0.05); transition: all 0.3s ease; }
.well:hover { background: #fff; }
.graph-img { width: 100%; max-width: 100%; border-radius: 6px; transition: all 0.3s ease; }
.graph-img:hover { transform: scale(1.02); }
#backTop { position: fixed; bottom: 30px; right: 30px; display: none; background: linear-gradient(135deg,#0d6efd,#6610f2); color: white; border: none; width: 50px; height: 50px; border-radius: 50%; cursor: pointer; font-size: 22px; z-index: 9999; box-shadow: 0 4px 12px rgba(0,0,0,0.3); display: flex; justify-content: center; align-items: center; transition: all 0.3s ease; }
#backTop:hover { transform: scale(1.2); background: linear-gradient(135deg,#6610f2,#0d6efd); }
.sort-arrow { font-size: 12px; margin-left: 5px; }
.table th, .table td { text-align: center !important; vertical-align: middle !important; }

</style>

<script>
$(document).ready(function(){  
    // 滚动到顶部按钮  
    $(window).scroll(function(){   
        if($(window).scrollTop() > 200) {   
            $("#backTop").fadeIn();   
        } else {   
            $("#backTop").fadeOut();   
        }   
    });  
      
    // 当选择"自定义时间段"时,显示模态框  
    // 不需要从localStorage或隐藏字段读取值,直接使用PHP设置的默认值  
    $('#intervalSelect').change(function(){    
        if($(this).val() === 'custom'){  
            var myModal = new bootstrap.Modal(document.getElementById('customTimeModal'));    
            myModal.show();    
        }    
    });  
      
    // 编辑按钮点击事件  
    $('#editCustomTime').click(function(){  
        var myModal = new bootstrap.Modal(document.getElementById('customTimeModal'));  
        myModal.show();  
    });  
        
    // 确认自定义时间(不提交表单,只更新显示)    
    $('#confirmCustomTime').click(function(){      
        var startTime = $('#startTime').val();      
        var endTime = $('#endTime').val();      
            
        if(!startTime || !endTime) {      
            alert('请选择起始和结束时间');      
            return;      
        }      
            
        var start = new Date(startTime);      
        var end = new Date(endTime);      
            
        if(end <= start) {      
            alert('结束时间必须晚于起始时间');      
            return;      
        } 
        // 新增:验证时间间隔是否至少为1小时  
        var timeDiff = end.getTime() - start.getTime(); // 毫秒差  
        var hoursDiff = timeDiff / (1000 * 60 * 60); // 转换为小时  
      
        if(hoursDiff < 1) {  
            alert('自定义时间段最少需要1小时,当前选择的时间间隔为 ' + Math.round(hoursDiff * 60) + ' 分钟,请重新选择');  
            return;  
        }
            
        // 更新隐藏字段    
        $('#hiddenStartTime').val(startTime);    
        $('#hiddenEndTime').val(endTime);    
            
        // 格式化时间显示    
        var startStr = start.getFullYear() + '年' +     
                      (start.getMonth()+1) + '月' +     
                      start.getDate() + '日' +     
                      String(start.getHours()).padStart(2,'0') + ':' +     
                      String(start.getMinutes()).padStart(2,'0');    
                          
        var endStr = end.getFullYear() + '年' +     
                    (end.getMonth()+1) + '月' +     
                    end.getDate() + '日' +     
                    String(end.getHours()).padStart(2,'0') + ':' +     
                    String(end.getMinutes()).padStart(2,'0');    
            
        // 使用 .html() 更新下拉菜单显示文本    
        $('#customOption').html(startStr + '-' + endStr);    
          
        // 确保下拉菜单保持选中 custom 选项  
        $('#intervalSelect').val('custom');  
          
        // 显示编辑按钮  
        if($('#hiddenStartTime').val() && $('#hiddenEndTime').val()) {  
            $('#editCustomTime').show();  
        }  
            
        // 关闭模态框    
        var myModal = bootstrap.Modal.getInstance(document.getElementById('customTimeModal'));    
        myModal.hide();    
    });    
          
    // 模态框关闭时的处理  
    $('#customTimeModal').on('hidden.bs.modal', function () {      
        if(!$('#hiddenStartTime').val() || !$('#hiddenEndTime').val()) {      
            $('#intervalSelect').val('none');  
            $('#customOption').html('自定义时间段');  
            $('#editCustomTime').hide();  
        }      
    });  
      
    // 页面加载时检查是否有自定义时间,如果有则显示编辑按钮  
    if($('#hiddenStartTime').val() && $('#hiddenEndTime').val()) {  
        $('#editCustomTime').show();  
    }  
});
  
function backToTop(){   
    $('html,body').animate({scrollTop:0},300);   
}  
    
let sortOrder = {}; // 保存每列排序顺序
function sortTable(colIndex) {
    const table = $(".table tbody");
    let rows = table.find("tr").toArray();
    const key = $(".table thead th").eq(colIndex).text();
    let order = sortOrder[key] === "asc" ? "desc" : "asc";
    sortOrder[key] = order;

    // 将带单位的流量转换为字节数
    function parseFlow(val){
        if(!val) return 0;
        let unit = val.replace(/[\d.]/g,"").trim().toUpperCase();
        let num = parseFloat(val);
        switch(unit){
            case "T": return num*1024*1024*1024*1024;
            case "G": return num*1024*1024*1024;
            case "M": return num*1024*1024;
            case "K": return num*1024;
            default: return num;
        }
    }

    // IP 地址转成整数数组用于比较
    function parseIP(ip){
        return ip.split(".").map(x=>parseInt(x,10));
    }

    rows.sort((a,b)=>{
        let valA = $(a).children().eq(colIndex).text().trim();
        let valB = $(b).children().eq(colIndex).text().trim();

        // 如果是 IP 地址列
        if(key.includes("IP")){
            let ipA = parseIP(valA), ipB = parseIP(valB);
            for(let i=0;i<4;i++){
                if(ipA[i] !== ipB[i]) return order==="asc" ? ipA[i]-ipB[i] : ipB[i]-ipA[i];
            }
            return 0;
        } else { // 流量列
            valA = parseFlow(valA);
            valB = parseFlow(valB);
            return order==="asc" ? valA - valB : valB - valA;
        }
    });
    table.append(rows);

    // 更新箭头
    $(".table th .sort-arrow").remove();
    $(".table thead th").eq(colIndex).append('<span class="sort-arrow">'+(order==="asc"?"▲":"▼")+'</span>');
}

</script>

</head>
<body>
<div class="container mt-4">
<div class="text-center mb-4">
<img src="logo.gif" id="logo" alt="">
</div>

<?php        
trim_get();        
if (isset($_GET['sensor_id']) && $_GET['sensor_id'] != "none")         
    $sensor_id = filter_var($_GET['sensor_id'], FILTER_SANITIZE_NUMBER_INT);        
        
// 处理自定义时间段 - 添加非空验证  
if (isset($_GET['start_time']) && isset($_GET['end_time']) &&   
    $_GET['start_time'] !== '' && $_GET['end_time'] !== '' &&  
    isset($_GET['interval']) && $_GET['interval'] == 'custom') {        
    $start_time = strtotime($_GET['start_time']);        
    $end_time = strtotime($_GET['end_time']);        
            
    if ($start_time && $end_time && $end_time > $start_time) {        
        $timestamp = $start_time;        
        $interval = $end_time - $start_time;    
    } else {        
        $interval = DFLT_INTERVAL;        
        $timestamp = time() - $interval + (0.05 * $interval);        
    }        
} else {    
    // 当选择非custom的时间间隔时,清除start_time和end_time参数    
    if (isset($_GET['start_time'])) unset($_GET['start_time']);    
    if (isset($_GET['end_time'])) unset($_GET['end_time']);    
        
    if (isset($_GET['interval']) && $_GET['interval'] != "none" && $_GET['interval'] != 'custom')         
        $interval = filter_var($_GET['interval'], FILTER_SANITIZE_NUMBER_INT);         
    else         
        $interval = DFLT_INTERVAL;        
            
    if (isset($_GET['timestamp']) && $_GET['timestamp'] != "none")         
        $timestamp = filter_var($_GET['timestamp'], FILTER_SANITIZE_NUMBER_INT);         
    else         
        $timestamp = time() - $interval + (0.05 * $interval);      
}
  
if (isset($_GET['subnet']) && $_GET['subnet'] != "none")   
    $subnet = sanitize_ip($_GET['subnet']);  
if (isset($_GET['limit']) && $_GET['limit'] == "all")   
    $limit = "all";  
elseif (isset($_GET['limit']) && $_GET['limit'] != "none")   
    $limit = filter_var($_GET['limit'], FILTER_SANITIZE_NUMBER_INT);  
else   
    $limit = 20;  
  
$db = ConnectDb();  
?>

<form class="row g-3 justify-content-center mb-4" method="get" action="<?= $_SERVER['PHP_SELF'] ?>">  
<input type="hidden" name="start_time" id="hiddenStartTime" value="<?=isset($_GET['start_time'])?$_GET['start_time']:''?>">  
<input type="hidden" name="end_time" id="hiddenEndTime" value="<?=isset($_GET['end_time'])?$_GET['end_time']:''?>">  
<div class="col-md-2">  
<select class="form-select" name="sensor_id">  
<option value="none">-- 选择设备节点 --  
<?php  
$sql = "SELECT sensor_id,sensor_name FROM sensors ORDER BY sensor_name;";  
$result = pg_query($db, $sql); 
while ($r = pg_fetch_array($result)) {  
    echo "<option value='".$r['sensor_id']."' ".(isset($sensor_id)&&$sensor_id==$r['sensor_id']?"selected":"").">".$r['sensor_name']."</option>";  
}  
?>  
</select>  
</div>  
<div class="col-md-2">    
<select class="form-select" name="interval" id="intervalSelect">    
<option value="none">-- 时间间隔 --    
<option value=<?=INT_DAILY?> <?=isset($interval)&&$interval==INT_DAILY?"selected":""?>>每日</option>    
<option value=<?=INT_WEEKLY?> <?=isset($interval)&&$interval==INT_WEEKLY?"selected":""?>>每周</option>    
<option value=<?=INT_MONTHLY?> <?=isset($interval)&&$interval==INT_MONTHLY?"selected":""?>>每月</option>    
<option value=<?=INT_YEARLY?> <?=isset($interval)&&$interval==INT_YEARLY?"selected":""?>>每年</option>    
<option value=<?=24*60*60?> <?=isset($interval)&&$interval==24*60*60?"selected":""?>>最近24小时</option>    
<option value=<?=30*24*60*60?> <?=isset($interval)&&$interval==30*24*60*60?"selected":""?>>最近30天</option>    
<option value="custom" id="customOption"       
    <?=isset($_GET['interval'])&&$_GET['interval']=='custom'?"selected":""?>>    
    <?php      
    if (isset($_GET['start_time']) && isset($_GET['end_time'])) {  
        // 确保 strtotime 能正确解析 datetime-local 格式  
        $start_timestamp = strtotime($_GET['start_time']);  
        $end_timestamp = strtotime($_GET['end_time']);  
        // 验证时间戳是否有效  
        if ($start_timestamp !== false && $end_timestamp !== false && $start_timestamp > 0 && $end_timestamp > 0) {  
            // 使用正确的格式字符串,确保包含时分  
            $start_display = date('Y年n月j日H:i', $start_timestamp);      
            $end_display = date('Y年n月j日H:i', $end_timestamp);      
            echo "$start_display-$end_display";  
        } else {  
            error_log("Invalid timestamp conversion");  
            echo "自定义时间段";  
        }  
    } else {      
        echo "自定义时间段";      
    }      
    ?>      
</option>
</select>    
</div>  

<div class="col-md-2">  
<select class="form-select" name="limit">  
<option value="none">-- 显示条数 -- 
<option value="5" <?=isset($limit)&&$limit==5?"selected":""?>>5</option>
<option value="10" <?=isset($limit)&&$limit==10?"selected":""?>>10</option>
<option value="20" <?=isset($limit)&&$limit==20?"selected":""?>>20</option>  
<option value="50" <?=isset($limit)&&$limit==50?"selected":""?>>50</option> 
<option value="75" <?=isset($limit)&&$limit==75?"selected":""?>>75</option>
<option value="100" <?=isset($limit)&&$limit==100?"selected":""?>>100</option>
<option value="150" <?=isset($limit)&&$limit==150?"selected":""?>>150</option>
<option value="200" <?=isset($limit)&&$limit==200?"selected":""?>>200</option>
<option value="all" <?=isset($limit)&&$limit=="all"?"selected":""?>>全部</option>  
</select>  
</div>  
<div class="col-md-3">  
<input id="subnet" class="form-control" name="subnet" value="<?=isset($subnet)?$subnet:"192.168.1.0/2"?>">  
</div>  
<div class="col-md-2">  
<button class="btn btn-success w-100" type="submit">开始分析</button>  
</div>  
</form>  
<!-- 自定义时间段模态框 -->    
<div class="modal fade" id="customTimeModal" tabindex="-1" aria-labelledby="customTimeModalLabel" aria-hidden="true">    
  <div class="modal-dialog modal-dialog-centered">    
    <div class="modal-content">    
      <div class="modal-header">    
        <h5 class="modal-title" id="customTimeModalLabel">选择自定义时间段</h5>    
        <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>    
      </div>    
      <div class="modal-body">  
        <div class="mb-3">    
          <label for="startTime" class="form-label">起始时间</label>    
          <input type="datetime-local" class="form-control" id="startTime"  
                 value="<?=isset($_GET['start_time'])?$_GET['start_time']:date('Y-m-d\TH:i', time()-3600)?>" required>    
        </div>    
        <div class="mb-3">    
          <label for="endTime" class="form-label">结束时间</label>    
          <input type="datetime-local" class="form-control" id="endTime"  
                 value="<?=isset($_GET['end_time'])?$_GET['end_time']:date('Y-m-d\TH:i')?>" required>    
        </div>  
      </div>    
      <div class="modal-footer">    
        <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">取消</button>    
        <button type="button" class="btn btn-success" id="confirmCustomTime">确定</button>    
      </div>    
    </div>    
  </div>    
</div>

<?php
if (!isset($sensor_id)) exit(0);
$sql = "SELECT sensor_name FROM sensors WHERE sensor_id = $sensor_id;";
$result = pg_query($db, $sql); 
$sensor_name = pg_fetch_row($result)[0];
if (!isset($sensor_name)) exit(0);
echo "<h4 class='text-center text-secondary mb-4'>当前设备：$sensor_name</h4>";

if (isset($subnet)) $sql_subnet = "and ip <<= '$subnet'";
$sql = "select tx.ip, rx.scale as rxscale, tx.scale as txscale, tx.total+rx.total as total, tx.total as sent,   
rx.total as received, tx.tcp+rx.tcp as tcp, tx.udp+rx.udp as udp,  
tx.icmp+rx.icmp as icmp, tx.http+rx.http as http,  
tx.p2p+rx.p2p as p2p, tx.ftp+rx.ftp as ftp  
from  
  
(SELECT ip, max(total/sample_duration)*8 as scale, sum(total) as total, sum(tcp) as tcp, sum(udp) as udp, sum(icmp) as icmp,  
sum(http) as http, sum(p2p) as p2p, sum(ftp) as ftp  
from sensors, bd_tx_log  
where sensor_name = '$sensor_name'  
and sensors.sensor_id = bd_tx_log.sensor_id  
$sql_subnet  
and timestamp > to_timestamp($timestamp) and timestamp < to_timestamp(".($timestamp+$interval).")  
group by ip) as tx,  
  
(SELECT ip, max(total/sample_duration)*8 as scale, sum(total) as total, sum(tcp) as tcp, sum(udp) as udp, sum(icmp) as icmp,  
sum(http) as http, sum(p2p) as p2p, sum(ftp) as ftp  
from sensors, bd_rx_log  
where sensor_name = '$sensor_name'  
and sensors.sensor_id = bd_rx_log.sensor_id  
$sql_subnet  
and timestamp > to_timestamp($timestamp) and timestamp < to_timestamp(".($timestamp+$interval).")  
group by ip) as rx  
  
where tx.ip = rx.ip  
order by total desc;";
pg_query($db, "SET sort_mem TO 30000;");
$result = pg_query($db, $sql); 
pg_query($db, "set sort_mem to default;");
if ($limit == "all") $limit = pg_num_rows($result);
?>

<div class="table-responsive mb-5">
<table class="table table-hover table-bordered table-striped align-middle">
<thead class="table-primary text-center">
<tr>
<th onclick="sortTable(0)">IP 地址</th>
<th onclick="sortTable(1)">主机名</th>
<th onclick="sortTable(2)">总流量</th>
<th onclick="sortTable(3)">上传</th>
<th onclick="sortTable(4)">下载</th>
<th onclick="sortTable(5)">TCP</th>
<th onclick="sortTable(6)">UDP</th>
<th onclick="sortTable(7)">ICMP</th>
<th onclick="sortTable(8)">HTTP</th>
<th onclick="sortTable(9)">P2P</th>
<th onclick="sortTable(10)">FTP</th>
</tr>
</thead>
<tbody>
<?php
if (!isset($subnet)) $subnet="0.0.0.0/0";
echo "<tr class='table-info'><td><a href='#Total'>汇总</a></td><td>$subnet</td>";
foreach(["total","sent","received","tcp","udp","icmp","http","p2p","ftp"] as $key){
    for($i=0,$sum=0;$i<pg_num_rows($result);$i++){
        $r=pg_fetch_array($result,$i);
        $sum+=$r[$key];
    } echo fmtb($sum);
} echo "</tr>";

for($i=0;$i<pg_num_rows($result)&&$i<$limit;$i++){
$r=pg_fetch_array($result,$i);
$hostname=gethostbyaddr($r['ip']);
echo "<tr><td><a href='#".$r['ip']."'>".$r['ip']."</a></td><td>".$hostname."</td>".
fmtb($r['total']).fmtb($r['sent']).fmtb($r['received']).fmtb($r['tcp']).fmtb($r['udp']).fmtb($r['icmp']).fmtb($r['http']).fmtb($r['p2p']).fmtb($r['ftp'])."</tr>";
}
echo "</tbody></table></div>";

if ($subnet=="0.0.0.0/0"){ $total_table="bd_tx_total_log"; $total_table2="bd_rx_total_log"; }
else { $total_table="bd_tx_log"; $total_table2="bd_rx_log"; }

echo "<div class='panel' id='Total'>

<div class='panel-heading'><h5 class='panel-title'><a href='details.php?sensor_id=$sensor_id&amp;ip=$subnet'>汇总图表 - $subnet</a></h5></div>
<div class='panel-body'>
<div class='well text-center'><b>上传流量</b><br>
<img class='graph-img' src='graph.php?ip=$subnet&amp;interval=$interval&amp;timestamp=$timestamp&amp;sensor_id=$sensor_id&amp;table=$total_table' alt='上传图表'><br>
<img src='legend.gif' alt='图例' class='img-fluid'></div>
<div class='well text-center'><b>下载流量</b><br>
<img class='graph-img' src='graph.php?ip=$subnet&amp;interval=$interval&amp;timestamp=$timestamp&amp;sensor_id=$sensor_id&amp;table=$total_table2' alt='下载图表'><br>
<img src='legend.gif' alt='图例' class='img-fluid'></div>
</div></div>";

for($i=0;$i<pg_num_rows($result)&&$i<$limit;$i++){
$r=pg_fetch_array($result,$i);
$ip=$r['ip'];
$hostname=gethostbyaddr($ip);
$yscale=max($r['txscale'],$r['rxscale']);
echo "<div class='panel'> <div class='panel-heading'><h5 id='$ip' class='panel-title'><a href='details.php?sensor_id=$sensor_id&amp;ip=$ip'>节点:$ip ($hostname)</a></h5></div> <div class='panel-body'> <div class='well text-center'><b>上传流量</b><br> <img class='graph-img' src='graph.php?ip=$ip&amp;interval=$interval&amp;timestamp=$timestamp&amp;sensor_id=$sensor_id&amp;table=bd_tx_log&amp;yscale=$yscale' alt='上传图表'><br> <img src='legend.gif' alt='图例' class='img-fluid'></div> <div class='well text-center'><b>下载流量</b><br> <img class='graph-img' src='graph.php?ip=$ip&amp;interval=$interval&amp;timestamp=$timestamp&amp;sensor_id=$sensor_id&amp;table=bd_rx_log&amp;yscale=$yscale' alt='下载图表'><br> <img src='legend.gif' alt='图例' class='img-fluid'></div> </div></div>";
}
include("footer.php");
?>

</div>
<button id="backTop" onclick="backToTop()">↑</button>
</body>
</html>
