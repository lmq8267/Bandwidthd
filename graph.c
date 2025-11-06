#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <time.h>
#include "bandwidthd.h"

#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif

extern unsigned int SubnetCount;
extern struct config config;

jmp_buf dnsjump;

static void rdnslngjmp(int signal);

void rdns(char *Buffer, unsigned long IP)  // This takes over sigalarm!
	{
#ifdef HAVE_RESOLV_H
	char DNSError[] = "DNS 超时：纠正以加快绘图速度";
	char None[] = "将 DNS 配置为对该 IP 地址进行反向解析";
	char TooManyDNSTimeouts[] = "DNS 超时次数过多，已暂停反向解析";
	struct hostent *hostent;
	char chrIP[50];
	static int Init = TRUE;
	static int DNSTimeouts = 0;  // This is reset for each run because we're forked
	unsigned long addr = htonl(IP);

    _res.retrans = 1;
    _res.retry = 2;

	if (Init)
		{
        signal(SIGALRM, rdnslngjmp);
		Init = FALSE;
		}

	if (DNSTimeouts > 100)
		{
		syslog(LOG_ERR, "DNS 超时过多，已暂停反向解析");
        strncpy(Buffer, TooManyDNSTimeouts, 253);
		Buffer[254] = '\0';
		return;
		}		

	if (setjmp(dnsjump) == 0)
		{
		alarm(10);  // Don't let gethostbyaddr hold us up too long
		hostent = gethostbyaddr((char *) &addr, 4, AF_INET); // (char *)&Data->IP				
		alarm(0);
		
		if (hostent)
			sprintf(Buffer, "%s", hostent->h_name);
		else
			{
	        strncpy(Buffer, None, 253);
			Buffer[254] = '\0';
			}
		}
	else  // Our alarm timed out
		{
		HostIp2CharIp(IP, chrIP);
		syslog(LOG_ERR, "%s DNS 超时：此问题会降低图形性能", chrIP);
		DNSTimeouts++;
        strncpy(Buffer, DNSError, 253);
		Buffer[254] = '\0';
		}
#else
	Buffer[0] = '\0';
#endif
	}

static void rdnslngjmp(int signal)
	{
    longjmp(dnsjump, 1);
	}

void swap(struct SummaryData **a, struct SummaryData **b) {
	struct SummaryData *temp;
    temp = *a; *a = *b; *b = temp;
}
void QuickSortSummaryData(struct SummaryData *SummaryData[], int left, int right) {
    int i,j,center;
    unsigned long long pivot;
    if (left==right) return;
    if (left+1==right) {
        if (SummaryData[left]->Total < SummaryData[right]->Total)
            swap(&SummaryData[left],&SummaryData[right]);
        return;
    }
    /* use the median-of-three method for picking pivot */
    center = (left+right)/2;
    if (SummaryData[left]->Total < SummaryData[center]->Total)
        swap(&SummaryData[left],&SummaryData[center]);
    if (SummaryData[left]->Total < SummaryData[right]->Total)
        swap(&SummaryData[left],&SummaryData[right]);
    if (SummaryData[center]->Total < SummaryData[right]->Total)
        swap(&SummaryData[center],&SummaryData[right]);
    pivot = SummaryData[center]->Total;
    swap(&SummaryData[center],&SummaryData[right-1]); /* hide the pivot */
    i = left; j = right - 1;
    do {
        do { ++i; } while (SummaryData[i]->Total > pivot);
        do { --j; } while (SummaryData[j]->Total < pivot);
        swap(&SummaryData[i],&SummaryData[j]);
    } while (j > i);
    swap(&SummaryData[i],&SummaryData[j]); /* undo last swap */
    swap(&SummaryData[i],&SummaryData[right-1]); /* restore pivot */
    QuickSortSummaryData(SummaryData,left,i-1);
    QuickSortSummaryData(SummaryData,i+1,right);
}

#define NumFactor 1024
static void FormatNum(unsigned long long n, char *buf, int len) {
    double f;
    if (n<NumFactor) { snprintf(buf,len,"<td align=\"center\"><tt>%i&nbsp;</tt></td>",(int)n); return; }
    f = n;
    f /= NumFactor; if (f<NumFactor) { snprintf(buf,len,"<td align=\"center\"><tt>%.1fK</tt></td>",f); return; }
    f /= NumFactor; if (f<NumFactor) { snprintf(buf,len,"<td align=\"center\"><tt>%.1fM</tt></td>",f); return; }
    f /= NumFactor; if (f<NumFactor) { snprintf(buf,len,"<td align=\"center\"><tt>%.1fG</tt></td>",f); return; }
    f /= NumFactor; snprintf(buf,len,"<td align=\"center\"><tt>%.1fT</tt></td>\n",f);
}

void PrintTableLine(FILE *stream, struct SummaryData *Data, int Counter)
	{
	char Buffer1[50];
	char Buffer2[50];
	char Buffer3[50];
	char Buffer4[50];
	char Buffer4b[50];
	char Buffer5[50];
	char Buffer5b[50];
	char Buffer6[50];
	char Buffer7[50];
	char Buffer8[50];
	char DisplayText[50];

	// First convert the info to nice, human readable stuff
	if (Data->IP == 0) {
		strcpy(Buffer1, "Total");
		strcpy(DisplayText, "总计");
	} else {
		HostIp2CharIp(Data->IP, Buffer1);
		strcpy(DisplayText, Buffer1);
	}

    FormatNum(Data->Total,         Buffer2,  50);
	FormatNum(Data->TotalSent,     Buffer3,  50);
	FormatNum(Data->TotalReceived, Buffer4,  50);
	FormatNum(Data->FTP, 		   Buffer4b, 50);
	FormatNum(Data->HTTP,          Buffer5,  50);
	FormatNum(Data->P2P,           Buffer5b, 50);
	FormatNum(Data->TCP,           Buffer6,  50);
	FormatNum(Data->UDP,           Buffer7,  50);
	FormatNum(Data->ICMP,          Buffer8,  50);

	if (Counter%4 == 0 || (Counter-1)%4 == 0)
		fprintf(stream, "<TR>");
	else
		fprintf(stream, "<TR bgcolor=lightblue>");

	if (Data->Graph)
		fprintf(stream, "<TD align=center><a href=\"#%s-%c\">%s</a></TD>%s%s%s%s%s%s%s%s%s</TR>\n",
			Buffer1, // Ip
			config.tag,
			DisplayText, // Ip
			Buffer2, // Total
			Buffer3, // TotalSent
			Buffer4, // TotalReceived
			Buffer4b, // FTP
			Buffer5, // HTTP
			Buffer5b, // P2P
			Buffer6, // TCP
			Buffer7, // UDP
			Buffer8); // ICMP
	else
		fprintf(stream, "<TD align=center>%s</TD>%s%s%s%s%s%s%s%s%s</TR>\n",
			DisplayText, // Ip
			Buffer2, // Total
			Buffer3, // TotalSent
			Buffer4, // TotalReceived
			Buffer4b, // FTP
			Buffer5, // HTTP
			Buffer5b, // P2P		
			Buffer6, // TCP
			Buffer7, // UDP
			Buffer8); // ICMP
	}

