</div>

<!-- 页脚开始 -->

<footer class="footer text-center mt-5 mb-3" style="color:#999;">
    <hr style="border-color:#ccc;">
    <p class="fade-in" style="font-size:14px;">
        页面加载完成，用时 
        <span id="loadTime" class="text-success" style="font-weight:bold;">
            <?= (time() - $starttime) ?>
        </span> 秒
    </p>
    <p class="fade-in" style="font-size:13px;">© <?=date('Y')?> BandWidthd - 网络流量监控系统 </p>
</footer>

<!-- 页面动画与交互效果 -->

<script>
$(document).ready(function(){
    // 页脚淡入动画
    $(".fade-in").css("opacity",0).each(function(index){
        $(this).delay(200*index).animate({opacity:1},600);
    });

    // 提示加载完成动画
    const loadTime = $("#loadTime");
    loadTime.css({"color":"#28a745","font-size":"15px"})
        .animate({fontSize:"18px"},400)
        .delay(300)
        .animate({fontSize:"14px"},400);
});
</script>

<!-- 可选：Bootstrap 动画库 AOS -->

<!-- <link href="https://unpkg.com/aos@2.3.4/dist/aos.css" rel="stylesheet"> -->
<!-- <script src="https://unpkg.com/aos@2.3.4/dist/aos.js"></script> -->
<link href="css/aos.css" rel="stylesheet">
<script src="js/aos.js"></script>
<script>
  AOS.init({ duration: 800, once: true });
</script>

</body>
</html>