void MakeIndexPages(int NumIps, struct SummaryData *SummaryData[])
	{
	int SubnetCounter;
	int Counter, tCounter;
	time_t WriteTime;
	char filename[] = "/tmp/Bandwidthd_html/lljk2.html";
	char *PeriodDesc;
	
	FILE *file;

	char Buffer1[50];
	char Buffer2[50];
	char HostName[255];
	char DisplayIP[50];

	WriteTime = time(NULL);
	
	QuickSortSummaryData(SummaryData, 0, NumIps-1);

	////////////////////////////////////////////////
	// Print main index page
	
	if (config.tag == '1')
		{
		if ((file = fopen("/tmp/Bandwidthd_html/lljk.html", "wt")) == NULL)
			{
			syslog(LOG_ERR, "打开 /tmp/Bandwidthd_html/lljk.html 失败");
			exit(1);
			}
		}
	else
		{
		filename[25] = config.tag;
		if ((file = fopen(filename, "wt")) == NULL)
			{
			syslog(LOG_ERR, "打开 %s 失败", filename);
			exit(1);
			}
		}

	switch (config.tag)
		{
		case '1': PeriodDesc = "日流量"; break;
		case '2': PeriodDesc = "周流量"; break;
		case '3': PeriodDesc = "月流量"; break;
		case '4': PeriodDesc = "年流量"; break;
		default: PeriodDesc = ""; break;
		}
	time_t now = WriteTime;                                 // 当前时间戳
	struct tm *tm_info = localtime(&now);                   // 转换为本地时间结构
	char time_buf[128];                                     // 用于格式化后的时间字符串
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);  // 格式化时间

	const char *weekdays_cn[] = {"日", "一", "二", "三", "四", "五", "六"};  // 中文星期
	int weekday = tm_info->tm_wday;                         // 获取星期几
		
	fprintf(file, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
	fprintf(file, "<HTML>\n<HEAD>\n<TITLE>Bandwidthd</TITLE>\n");
	fprintf(file, "<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=UTF-8\">\n");
	
	if (config.meta_refresh)
		fprintf(file, "<META HTTP-EQUIV=\"REFRESH\" content=\"%u\">\n",
				config.meta_refresh);
	fprintf(file, "<META HTTP-EQUIV=\"EXPIRES\" content=\"-1\">\n");
	fprintf(file, "<META HTTP-EQUIV=\"PRAGMA\" content=\"no-cache\">\n");
	fprintf(file, "</HEAD>\n<BODY vlink=blue>\n");
	fprintf(file, "<center><img src=\"%s\" ALT=\"Logo\"><BR>\n", logo_base64);
	//fprintf(file, "由 David Hinkle 编程，受 <a href=\"http://www.derbytech.com\">DerbyTech</a> 无线网络公司委托开发<BR>");
	fprintf(file, "<script>\n");
	fprintf(file, "document.addEventListener('DOMContentLoaded',function(){\n");
	fprintf(file, "document.querySelectorAll('a').forEach(function(a){\n");
	fprintf(file, "a.style.transition='all 0.3s ease';\n");
	fprintf(file, "a.addEventListener('mouseenter',function(){\n");
	fprintf(file, "a.style.transform='scale(1.05)';");
	fprintf(file, "a.style.boxShadow='0 4px 10px rgba(0,0,0,0.2)';");
	fprintf(file, "a.style.filter='brightness(90%)';");
	fprintf(file, "});");
	fprintf(file, "a.addEventListener('mouseleave',function(){");
	fprintf(file, "a.style.transform='scale(1)';");
	fprintf(file, "a.style.boxShadow='0 2px 5px rgba(0,0,0,0.15)';");
	fprintf(file, "a.style.filter='brightness(100%)';");
	fprintf(file, "});");
	fprintf(file, "});");
	fprintf(file, "});");
	fprintf(file, "var sortDirection={};\n");  
	fprintf(file, "function sortTable(col){\n");  
	fprintf(file, "var table=document.querySelector('table');\n");  
	fprintf(file, "var rows=Array.from(table.querySelectorAll('tr')).slice(1);\n");  
	fprintf(file, "var asc=sortDirection[col]!==true;\n");  
	fprintf(file, "sortDirection[col]=asc;\n");  
	fprintf(file, "rows.sort(function(a,b){\n");  
	fprintf(file, "var aText=a.cells[col].textContent.trim();\n");  
	fprintf(file, "var bText=b.cells[col].textContent.trim();\n");  
	fprintf(file, "if(col===0){\n");  
	fprintf(file, "var aIP=aText.replace(/<[^>]*>/g,'').split('.').map(function(n){return('000'+n).slice(-3);}).join('.');\n");  
	fprintf(file, "var bIP=bText.replace(/<[^>]*>/g,'').split('.').map(function(n){return('000'+n).slice(-3);}).join('.');\n");  
	fprintf(file, "return asc?aIP.localeCompare(bIP):bIP.localeCompare(aIP);\n");  
	fprintf(file, "}\n");  
	fprintf(file, "var aVal=parseValue(aText);\n");  
	fprintf(file, "var bVal=parseValue(bText);\n");  
	fprintf(file, "return asc?aVal-bVal:bVal-aVal;\n");  
	fprintf(file, "});\n");  
	fprintf(file, "rows.forEach(function(row,idx){\n");  
	fprintf(file, "if(idx%%4===0||idx%%4===1){row.removeAttribute('bgcolor');}else{row.setAttribute('bgcolor','lightblue');}\n");  
	fprintf(file, "table.appendChild(row);\n");  
	fprintf(file, "});\n");  
	fprintf(file, "}\n");  
	fprintf(file, "function parseValue(text){\n");  
	fprintf(file, "var match=text.match(/([0-9.]+)([KMGT]?)/);\n");  
	fprintf(file, "if(!match)return 0;\n");  
	fprintf(file, "var num=parseFloat(match[1]);\n");  
	fprintf(file, "var unit=match[2];\n");  
	fprintf(file, "var mult={'K':1024,'M':1048576,'G':1073741824,'T':1099511627776};\n");  
	fprintf(file, "return num*(mult[unit]||1);\n");  
	fprintf(file, "}\n");
	fprintf(file, "</script>\n");
	fprintf(file, "更新时间： %s 星期%s<br>\n", time_buf, weekdays_cn[weekday]);
	// 添加下次更新时间  
	time_t next_update = now + config.interval;  
	struct tm *next_tm = localtime(&next_update);  
	char next_time_buf[128];  
	strftime(next_time_buf, sizeof(next_time_buf), "%Y-%m-%d %H:%M:%S", next_tm);  
	fprintf(file, "下次更新： %s 星期%s (间隔%d秒)<br>\n", next_time_buf, weekdays_cn[next_tm->tm_wday], config.interval);
	fprintf(file, "<BR>\n <a href=\"lljk.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#007BFF;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">日流量</a> \n ");
	fprintf(file, " <a href=\"lljk2.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#28A745;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">周流量</a> \n ");
	fprintf(file, " <a href=\"lljk3.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#FFC107;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">月流量</a> \n ");
	fprintf(file, " <a href=\"lljk4.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#DC3545;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">年流量</a><BR>\n");

	fprintf(file, "<BR>\n选择一个子网地址：<BR>\n");	
	if (config.tag == '1')
		fprintf(file, "- <a href=\"lljk.html\">排名前20</a> -");
	else
		fprintf(file, "- <a href=\"lljk%c.html\">排名前20</a> -", config.tag);

	for (Counter = 0; Counter < SubnetCount; Counter++)            
		{
		HostIp2CharIp(SubnetTable[Counter].ip, Buffer1);
		fprintf(file, "- <a href=\"%c-%s.html\">%s</a> -", config.tag, Buffer1, Buffer1);
		}

	/////  TOP 20

	fprintf(file, "<H1>排名前20的IP地址流量统计表 - %s</H1></center>", PeriodDesc);
	fprintf(file, "<center>\n<table width=\"100%%\" border=1 cellspacing=0>\n");

    // PASS 1:  Write out the table

	fprintf(file, "<TR bgcolor=lightblue><TD align=center onclick=\"sortTable(0)\" style=\"cursor:pointer;color:blue;border-color:black\">IP地址</TD><TD align=center onclick=\"sortTable(1)\" style=\"cursor:pointer;color:blue;border-color:black\">总计</TD><TD align=center onclick=\"sortTable(2)\" style=\"cursor:pointer;color:blue;border-color:black\">上传总量</TD><TD align=center onclick=\"sortTable(3)\" style=\"cursor:pointer;color:blue;border-color:black\">下载总量</TD><TD align=center onclick=\"sortTable(4)\" style=\"cursor:pointer;color:blue;border-color:black\">FTP流量</TD><TD align=center onclick=\"sortTable(5)\" style=\"cursor:pointer;color:blue;border-color:black\">HTTP流量</TD><TD align=center onclick=\"sortTable(6)\" style=\"cursor:pointer;color:blue;border-color:black\">SMTP流量</TD><TD align=center onclick=\"sortTable(7)\" style=\"cursor:pointer;color:blue;border-color:black\">TCP流量</TD><TD align=center onclick=\"sortTable(8)\" style=\"cursor:pointer;color:blue;border-color:black\">UDP流量</TD><TD align=center onclick=\"sortTable(9)\" style=\"cursor:pointer;color:blue;border-color:black\">ICMP流量</TD>\n");
	for (Counter=0; Counter < 21 && Counter < NumIps; Counter++)
		PrintTableLine(file, SummaryData[Counter], Counter);

	fprintf(file, "</table></center>\n");

	// PASS 2: The graphs
	for (Counter=0; Counter < 21 && Counter < NumIps; Counter++)
		if (SummaryData[Counter]->Graph)
			{
			if (SummaryData[Counter]->IP == 0)
				{
				strcpy(Buffer1, "Total");	
				strcpy(HostName, "总计");
				strcpy(DisplayIP, "流量"); 
				}
			else
				{	
				HostIp2CharIp(SummaryData[Counter]->IP, Buffer1);
				rdns(HostName, SummaryData[Counter]->IP);
				strcpy(DisplayIP, Buffer1);
				}
			fprintf(file,
    				"<a name=\"%s-%c\"></a>"
    				"<div style=\"background-color:#f0f8ff; border:1px solid #ccc; padding:15px; margin:20px 0;\">\n"
	    			"<h5 style=\"margin:0 0 10px 0;\">\n"
	    			"<a href=\"#top\" "
    				"style=\"display:inline-block; background:#007bff; color:white; padding:5px 10px; text-decoration:none; border-radius:5px;\">返回顶部</a>\n"
    				"</h5>\n"
    				"<h1 style=\"margin-top:0;\">%s - %s</h1>\n"
    				"<strong>上传流量:</strong><br>\n"
    				"<img src=\"%s-%c-S.png\" alt=\"上传流量数据图 %s\"><br>\n"
    				"<img src=\"%s\" alt=\"图例\"><br>\n"
    				"<strong>下载流量:</strong><br>\n"
    				"<img src=\"%s-%c-R.png\" alt=\"下载流量数据图 %s\"><br>\n"
    				"<img src=\"%s\" alt=\"图例\">\n"
    				"</div>\n",
    				Buffer1, config.tag,              // 锚点
    				DisplayIP, HostName,                // 标题中的 IP 和主机名
    				Buffer1, config.tag, Buffer1,     // 发送图像路径和 ALT
    				legend_base64,                    // 图例图片
    				Buffer1, config.tag, Buffer1,     // 接收图像路径和 ALT
    				legend_base64                     // 图例图片
				);
			}

	fprintf(file, "</BODY></HTML>\n");

	const char *period_desc;  
	switch (config.tag) {  
    	case '1': period_desc = "每日统计"; break;  
    	case '2': period_desc = "每周统计"; break;  
    	case '3': period_desc = "每月统计"; break;  
    	case '4': period_desc = "每年统计"; break;  
    	default: period_desc = "未知周期"; break;  
	}  
  
	// syslog(LOG_INFO, "[%s] 前端页面更新成功: %s, 包含 %d 个IP", period_desc, config.tag == '1' ? "/tmp/Bandwidthd_html/lljk.html" : filename, NumIps);
		
	fclose(file);

	////////////////////////////////////////////////
	// Print each subnet page

	for (SubnetCounter = 0; SubnetCounter < SubnetCount; SubnetCounter++)
		{
		HostIp2CharIp(SubnetTable[SubnetCounter].ip, Buffer1);
		sprintf(Buffer2, "/tmp/Bandwidthd_html/%c-%s.html", config.tag, Buffer1);
		file = fopen(Buffer2, "wt");
		fprintf(file, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
		fprintf(file, "<HTML>\n<HEAD><TITLE>Bandwidthd - 子网 %s</TITLE>\n", Buffer1);
		fprintf(file, "<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=UTF-8\">\n");
		if (config.meta_refresh)
			fprintf(file, "<META HTTP-EQUIV=\"REFRESH\" content=\"%u\">\n",
					config.meta_refresh);
		fprintf(file, "<META HTTP-EQUIV=\"EXPIRES\" content=\"-1\">\n");
		fprintf(file, "<META HTTP-EQUIV=\"PRAGMA\" content=\"no-cache\">\n");
		fprintf(file, "</HEAD>\n<BODY vlink=blue>\n");
		fprintf(file, "<center><img src=\"%s\" ALT=\"Logo\"><BR>\n", logo_base64);
		//fprintf(file, "由 David Hinkle 编程，受 <a href=\"http://www.derbytech.com\">DerbyTech</a> 无线网络公司委托开发<BR>\n");

		fprintf(file, "<script>\n");
		fprintf(file, "document.addEventListener('DOMContentLoaded',function(){\n");
		fprintf(file, "document.querySelectorAll('a').forEach(function(a){\n");
		fprintf(file, "a.style.transition='all 0.3s ease';\n");
		fprintf(file, "a.addEventListener('mouseenter',function(){\n");
		fprintf(file, "a.style.transform='scale(1.05)';");
		fprintf(file, "a.style.boxShadow='0 4px 10px rgba(0,0,0,0.2)';");
		fprintf(file, "a.style.filter='brightness(90%)';");
		fprintf(file, "});");
		fprintf(file, "a.addEventListener('mouseleave',function(){");
		fprintf(file, "a.style.transform='scale(1)';");
		fprintf(file, "a.style.boxShadow='0 2px 5px rgba(0,0,0,0.15)';");
		fprintf(file, "a.style.filter='brightness(100%)';");
		fprintf(file, "});");
		fprintf(file, "});");
		fprintf(file, "});");
		fprintf(file, "var sortDirection={};\n");  
		fprintf(file, "function sortTable(col){\n");  
		fprintf(file, "var table=document.querySelector('table');\n");  
		fprintf(file, "var rows=Array.from(table.querySelectorAll('tr')).slice(1);\n");  
		fprintf(file, "var asc=sortDirection[col]!==true;\n");  
		fprintf(file, "sortDirection[col]=asc;\n");  
		fprintf(file, "rows.sort(function(a,b){\n");  
		fprintf(file, "var aText=a.cells[col].textContent.trim();\n");  
		fprintf(file, "var bText=b.cells[col].textContent.trim();\n");  
		fprintf(file, "if(col===0){\n");  
		fprintf(file, "var aIP=aText.replace(/<[^>]*>/g,'').split('.').map(function(n){return('000'+n).slice(-3);}).join('.');\n");  
		fprintf(file, "var bIP=bText.replace(/<[^>]*>/g,'').split('.').map(function(n){return('000'+n).slice(-3);}).join('.');\n");  
		fprintf(file, "return asc?aIP.localeCompare(bIP):bIP.localeCompare(aIP);\n");  
		fprintf(file, "}\n");  
		fprintf(file, "var aVal=parseValue(aText);\n");  
		fprintf(file, "var bVal=parseValue(bText);\n");  
		fprintf(file, "return asc?aVal-bVal:bVal-aVal;\n");  
		fprintf(file, "});\n");  
		fprintf(file, "rows.forEach(function(row,idx){\n");  
		fprintf(file, "if(idx%%4===0||idx%%4===1){row.removeAttribute('bgcolor');}else{row.setAttribute('bgcolor','lightblue');}\n");  
		fprintf(file, "table.appendChild(row);\n");  
		fprintf(file, "});\n");  
		fprintf(file, "}\n");  
		fprintf(file, "function parseValue(text){\n");  
		fprintf(file, "var match=text.match(/([0-9.]+)([KMGT]?)/);\n");  
		fprintf(file, "if(!match)return 0;\n");  
		fprintf(file, "var num=parseFloat(match[1]);\n");  
		fprintf(file, "var unit=match[2];\n");  
		fprintf(file, "var mult={'K':1024,'M':1048576,'G':1073741824,'T':1099511627776};\n");  
		fprintf(file, "return num*(mult[unit]||1);\n");  
		fprintf(file, "}\n");
		fprintf(file, "</script>\n");
		fprintf(file, "更新时间： %s 星期%s<br>\n", time_buf, weekdays_cn[weekday]);
		// 添加下次更新时间  
		time_t next_update = now + config.interval;  
		struct tm *next_tm = localtime(&next_update);  
		char next_time_buf[128];  
		strftime(next_time_buf, sizeof(next_time_buf), "%Y-%m-%d %H:%M:%S", next_tm);  
		fprintf(file, "下次更新： %s 星期%s (间隔%d秒)<br>\n", next_time_buf, weekdays_cn[next_tm->tm_wday], config.interval);
		fprintf(file, "<BR>\n <a href=\"lljk.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#007BFF;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">日流量</a> \n ");
		fprintf(file, " <a href=\"lljk2.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#28A745;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">周流量</a> \n ");
		fprintf(file, " <a href=\"lljk3.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#FFC107;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">月流量</a> \n ");
		fprintf(file, " <a href=\"lljk4.html\" style=\"display:inline-block;margin:6px;padding:10px 20px;font-size:15px;background:#DC3545;color:#fff;border-radius:8px;text-decoration:none;box-shadow:0 2px 5px rgba(0,0,0,0.15);transition:background 0.3s ease;\">年流量</a><BR>\n");

		fprintf(file, "<BR>\n选择一个子网地址：<BR>\n");
		if (config.tag == '1')
			fprintf(file, "- <a href=\"lljk.html\">排名前20</a> -");
		else
			fprintf(file, "- <a href=\"lljk%c.html\">排名前20</a> -", config.tag);

		for (Counter = 0; Counter < SubnetCount; Counter++)
			{
			HostIp2CharIp(SubnetTable[Counter].ip, Buffer2);
			fprintf(file, "- <a href=\"%c-%s.html\">%s</a> -", config.tag, Buffer2, Buffer2);
			}

		fprintf(file, "<H1>%s - %s</H1></center>", Buffer1, PeriodDesc);
		fprintf(file, "<table width=\"100%%\" border=1 cellspacing=0>\n");

        // PASS 1:  Write out the table

		fprintf(file, "<TR bgcolor=lightblue><TD align=center onclick=\"sortTable(0)\" style=\"cursor:pointer;color:blue;border-color:black\">IP地址</TD><TD align=center onclick=\"sortTable(1)\" style=\"cursor:pointer;color:blue;border-color:black\">总计</TD><TD align=center onclick=\"sortTable(2)\" style=\"cursor:pointer;color:blue;border-color:black\">上传总量</TD><TD align=center onclick=\"sortTable(3)\" style=\"cursor:pointer;color:blue;border-color:black\">下载总量</TD><TD align=center onclick=\"sortTable(4)\" style=\"cursor:pointer;color:blue;border-color:black\">FTP流量</TD><TD align=center onclick=\"sortTable(5)\" style=\"cursor:pointer;color:blue;border-color:black;border-color:black\">HTTP流量</TD><TD align=center onclick=\"sortTable(6)\" style=\"cursor:pointer;color:blue;border-color:black\">SMTP流量</TD><TD align=center onclick=\"sortTable(7)\" style=\"cursor:pointer;color:blue;border-color:black\">TCP流量</TD><TD align=center onclick=\"sortTable(8)\" style=\"cursor:pointer;color:blue;border-color:black\">UDP流量</TD><TD align=center onclick=\"sortTable(9)\" style=\"cursor:pointer;color:blue;border-color:black\">ICMP流量</TD>\n");
		for (tCounter=0, Counter=0; Counter < NumIps; Counter++)
			{
            if (SubnetTable[SubnetCounter].ip == (SummaryData[Counter]->IP & SubnetTable[SubnetCounter].mask))
				{ // The ip belongs to this subnet
				PrintTableLine(file, SummaryData[Counter], tCounter++);
    			}
			}

		fprintf(file, "</table>\n");

		// PASS 2: The graphs
		for (Counter=0; Counter < NumIps; Counter++)
			{
            if (SubnetTable[SubnetCounter].ip == (SummaryData[Counter]->IP & SubnetTable[SubnetCounter].mask))
				{ // The ip belongs to this subnet
				if (SummaryData[Counter]->Graph)
					{
					HostIp2CharIp(SummaryData[Counter]->IP, Buffer1);
					rdns(HostName, SummaryData[Counter]->IP);
					fprintf(file,
    						"<a name=\"%s-%c\"></a>"
    						"<div style=\"background-color:#f0f8ff; border:1px solid #ccc; padding:15px; margin:20px 0;\">\n"
	    					"<h5 style=\"margin:0 0 10px 0;\">\n"
	    					"<a href=\"#top\" "
    						"style=\"display:inline-block; background:#007bff; color:white; padding:5px 10px; text-decoration:none; border-radius:5px;\">返回顶部</a>\n"
    						"</h5>\n"
    						"<h1 style=\"margin-top:0;\">%s - %s</h1>\n"
    						"<strong>上传流量:</strong><br>\n"
    						"<img src=\"%s-%c-S.png\" alt=\"上传流量数据图 %s\"><br>\n"
    						"<img src=\"%s\" alt=\"图例\"><br>\n"
    						"<strong>下载流量:</strong><br>\n"
    						"<img src=\"%s-%c-R.png\" alt=\"下载流量数据图 %s\"><br>\n"
    						"<img src=\"%s\" alt=\"图例\">\n"
    						"</div>\n",
    						Buffer1, config.tag,              // 锚点
    						DisplayIP, HostName,                // 标题中的 IP 和主机名
    						Buffer1, config.tag, Buffer1,     // 发送图像路径和 ALT
    						legend_base64,                    // 图例图片
    						Buffer1, config.tag, Buffer1,     // 接收图像路径和 ALT
    						legend_base64                     // 图例图片
						);
					}
				}
			}

		fprintf(file, "</BODY></HTML>\n");
		fclose(file);
		}

	free(SummaryData);
	}

void GraphIp(struct IPDataStore *DataStore, struct SummaryData *SummaryData, time_t timestamp)
    {
    FILE *OutputFile;
    char outputfilename[50];
    gdImagePtr im, im2;
    int white;
    unsigned long long int YMax;
	char CharIp[20];

    time_t GraphBeginTime;

	// TODO: First determine if graph will be printed before creating image and drawing backround, etc

	if (DataStore->ip == 0)
		strcpy(CharIp, "Total");
	else
		HostIp2CharIp(DataStore->ip, CharIp);

    GraphBeginTime = timestamp - config.range;

    im = gdImageCreate(XWIDTH, YHEIGHT);
    white = gdImageColorAllocate(im, 255, 255, 255);
    //gdImageFill(im, 10, 10, white);

    im2 = gdImageCreate(XWIDTH, YHEIGHT);
    white = gdImageColorAllocate(im2, 255, 255, 255);
    //gdImageFill(im2, 10, 10, white);

    YMax = GraphData(im, im2, DataStore, GraphBeginTime, SummaryData);
    if (YMax != 0)
        {
        // Finish the graph
        PrepareXAxis(im, timestamp);
        PrepareYAxis(im, YMax);

        PrepareXAxis(im2, timestamp);
        PrepareYAxis(im2, YMax);

        sprintf(outputfilename, "/tmp/Bandwidthd_html/%s-%c-S.png", CharIp, config.tag);
        OutputFile = fopen(outputfilename, "wb");    
        gdImagePng(im, OutputFile);
        fclose(OutputFile);

        sprintf(outputfilename, "/tmp/Bandwidthd_html/%s-%c-R.png", CharIp, config.tag);
        OutputFile = fopen(outputfilename, "wb");
        gdImagePng(im2, OutputFile);
        fclose(OutputFile);
        }
    else
        {
        // The graph isn't worth clutering up the web pages with
        sprintf(outputfilename, "/tmp/Bandwidthd_html/%s-%c-R.png", CharIp, config.tag);
        unlink(outputfilename);
        sprintf(outputfilename, "/tmp/Bandwidthd_html/%s-%c-S.png", CharIp, config.tag);
        unlink(outputfilename);
        }

	gdImageDestroy(im);
	gdImageDestroy(im2);
    }

// Returns YMax
unsigned long long int GraphData(gdImagePtr im, gdImagePtr im2, struct IPDataStore *DataStore, time_t timestamp, struct SummaryData *SummaryData)
    {
    unsigned long long int YMax=0;
	
	struct DataStoreBlock *CurrentBlock;
    struct IPData *Data;

	// TODO: These should be a structure!!!!
	// TODO: This is an awfull lot of data to be allocated on the stack

    unsigned long long total[XWIDTH];
    unsigned long long icmp[XWIDTH];
    unsigned long long udp[XWIDTH];
    unsigned long long tcp[XWIDTH];
	unsigned long long ftp[XWIDTH];
    unsigned long long http[XWIDTH];
    unsigned long long p2p[XWIDTH];
    int Count[XWIDTH];

    unsigned long long total2[XWIDTH];
    unsigned long long icmp2[XWIDTH];
    unsigned long long udp2[XWIDTH];
    unsigned long long tcp2[XWIDTH];
	unsigned long long ftp2[XWIDTH];
    unsigned long long http2[XWIDTH];
    unsigned long long p2p2[XWIDTH];

    size_t DataPoints;
    double x;
    int xint;
    int Counter;
    char Buffer[30];
    char Buffer2[50];
    
    int blue, lblue, red, purple, green, brown, black;
    int blue2, lblue2, red2, purple2, green2, brown2, black2;

	unsigned long long int SentPeak = 0;
	unsigned long long int ReceivedPeak = 0;

    purple   = gdImageColorAllocate(im, 255, 0, 255);
    green    = gdImageColorAllocate(im, 0, 255, 0);
    blue     = gdImageColorAllocate(im, 0, 0, 255);
	lblue	 = gdImageColorAllocate(im, 128, 128, 255);
    brown    = gdImageColorAllocate(im, 128, 0, 0);
    red      = gdImageColorAllocate(im, 255, 0, 0);
    black 	 = gdImageColorAllocate(im, 0, 0, 0);
    
    purple2   = gdImageColorAllocate(im2, 255, 0, 255);
    green2   = gdImageColorAllocate(im2, 0, 255, 0);
    blue2    = gdImageColorAllocate(im2, 0, 0, 255);
	lblue2	 = gdImageColorAllocate(im2, 128, 128, 255);
    brown2   = gdImageColorAllocate(im2, 128, 0, 0);
    red2     = gdImageColorAllocate(im2, 255, 0, 0);
    black2   = gdImageColorAllocate(im2, 0, 0, 0);

	CurrentBlock = DataStore->FirstBlock;
	Data = CurrentBlock->Data;
    DataPoints = CurrentBlock->NumEntries;

	memset(SummaryData, 0, sizeof(struct SummaryData));
	SummaryData->IP = Data[0].ip;
	
    memset(Count, 0, sizeof(Count[0])*XWIDTH);

    memset(total, 0, sizeof(total[0])*XWIDTH);
    memset(icmp, 0, sizeof(total[0])*XWIDTH);
    memset(udp, 0, sizeof(total[0])*XWIDTH);
    memset(tcp, 0, sizeof(total[0])*XWIDTH);
	memset(ftp, 0, sizeof(total[0])*XWIDTH);
    memset(http, 0, sizeof(total[0])*XWIDTH);
    memset(p2p, 0, sizeof(total[0])*XWIDTH);

    memset(total2, 0, sizeof(total[0])*XWIDTH);
    memset(icmp2, 0, sizeof(total[0])*XWIDTH);
    memset(udp2, 0, sizeof(total[0])*XWIDTH);
    memset(tcp2, 0, sizeof(total[0])*XWIDTH);
    memset(ftp2, 0, sizeof(total[0])*XWIDTH);
    memset(http2, 0, sizeof(total[0])*XWIDTH);
    memset(p2p2, 0, sizeof(total[0])*XWIDTH);

	// Change this to just run through all the datapoints we have stored in ram

	// Sum up the bytes/second
    while(DataPoints > 0)  // We have data to graph
        {
        for (Counter = 0; Counter < DataPoints; Counter++)  // Graph it all
            {
            x = (Data[Counter].timestamp-timestamp)*((XWIDTH-XOFFSET)/config.range)+XOFFSET;        
            xint = x;

            if (xint >= 0 && xint < XWIDTH)
                {
                Count[xint]++;
				
				if (Data[Counter].Send.total > SentPeak)
					SentPeak = Data[Counter].Send.total;
       	        total[xint] += Data[Counter].Send.total;
           	    icmp[xint] += Data[Counter].Send.icmp;
               	udp[xint] += Data[Counter].Send.udp;
                tcp[xint] += Data[Counter].Send.tcp;
				ftp[xint] += Data[Counter].Send.ftp;
       	        http[xint] += Data[Counter].Send.http;
				p2p[xint] += Data[Counter].Send.p2p;

                if (Data[Counter].Receive.total > ReceivedPeak)
   	            	ReceivedPeak = Data[Counter].Receive.total;
       	        total2[xint] += Data[Counter].Receive.total;
           	    icmp2[xint] += Data[Counter].Receive.icmp;
               	udp2[xint] += Data[Counter].Receive.udp;
                tcp2[xint] += Data[Counter].Receive.tcp;
				ftp2[xint] += Data[Counter].Receive.ftp;
       	        http2[xint] += Data[Counter].Receive.http;
				p2p2[xint] += Data[Counter].Receive.p2p;
                }
            }

		CurrentBlock = CurrentBlock->Next;
			
		if (CurrentBlock)
			{
         	Data = CurrentBlock->Data;
			DataPoints = CurrentBlock->NumEntries;
			}
		else
			DataPoints = 0;		
        }

	// Convert SentPeak and ReceivedPeak from bytes to bytes/second
	SentPeak /= config.interval; ReceivedPeak /= config.interval;

    // Preform the Average
    for(Counter=XOFFSET+1; Counter < XWIDTH; Counter++)
            {
            if (Count[Counter] > 0)
                {
            	SummaryData->Total += total[Counter] + total2[Counter];
				SummaryData->TotalSent += total[Counter];
 				SummaryData->TotalReceived += total2[Counter];
				SummaryData->TCP += tcp[Counter] + tcp2[Counter];
				SummaryData->FTP += ftp[Counter] + ftp2[Counter];
				SummaryData->HTTP += http[Counter] + http2[Counter];
				SummaryData->P2P += p2p[Counter] + p2p2[Counter];
				SummaryData->UDP += udp[Counter] + udp2[Counter];
				SummaryData->ICMP += icmp[Counter] + icmp2[Counter];

                // Preform the average
                total[Counter] /= (Count[Counter]*config.interval);
                tcp[Counter] /= (Count[Counter]*config.interval);
                ftp[Counter] /= (Count[Counter]*config.interval);
                http[Counter] /= (Count[Counter]*config.interval);
				p2p[Counter] /= (Count[Counter]*config.interval);
                udp[Counter] /= (Count[Counter]*config.interval);
                icmp[Counter] /= (Count[Counter]*config.interval);
								
                total2[Counter] /= (Count[Counter]*config.interval);
                tcp2[Counter] /= (Count[Counter]*config.interval);
				ftp2[Counter] /= (Count[Counter]*config.interval);
                http2[Counter] /= (Count[Counter]*config.interval);
				p2p2[Counter] /= (Count[Counter]*config.interval);
                udp2[Counter] /= (Count[Counter]*config.interval);
                icmp2[Counter] /= (Count[Counter]*config.interval);


                if (total[Counter] > YMax)
                    YMax = total[Counter];
                
                if (total2[Counter] > YMax)
                    YMax = total2[Counter];
                }
            }

    YMax += YMax*0.05;    // Add an extra 5%
	
    if ((SummaryData->IP != 0 && SummaryData->Total < config.graph_cutoff))
		{
		SummaryData->Graph = FALSE;
        return(0);
		}
	else
        SummaryData->Graph = TRUE;

    // Plot the points
    for(Counter=XOFFSET+1; Counter < XWIDTH; Counter++)    
            {
            if (Count[Counter] > 0)
                {
                // Convert the bytes/sec to y coords
                total[Counter] = (total[Counter]*(YHEIGHT-YOFFSET))/YMax;
                tcp[Counter] = (tcp[Counter]*(YHEIGHT-YOFFSET))/YMax;
                ftp[Counter] = (ftp[Counter]*(YHEIGHT-YOFFSET))/YMax;
                http[Counter] = (http[Counter]*(YHEIGHT-YOFFSET))/YMax;
                p2p[Counter] = (p2p[Counter]*(YHEIGHT-YOFFSET))/YMax;
                udp[Counter] = (udp[Counter]*(YHEIGHT-YOFFSET))/YMax;
                icmp[Counter] = (icmp[Counter]*(YHEIGHT-YOFFSET))/YMax;

                total2[Counter] = (total2[Counter]*(YHEIGHT-YOFFSET))/YMax;
                tcp2[Counter] = (tcp2[Counter]*(YHEIGHT-YOFFSET))/YMax;
                ftp2[Counter] = (ftp2[Counter]*(YHEIGHT-YOFFSET))/YMax;
                http2[Counter] = (http2[Counter]*(YHEIGHT-YOFFSET))/YMax;
				p2p2[Counter] = (p2p2[Counter]*(YHEIGHT-YOFFSET))/YMax;
                udp2[Counter] = (udp2[Counter]*(YHEIGHT-YOFFSET))/YMax;
                icmp2[Counter] = (icmp2[Counter]*(YHEIGHT-YOFFSET))/YMax;

                // Stack 'em up!
                // Total is stacked from the bottom
                // Icmp is on the bottom too
                // Udp is stacked on top of icmp
                udp[Counter] += icmp[Counter];
				udp2[Counter] += icmp2[Counter];
                // TCP and p2p are stacked on top of Udp
                tcp[Counter] += udp[Counter];
                tcp2[Counter] += udp2[Counter];
                p2p[Counter] += udp[Counter];
                p2p2[Counter] += udp2[Counter];
				// Http is stacked on top of p2p
                http[Counter] += p2p[Counter];
                http2[Counter] += p2p2[Counter];
				// Ftp is stacked on top of http
                ftp[Counter] += http[Counter];
                ftp2[Counter] += http2[Counter];

                // Plot them!
				// Sent
                gdImageLine(im, Counter, (YHEIGHT-YOFFSET) - icmp[Counter], Counter, YHEIGHT-YOFFSET-1, red);
                gdImageLine(im, Counter, (YHEIGHT-YOFFSET) - udp[Counter], Counter, (YHEIGHT-YOFFSET) - icmp[Counter] - 1, brown);
                gdImageLine(im, Counter, (YHEIGHT-YOFFSET) - tcp[Counter], Counter, (YHEIGHT-YOFFSET) - udp[Counter] - 1, green);
                gdImageLine(im, Counter, (YHEIGHT-YOFFSET) - p2p[Counter], Counter, (YHEIGHT-YOFFSET) - udp[Counter] - 1, purple);
                gdImageLine(im, Counter, (YHEIGHT-YOFFSET) - http[Counter], Counter, (YHEIGHT-YOFFSET) - p2p[Counter] - 1, blue);
                gdImageLine(im, Counter, (YHEIGHT-YOFFSET) - ftp[Counter], Counter, (YHEIGHT-YOFFSET) - http[Counter] - 1, lblue);
								
				// Receive
                gdImageLine(im2, Counter, (YHEIGHT-YOFFSET) - icmp2[Counter], Counter, YHEIGHT-YOFFSET-1, red2);
                gdImageLine(im2, Counter, (YHEIGHT-YOFFSET) - udp2[Counter], Counter, (YHEIGHT-YOFFSET) - icmp2[Counter] - 1, brown2);
                gdImageLine(im2, Counter, (YHEIGHT-YOFFSET) - tcp2[Counter], Counter, (YHEIGHT-YOFFSET) - udp2[Counter] - 1, green2);
                gdImageLine(im2, Counter, (YHEIGHT-YOFFSET) - p2p2[Counter], Counter, (YHEIGHT-YOFFSET) - udp2[Counter] - 1, purple2);
                gdImageLine(im2, Counter, (YHEIGHT-YOFFSET) - http2[Counter], Counter, (YHEIGHT-YOFFSET) - p2p2[Counter] - 1, blue2);
                gdImageLine(im2, Counter, (YHEIGHT-YOFFSET) - ftp2[Counter], Counter, (YHEIGHT-YOFFSET) - http2[Counter] - 1, lblue2);


                }
            }

	if (SentPeak < 1024/8)
		snprintf(Buffer2, 50, "Peak Send Rate: %.1f Bits/sec", (double)SentPeak*8);
	else if (SentPeak < (1024*1024)/8)
		snprintf(Buffer2, 50, "Peak Send Rate: %.1f KBits/sec", ((double)SentPeak*8.0)/1024.0);
	else snprintf(Buffer2, 50, "Peak Send Rate: %.1f MBits/sec", ((double)SentPeak*8.0)/(1024.0*1024.0));
								
	if (SummaryData->TotalSent < 1024)
		snprintf(Buffer, 30, "Sent %.1f Bytes", (double)SummaryData->TotalSent);					
	else if (SummaryData->TotalSent < 1024*1024)
		snprintf(Buffer, 30, "Sent %.1f KBytes", (double)SummaryData->TotalSent/1024.0);
	else snprintf(Buffer, 30, "Sent %.1f MBytes", (double)SummaryData->TotalSent/(1024.0*1024.0));

	gdImageString(im, gdFontSmall, XOFFSET+5,  YHEIGHT-20, Buffer, black);
	gdImageString(im, gdFontSmall, XWIDTH/2+XOFFSET/2,  YHEIGHT-20, Buffer2, black);				

	if (ReceivedPeak < 1024/8)
       	snprintf(Buffer2, 50, "Peak Receive Rate: %.1f Bits/sec", (double)ReceivedPeak*8);
    else if (ReceivedPeak < (1024*1024)/8)
    	snprintf(Buffer2, 50, "Peak Receive Rate: %.1f KBits/sec", ((double)ReceivedPeak*8.0)/1024.0);               
	else snprintf(Buffer2, 50, "Peak Receive Rate: %.1f MBits/sec", ((double)ReceivedPeak*8.0)/(1024.0*1024.0));

    if (SummaryData->TotalReceived < 1024)
        snprintf(Buffer, 30, "Received %.1f Bytes", (double)SummaryData->TotalReceived);
    else if (SummaryData->TotalReceived < 1024*1024)
        snprintf(Buffer, 30, "Received %.1f KBytes", (double)SummaryData->TotalReceived/1024.0);
    else snprintf(Buffer, 30, "Received %.1f MBytes", (double)SummaryData->TotalReceived/(1024.0*1024.0));
                                                                                                              
    gdImageString(im2, gdFontSmall, XOFFSET+5,  YHEIGHT-20, Buffer, black2);                
    gdImageString(im2, gdFontSmall, XWIDTH/2+XOFFSET/2,  YHEIGHT-20, Buffer2, black2);

    return(YMax);
    }

void PrepareYAxis(gdImagePtr im, unsigned long long int YMax)
    {
    char buffer[20];

	char YLegend;
	long long int Divisor;

    int black;
    float YTic = 0;
    double y;
    long int YStep;
    
    black = gdImageColorAllocate(im, 0, 0, 0);
    gdImageLine(im, XOFFSET, 0, XOFFSET, YHEIGHT, black);

    YLegend = ' ';
    Divisor = 1;
    if (YMax*8 > 1024*2)
        {
        Divisor = 1024;    // Display in K
        YLegend = 'k';
        }
    if (YMax*8 > 1024*1024*2)
        {
        Divisor = 1024*1024; // Display in M
        YLegend = 'm';
        }
    if (YMax*8 > (long long)1024*1024*1024*2)
        {
        Divisor = 1024*1024*1024; // Display in G
        YLegend = 'g';
        }

    YStep = YMax/10;
    if (YStep < 1)
        YStep=1;
    YTic=YStep;

    while (YTic < (YMax - YMax/10))
        {
        y = (YHEIGHT-YOFFSET)-((YTic*(YHEIGHT-YOFFSET))/YMax);        

        gdImageLine(im, XOFFSET, y, XWIDTH, y, black);        
        snprintf(buffer, 20, "%4.1f %cbits/s", (float)(8.0*YTic)/Divisor, YLegend);
        gdImageString(im, gdFontSmall, 3, y-7, buffer, black);        

        YTic += YStep;
        }
    } 

void PrepareXAxis(gdImagePtr im, time_t timestamp)
    {
    char buffer[100];
    int black, red;
    time_t sample_begin, sample_end;    
    struct tm *timestruct;
    time_t MarkTime;
    time_t MarkTimeStep;
    double x;
    
    sample_begin=timestamp-config.range;
    sample_end=sample_begin+config.interval;

    black = gdImageColorAllocate(im, 0, 0, 0);
    //red   = gdImageColorAllocate(im, 255, 0, 0);

    gdImageLine(im, 0, YHEIGHT-YOFFSET, XWIDTH, YHEIGHT-YOFFSET, black);

    // ********************************************************************
    // ****  Write the red day/month seperator bars
    // ********************************************************************

	if ((24*60*60*(XWIDTH-XOFFSET))/config.range > (XWIDTH-XOFFSET)/10)
		{
		// Day bars
	    timestruct = localtime((time_t *)&sample_begin);
    	timestruct->tm_sec = 0;
	    timestruct->tm_min = 0;
    	timestruct->tm_hour = 0;
	    MarkTime = mktime(timestruct);
            
    	x = (MarkTime-sample_begin)*( ((double)(XWIDTH-XOFFSET)) / config.range) + XOFFSET;
	    while (x < XOFFSET)
    	    {
        	MarkTime += (24*60*60);
	        x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
    	    }

	    while (x < (XWIDTH-10))
    	    {
        	// Day Lines
	        gdImageLine(im, x, 0, x, YHEIGHT-YOFFSET, black);
    	    gdImageLine(im, x+1, 0, x+1, YHEIGHT-YOFFSET, black);
	
    	    // 使用 localtime 的副本，防止修改内部静态结构体
    	    struct tm tmp = *localtime(&MarkTime);

    	    // 使用 年-月-日 格式格式化日期（无中文）
    	    strftime(buffer, 100, "%y-%m-%d", &tmp);
    	    gdImageString(im, gdFontSmall, x-30,  YHEIGHT-YOFFSET+10, buffer, black);        

	        // Calculate Next x
    	    MarkTime += (24*60*60);
        	x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
	        }
		}
	else
		{
    	// Month Bars
        timestruct = localtime((time_t *)&sample_begin);
        timestruct->tm_sec = 0;
        timestruct->tm_min = 0;
        timestruct->tm_hour = 0;
		timestruct->tm_mday = 1;
		timestruct->tm_mon--; // Start the month before the sample
        MarkTime = mktime(timestruct);

    	x = (MarkTime-sample_begin)*( ((double)(XWIDTH-XOFFSET)) / config.range) + XOFFSET;
	    while (x < XOFFSET)
    	    {
			timestruct->tm_mon++;
        	MarkTime = mktime(timestruct);
	        x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
    	    }

	    while (x < (XWIDTH-10))
    	    {
        	// Month Lines
	        gdImageLine(im, x, 0, x, YHEIGHT-YOFFSET, black);
    	    gdImageLine(im, x+1, 0, x+1, YHEIGHT-YOFFSET, black);
	
    	    // 使用本地副本，防止修改全局结构体
    	    struct tm tmp = *localtime(&MarkTime);  // 拷贝 localtime 返回的结构体内容
    	    strftime(buffer, 100, "%y-%m-%d", &tmp);  // 用副本打印日期
    	    gdImageString(im, gdFontSmall, x-6,  YHEIGHT-YOFFSET+10, buffer, black);        

	     	// 修改副本的月份
    		tmp.tm_mon++;
    		MarkTime = mktime(&tmp);  // 转成新的时间戳
        	x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
	        }				
		}

    // ********************************************************************
    // ****  Write the tic marks
    // ********************************************************************

    timestruct = localtime((time_t *)&sample_begin);
    timestruct->tm_sec = 0;
    timestruct->tm_min = 0;
    timestruct->tm_hour = 0;
    MarkTime = mktime(timestruct);

	if ((6*60*60*(XWIDTH-XOFFSET))/config.range > 10) // pixels per 6 hours is more than 2
		MarkTimeStep = 6*60*60; // Major ticks are 6 hours
	else if ((24*60*60*(XWIDTH-XOFFSET))/config.range > 10)
		MarkTimeStep = 24*60*60; // Major ticks are 24 hours;
	else
		return; // Done		

	x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
	while (x < XOFFSET)
   		{
		MarkTime += MarkTimeStep;
	    x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
    	}

    while (x < (XWIDTH-10))
    	{
	    if (x > XOFFSET) {
    		gdImageLine(im, x, YHEIGHT-YOFFSET-5, x, YHEIGHT-YOFFSET+5, black);
	       	gdImageLine(im, x+1, YHEIGHT-YOFFSET-5, x+1, YHEIGHT-YOFFSET+5, black);
        	}
		MarkTime += MarkTimeStep;
   		x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
        }

    timestruct = localtime((time_t *)&sample_begin);
    timestruct->tm_sec = 0;
    timestruct->tm_min = 0;
    timestruct->tm_hour = 0;
    MarkTime = mktime(timestruct);

	if ((60*60*(XWIDTH-XOFFSET))/config.range > 2) // pixels per hour is more than 2
		MarkTimeStep = 60*60;  // Minor ticks are 1 hour
	else if ((6*60*60*(XWIDTH-XOFFSET))/config.range > 2)
		MarkTimeStep = 6*60*60; // Minor ticks are 6 hours
	else if ((24*60*60*(XWIDTH-XOFFSET))/config.range > 2)
		MarkTimeStep = 24*60*60;
	else
		return; // Done

	// Draw Minor Tic Marks
	x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;

	while (x < XOFFSET)
   		{
		MarkTime += MarkTimeStep;
	    x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
    	}

    while (x < (XWIDTH-10))
        {
	    if (x > XOFFSET) {
    		gdImageLine(im, x, YHEIGHT-YOFFSET, x, YHEIGHT-YOFFSET+5, black);
        	gdImageLine(im, x+1, YHEIGHT-YOFFSET, x+1, YHEIGHT-YOFFSET+5, black);
            }
	    MarkTime+=MarkTimeStep;
    	x = (MarkTime-sample_begin)*((XWIDTH-XOFFSET)/config.range) + XOFFSET;
        }
    }
