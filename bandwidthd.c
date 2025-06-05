#include "bandwidthd.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> 
#include <sys/stat.h>
#ifdef HAVE_LIBPQ
#include <libpq-fe.h>
#endif

// We must call regular exit to write out profile data, but child forks are supposed to usually
// call _exit?
#ifdef PROFILE
#define _exit(x) exit(x)
#endif

#define WEB_ROOT "/tmp/Bandwidthd_html"
#define INDEX_PAGE "lljk.html"
#define BUFFER_SIZE 8096
/*
#ifdef DEBUG
#define fork() (0)
#endif
*/

// ****************************************************************************************
// ** Global Variables
// ****************************************************************************************
const char *legend_base64 = "data:image/gif;base64,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";
const char *logo_base64 = "data:image/gif;base64,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";

static pcap_t *pd;

unsigned int GraphIntervalCount = 0;
unsigned int IpCount = 0;
unsigned int SubnetCount = 0;
time_t IntervalStart;
time_t ProgramStart;
int RotateLogs = FALSE;
    
struct SubnetData SubnetTable[SUBNET_NUM];
struct IPData IpTable[IP_NUM];

int DataLink;
int IP_Offset;

struct IPDataStore *IPDataStore = NULL;
extern int bdconfig_parse(void);
extern FILE *bdconfig_in;

struct config config;

pid_t workerchildpids[NR_WORKER_CHILDS];

void signal_handler(int sig)
	{
	switch (sig) 
		{
		case SIGHUP:
			signal(SIGHUP, signal_handler);
			RotateLogs++;
			if (config.tag == '1') 
				{
				int i;

				/* signal children */
				for (i=0; i < NR_WORKER_CHILDS; i++) 
					kill(workerchildpids[i], SIGHUP);
				}
			break;
		case SIGTERM:
			if (config.tag == '1') 
				{
				int i;

				/* send term signal to children */
				for (i=0; i < NR_WORKER_CHILDS; i++) 
					kill(workerchildpids[i], SIGTERM);
				}
			// TODO: Might want to make sure we're not in the middle of wrighting out a log file
			exit(0);
			break;
		}
	}

void bd_CollectingData(char *filename)
	{
	FILE *index;
	if (access("/tmp/Bandwidthd_html", F_OK) != 0)
    	{
        	if (mkdir("/tmp/Bandwidthd_html", 0755) != 0)
        	{
            	syslog(LOG_ERR, "创建目录 /tmp/Bandwidthd_html 失败，将无法生成html页面文件");
        	}
    	}
    	// 如果文件已存在并且不为空，则直接跳过创建
    	struct stat st;
    	if (stat(filename, &st) == 0 && st.st_size > 0)
    	{
        	//syslog(LOG_INFO, "文件 %s 已存在且不为空，跳过 HTML 页面生成", filename);
        	return;
    	}
	index = fopen(filename, "wt");	
	if (index)
		{
		fprintf(index, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
		fprintf(index, "<HTML><HEAD><TITLE>Bandwidthd</TITLE>\n");
		fprintf(index, "<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=UTF-8\">\n");
		if (config.meta_refresh)
			fprintf(index, "<META HTTP-EQUIV=\"REFRESH\" content=\"%u\">\n",
					config.meta_refresh);
		fprintf(index, "<META HTTP-EQUIV=\"EXPIRES\" content=\"-1\">\n");
		fprintf(index, "<META HTTP-EQUIV=\"PRAGMA\" content=\"no-cache\">\n");
		fprintf(index, "</HEAD>\n<BODY><center><img src=\"%s\" ALT=\"Logo\"><BR>\n", logo_base64);
		fprintf(index, "<BR>\n - <a href=\"lljk.html\">今日</a> -- <a href=\"lljk2.html\">周</a> -- ");
		fprintf(index, "<a href=\"lljk3.html\">月</a> -- <a href=\"lljk4.html\">年</a><BR>\n");
		fprintf(index, "<br><div align=\"center\">bandwidthd 没有要绘制的图表。您应该等待一段时间后就能看到流量统计图表。如果没有，请查看 README 中的 \"已知 bug 和故障排除\" 部分。</div>");
		
		fprintf(index, "</BODY></HTML>\n");
		fclose(index);
		}
	else
		{
		syslog(LOG_ERR, "无法打开 %s 进行写入", filename);
		exit(1);
		}
	}

int WriteOutWebpages(long int timestamp)
{
	struct IPDataStore *DataStore = IPDataStore;
	struct SummaryData **SummaryData;
	int NumGraphs = 0;
	pid_t graphpid;
	int Counter;

	/* Did we catch any packets since last time? */
	if (!DataStore) return -1;

	// break off from the main line so we don't miss any packets while we graph
	graphpid = fork();

	switch (graphpid) {
		case 0: /* we're the child, graph. */
			{
#ifdef PROFILE
			// Got this incantation from a message board.  Don't forget to set
			// GMON_OUT_PREFIX in the shell
			extern void _start(void), etext(void);
			syslog(LOG_INFO, "调用分析器启动...");
			monstartup((u_long) &_start, (u_long) &etext);
#endif
          	signal(SIGHUP, SIG_IGN);
			
    	    nice(4); // reduce priority so I don't choke out other tasks

			// Count Number of IP's in datastore
			for (DataStore = IPDataStore, Counter = 0; DataStore; Counter++, DataStore = DataStore->Next);

			// +1 because we don't want to accidently allocate 0
			SummaryData = malloc(sizeof(struct SummaryData *)*Counter+1);

			DataStore = IPDataStore;
			while (DataStore) // Is not null
				{
				if (DataStore->FirstBlock->NumEntries > 0)
					{
					SummaryData[NumGraphs] = (struct SummaryData *) malloc(sizeof(struct SummaryData));
					GraphIp(DataStore, SummaryData[NumGraphs++], timestamp+LEAD*config.range);
					}
			    DataStore = DataStore->Next;
				}

			MakeIndexPages(NumGraphs, SummaryData);
	
			_exit(0);
			}
		break;

		case -1:
			syslog(LOG_ERR, "生成子进程失败!");
			return -2;
		break;

		default: /* parent + successful fork, assume graph success */
			return 0;
		break;
	}
}

void setchildconfig (int level) {
	static unsigned long long graph_cutoff;

	switch (level) {
		case 0:
			config.range = RANGE1;
			config.interval = INTERVAL1;
			config.tag = '1';
			graph_cutoff = config.graph_cutoff;
		break;
		case 1:
			// Overide skip_intervals for children
			config.skip_intervals = CONFIG_GRAPHINTERVALS;
			config.range = RANGE2;
			config.interval = INTERVAL2;
			config.tag = '2';
			config.graph_cutoff = graph_cutoff*(RANGE2/RANGE1);	
		break;
		case 2:
			// Overide skip_intervals for children
			config.skip_intervals = CONFIG_GRAPHINTERVALS;
			config.range = RANGE3;
			config.interval = INTERVAL3;
			config.tag = '3';
			config.graph_cutoff = graph_cutoff*(RANGE3/RANGE1);
		break;
		case 3:
			// Overide skip_intervals for children
			config.skip_intervals = CONFIG_GRAPHINTERVALS;
			config.range = RANGE4;
			config.interval = INTERVAL4;
			config.tag = '4';
			config.graph_cutoff = graph_cutoff*(RANGE4/RANGE1);
		break;

		default:
			syslog(LOG_ERR, "setchildconfig 收到了一个无效的 level 参数: %d", level);
			_exit(1);
	}
}

void makepidfile(pid_t pid) 
	{
	FILE *pidfile;

	pidfile = fopen("/var/run/bandwidthd.pid", "wt");
	if (pidfile) 
		{
		if (fprintf(pidfile, "%d\n", pid) == 0) 
			{
			syslog(LOG_ERR, "Bandwidthd: 无法将 '%d' 写入 /var/run/bandwidthd.pid", pid);
			fclose(pidfile);
			unlink("/var/run/bandwidthd.pid");
			}
		else
			fclose(pidfile);		
		}
	else
		syslog(LOG_ERR, "无法打开 /var/run/bandwidthd.pid 进行写入");
	}

// 启动 HTTP 服务函数（后台运行）
void start_http_server(int port) {
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "无法 fork HTTP 子进程");
        return;
    } else if (pid > 0) {
        return;  // 父进程直接返回
    }

    // 子进程独立运行
    setsid();                        // 脱离控制终端
    chdir("/");                      // 改变工作目录，避免锁定
    close(STDIN_FILENO);             // 关闭标准输入
    close(STDOUT_FILENO);            // 关闭标准输出
    close(STDERR_FILENO);            // 关闭标准错误

    int server_fd;
    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(port);
    addr6.sin6_addr = in6addr_any;

    server_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "IPv6 socket 创建失败，尝试使用 IPv4...");
        goto ipv4_fallback;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    opt = 0;
    setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)); // 启用 IPv4 映射

    if (bind(server_fd, (struct sockaddr*)&addr6, sizeof(addr6)) < 0) {
        syslog(LOG_ERR, "IPv6 bind 失败，尝试使用 IPv4...");
        close(server_fd);
        goto ipv4_fallback;
    }

    goto serve;

ipv4_fallback: {
    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(port);
    addr4.sin_addr.s_addr = INADDR_ANY;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "IPv4 socket 创建失败");
        exit(1);
    }

    opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&addr4, sizeof(addr4)) < 0) {
        syslog(LOG_ERR, "IPv4 bind 失败");
        close(server_fd);
        exit(1);
    }
}

serve:
    if (listen(server_fd, 10) < 0) {
        syslog(LOG_ERR, "HTTP listen 失败");
        close(server_fd);
        exit(1);
    }

    syslog(LOG_ERR, "HTTP 服务已启动，监听端口 %d", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char buffer[BUFFER_SIZE] = {0};
        read(client_fd, buffer, BUFFER_SIZE - 1);

        // 解析 HTTP 请求第一行：方法和路径
        char method[8], path[256];
        char *line = strtok(buffer, "\r\n");
        if (!line || sscanf(line, "%7s %255s", method, path) != 2) {
            close(client_fd);
            continue;
        }

        // 移除路径中的锚点 (#...) 或参数 (?...)
        char *p = strpbrk(path, "?#");
        if (p) *p = '\0';

        // 判断是否为根目录请求
        const char *req_path = (strcmp(path, "/") == 0) ? INDEX_PAGE : path + 1;

        // 构建文件绝对路径
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", WEB_ROOT, req_path);

        // 打开文件并发送
        int fd = open(file_path, O_RDONLY);
        if (fd < 0) {
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
            write(client_fd, not_found, strlen(not_found));
        } else {
            const char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            write(client_fd, header, strlen(header));
            ssize_t n;
            while ((n = read(fd, buffer, BUFFER_SIZE)) > 0) {
                write(client_fd, buffer, n);
            }
            close(fd);
        }

        close(client_fd);
    }

    close(server_fd);
}

#define DEFAULT_CONFIG_DIR "/etc/storage"
int main(int argc, char **argv)
    {
    struct bpf_program fcode;
    u_char *pcap_userdata = 0;
#ifdef HAVE_PCAP_FINDALLDEVS
	pcap_if_t *Devices;
#endif
	char Error[PCAP_ERRBUF_SIZE];
	struct stat StatBuf;
	int i;
	int ForkBackground = TRUE;
	int ListDevices = FALSE;
	int Counter;
	
	ProgramStart = time(NULL);

	config.dev = NULL;
	config.filter = "ip";
	config.skip_intervals = CONFIG_GRAPHINTERVALS;
	config.graph_cutoff = CONFIG_GRAPHCUTOFF;
	config.promisc = TRUE;
	config.graph = TRUE;
	config.output_cdf = FALSE;
	config.recover_cdf = FALSE;
	config.meta_refresh = CONFIG_METAREFRESH;
	config.output_database = FALSE;
	config.db_connect_string = NULL;
	config.sensor_id = "unset";  

	openlog("bandwidthd", LOG_CONS, LOG_DAEMON);
	char resolved_path[1024];
    	char resolved_copy[1024];
    	char *INSTALL_DIR = NULL;
    	if (realpath(argv[0], resolved_path)) {
        	strncpy(resolved_copy, resolved_path, sizeof(resolved_copy));
        	resolved_copy[sizeof(resolved_copy) - 1] = '\0';
        	INSTALL_DIR = strdup(dirname(resolved_copy));  
    	} else {
        	INSTALL_DIR = strdup(DEFAULT_CONFIG_DIR);
    	}

	if (stat("/etc/storage/bandwidthd.conf", &StatBuf))
		{
    		
		char config_path[1024];
		snprintf(config_path, sizeof(config_path), "%s/bandwidthd.conf", INSTALL_DIR);
		if (stat(config_path, &StatBuf))
			{
			printf("无法找到 /etc/storage/bandwidthd.conf 或 %s/bandwidthd.conf\n", INSTALL_DIR);
			syslog(LOG_ERR, "无法找到 /etc/storage/bandwidthd.conf 或 %s/bandwidthd.conf", INSTALL_DIR);
			exit(1);
			}
		else
		        {
		        bdconfig_in = fopen(config_path, "rt");
                        }
		}
	else
	        {
			bdconfig_in = fopen("/etc/storage/bandwidthd.conf", "rt");
		}

	if (!bdconfig_in)
		{
		syslog(LOG_ERR, "无法打开 bandwidthd.conf");
		printf("无法打开 bandwidthd.conf\n");
		exit(1);
		}
	bdconfig_parse();
	/*
	// Scary
	printf("Estimated max ram utilization\nDataPoints = %.0f/%ld = %.0f\nIPData = %d * DataPoints = %.1f (%.2fKBytes) per IP\nIP_NUM = %d\nTotal = %.1fMBytes * 4 to 8 = %.1fMBytes to %.1fMBytes\n", 
		RANGE1, INTERVAL1, RANGE1/INTERVAL1,		
		sizeof(struct IPData), 
		(float) sizeof(struct IPData)*(RANGE1/INTERVAL1), 
		(float) (sizeof(struct IPData)*(RANGE1/INTERVAL1))/1024.0,
		IP_NUM, 
		(float)((sizeof(struct IPData)*(RANGE1/INTERVAL1)*IP_NUM)/1024.0)/1024.0,
		(float)4*((sizeof(struct IPData)*(RANGE1/INTERVAL1)*IP_NUM)/1024.0)/1024.0,
		(float)8*((sizeof(struct IPData)*(RANGE1/INTERVAL1)*IP_NUM)/1024.0)/1024.0);
	printf("Sizeof unsigned long: %d, sizeof unsigned long long: %d\n%lu, %llu\n",
		sizeof(unsigned long), sizeof (unsigned long long),
		(unsigned long) (0-1), (unsigned long long) (0-1));
		exit(1);
	*/
 	int ListenPort = 8080;
	for(Counter = 1; Counter < argc; Counter++)
		{
		if (argv[Counter][0] == '-')
			{
			switch(argv[Counter][1])
				{
				case 'D':
				case 'd':
					ForkBackground = FALSE;
					break;
				case 'L':
				case 'l':
					ListDevices = TRUE; 
			 		break;
			 	case 'P':
				case 'p':
					if (Counter + 1 < argc) 
					{
						ListenPort = atoi(argv[++Counter]); 
						if (ListenPort <= 0 || ListenPort > 65535)
						{
							printf("无效的端口号: %s\n", argv[Counter]);
							exit(1);
						}
					}
					else
					{
						printf("缺少 -P 参数后的端口号\n");
						exit(1);
					}
					break;
				default:
					printf("不正确的参数: %s\n", argv[Counter]);
					exit(1);
				}
			}
		}

#ifdef HAVE_PCAP_FINDALLDEVS
	pcap_findalldevs(&Devices, Error);
	if (config.dev == NULL && Devices->name)
		config.dev = strdup(Devices->name);
	if (ListDevices)
		{	
		while(Devices)
			{
			printf("描述: %s\n名称: \"%s\"\n\n", Devices->description, Devices->name);
			Devices = Devices->next;
			}
		exit(0);
		}
#else
	if (ListDevices)
		{
		printf("您的 libpcap 版本不支持列出设备\n");
		exit(0);
		}
#endif	

	if (config.graph)
		{
		bd_CollectingData("/tmp/Bandwidthd_html/lljk.html");
		bd_CollectingData("/tmp/Bandwidthd_html/lljk2.html");
		bd_CollectingData("/tmp/Bandwidthd_html/lljk3.html");
		bd_CollectingData("/tmp/Bandwidthd_html/lljk4.html");
		start_http_server(ListenPort); // ListenPort 是通过 -P 参数设置的
		}

	/* detach from console. */
	if (ForkBackground)
		if (fork2())
			exit(0);

	makepidfile(getpid());

	setchildconfig(0); /* initialize first (day graphing) process config */

	if (config.graph || config.output_cdf)
		{
		/* fork processes for week, month and year graphing. */
		for (i=0; i<NR_WORKER_CHILDS; i++) 
			{
			workerchildpids[i] = fork();

			/* initialize children and let them start doing work,
			 * while parent continues to fork children.
			 */

			if (workerchildpids[i] == 0) 
				{ /* child */
				setchildconfig(i+1);
				break;
				}

			if (workerchildpids[i] == -1) 
				{ /* fork failed */
				syslog(LOG_ERR, "图形生成子进程分叉失败 (%d)", i);
				/* i--; ..to retry? -> possible infinite loop */
				continue;
				}
			}

	    if(config.recover_cdf)
		    RecoverDataFromCDF();
		}	

    IntervalStart = time(NULL);

	syslog(LOG_INFO, "正在检测 %s 接口", config.dev);	
	pd = pcap_open_live(config.dev, 100, config.promisc, 1000, Error);
        if (pd == NULL) 
			{
			syslog(LOG_ERR, "%s", Error);
			exit(0);
			}

    if (pcap_compile(pd, &fcode, config.filter, 1, 0) < 0)
		{
        pcap_perror(pd, "Error");
		printf("在 bandwidthd.conf 中的 libpcap 过滤器字符串格式不正确\n");
		syslog(LOG_ERR, "在 bandwidthd.conf 中的 libpcap 过滤器字符串格式不正确");
		exit(1);
		}

    if (pcap_setfilter(pd, &fcode) < 0)
        pcap_perror(pd, "Error");

	switch (DataLink = pcap_datalink(pd))
		{
		default:
			if (config.dev)
				printf("未知的数据链路类型 %d, 默认使用Ethernet\n请将此错误信息和一个数据包样本（使用 \"tcpdump -i %s -s 2000 -n -w capture.cap\"捕获）发送至 hinkle@derbyworks.com\n", DataLink, config.dev);
			else
				printf("未知的数据链路类型 %d, 默认使用Ethernet\n请将此错误信息和一个数据包样本（使用 \"tcpdump -s 2000 -n -w capture.cap\"捕获）发送至 hinkle@derbyworks.com\n", DataLink);
			syslog(LOG_INFO, "未知的数据链路类型, 默认使用Ethernet");
		case DLT_EN10MB:
			syslog(LOG_INFO, "数据包编码: Ethernet");
			IP_Offset = 14; //IP_Offset = sizeof(struct ether_header);
			break;	
#ifdef DLT_LINUX_SLL 
		case DLT_LINUX_SLL:
			syslog(LOG_INFO, "数据包编码: Linux Cooked Socket");
			IP_Offset = 16;
			break;
#endif
#ifdef DLT_RAW
		case DLT_RAW:
			printf("未测试的数据链路类型 %d\n如果 bandwidthd 在此接口上对您有效，请报告至 hinkle@derbyworks.net \n", DataLink);
			printf("数据包编码:\n\tRaw\n");
			syslog(LOG_INFO, "未测试的数据包编码: Raw");
			IP_Offset = 0;
			break;
#endif
		case DLT_IEEE802:
			printf("未测试的数据链路类型 %d\n如果 bandwidthd 在此接口上对您有效，请报告至 hinkle@derbyworks.net \n", DataLink);
			printf("数据包编码:\nToken Ring\n");
			syslog(LOG_INFO, "未测试的数据包编码: Token Ring");
			IP_Offset = 22;
			break;
		}

	if (ForkBackground)
		{                                           
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
		}

	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);

	if (IPDataStore)  // If there is data in the datastore draw some initial graphs
		{
		syslog(LOG_INFO, "绘制初始图表");
		WriteOutWebpages(IntervalStart+config.interval);
		}

    if (pcap_loop(pd, -1, PacketCallback, pcap_userdata) < 0) {
        syslog(LOG_ERR, "Bandwidthd: pcap_loop: %s",  pcap_geterr(pd));
        exit(1);
        }

    pcap_close(pd);
    exit(0);        
    }
   
void PacketCallback(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
    {
    unsigned int counter;

    u_int caplen = h->caplen;
    const struct ip *ip;

    uint32_t srcip;
    uint32_t dstip;

    struct IPData *ptrIPData;

    if (h->ts.tv_sec > IntervalStart + config.interval)  // Then write out this intervals data and possibly kick off the grapher
        {
        GraphIntervalCount++;
        CommitData(IntervalStart+config.interval);
		IpCount = 0;
        IntervalStart=h->ts.tv_sec;
        }

    caplen -= IP_Offset;  // We're only measuring ip size, so pull off the ethernet header
    p += IP_Offset; // Move the pointer past the datalink header

    ip = (const struct ip *)p; // Point ip at the ip header

	if (ip->ip_v != 4) // then not an ip packet so skip it
		return;

    srcip = ntohl(*(uint32_t *) (&ip->ip_src));
    dstip = ntohl(*(uint32_t *) (&ip->ip_dst));
	
    for (counter = 0; counter < SubnetCount; counter++)
        {	 
		// Packets from a monitored subnet to a monitored subnet will be
		// credited to both ip's

        if (SubnetTable[counter].ip == (srcip & SubnetTable[counter].mask))
            {
            ptrIPData = FindIp(srcip);  // Return or create this ip's data structure
			if (ptrIPData)
	            Credit(&(ptrIPData->Send), ip);

            ptrIPData = FindIp(0);  // Totals
			if (ptrIPData)
	            Credit(&(ptrIPData->Send), ip);
            }
    
        if (SubnetTable[counter].ip == (dstip & SubnetTable[counter].mask))
            {
            ptrIPData = FindIp(dstip);
    		if (ptrIPData)
		        Credit(&(ptrIPData->Receive), ip);

            ptrIPData = FindIp(0);
    		if (ptrIPData)
		        Credit(&(ptrIPData->Receive), ip);
            }                        
        }
    }

inline void Credit(struct Statistics *Stats, const struct ip *ip)
    {
    unsigned long size;
    const struct tcphdr *tcp;
    uint16_t sport, dport;

    size = ntohs(ip->ip_len);

    Stats->total += size;
    
    switch(ip->ip_p)
        {
        case 6:     // TCP
            tcp = (struct tcphdr *)(ip+1);
			tcp = (struct tcphdr *) ( ((char *)tcp) + ((ip->ip_hl-5)*4) ); // Compensate for IP Options
            Stats->tcp += size;
            sport = ntohs(tcp->TCPHDR_SPORT);
            dport = ntohs(tcp->TCPHDR_DPORT);			
            if (sport == 80 || dport == 80 || sport == 443 || dport == 443)
                Stats->http += size;
	
			if (sport == 20 || dport == 20 || sport == 21 || dport == 21)
				Stats->ftp += size;

            if ( sport == 25 || dport == 25 || sport == 465 || dport == 465 || sport == 587 || dport == 587)   // IN and OUT email
                Stats->p2p += size;
            break;
        case 17:
            Stats->udp += size;
            break;
        case 1: 
            Stats->icmp += size;
            break;
        }
    }

// TODO:  Throw away old data!
void DropOldData(long int timestamp) 	// Go through the ram datastore and dump old data
	{
	struct IPDataStore *DataStore;
	struct IPDataStore *PrevDataStore;	
	struct DataStoreBlock *DeletedBlock;
	
	PrevDataStore = NULL;
    DataStore = IPDataStore;

	// Progress through the linked list until we reach the end
	while(DataStore)  // we have data
		{
		// If the First block is out of date, purge it, if it is the only block
		// purge the node
        while(DataStore->FirstBlock->LatestTimestamp < timestamp - config.range)
			{
            if ((!DataStore->FirstBlock->Next) && PrevDataStore) // There is no valid block of data for this ip, so unlink the whole ip
				{ 												// Don't bother unlinking the ip if it's the first one, that's to much
																// Trouble
				PrevDataStore->Next = DataStore->Next;	// Unlink the node
				free(DataStore->FirstBlock->Data);      // Free the memory
				free(DataStore->FirstBlock);
				free(DataStore);												
				DataStore = PrevDataStore->Next;	// Go to the next node
				if (!DataStore) return; // We're done
				}				
			else if (!DataStore->FirstBlock->Next)
				{
				// There is no valid block of data for this ip, and we are 
				// the first ip, so do nothing 
				break; // break out of this loop so the outside loop increments us
				} 
			else // Just unlink this block
				{
				DeletedBlock = DataStore->FirstBlock;
				DataStore->FirstBlock = DataStore->FirstBlock->Next;	// Unlink the block
				free(DeletedBlock->Data);
				free(DeletedBlock);
			    }
			}

		PrevDataStore = DataStore;				
		DataStore = DataStore->Next;
		}
	}

void StoreIPDataInPostgresql(struct IPData IncData[])
	{
#ifdef HAVE_LIBPQ
	struct IPData *IPData;
	unsigned int counter;
	struct Statistics *Stats;
    PGresult   *res;
	static PGconn *conn = NULL;
	static char sensor_id[50];
	const char *paramValues[10];
	char *sql1; 
	char *sql2;
	char Values[10][50];

	if (!config.output_database == DB_PGSQL)
		return;

	paramValues[0] = Values[0];
	paramValues[1] = Values[1];
	paramValues[2] = Values[2];
	paramValues[3] = Values[3];	
	paramValues[4] = Values[4];
	paramValues[5] = Values[5];
	paramValues[6] = Values[6];
	paramValues[7] = Values[7];
	paramValues[8] = Values[8];
	paramValues[9] = Values[9];

	// ************ Inititialize the db if it's not already
	if (!conn)
		{
		/* Connect to the database */
    	conn = PQconnectdb(config.db_connect_string);

	    /* Check to see that the backend connection was successfully made */
    	if (PQstatus(conn) != CONNECTION_OK)
        	{
	       	syslog(LOG_ERR, "连接到数据库 '%s' 失败: %s", config.db_connect_string, PQerrorMessage(conn));
    	    PQfinish(conn);
        	conn = NULL;
	        return;
    	    }

		strncpy(Values[0], config.sensor_id, 50);
		res = PQexecParams(conn, "select sensor_id from sensors where sensor_name = $1;",
			1,       /* one param */
            NULL,    /* let the backend deduce param type */
   	        paramValues,
       	    NULL,    /* don't need param lengths since text */
           	NULL,    /* default to all text params */
            0);      /* ask for binary results */
		
   		if (PQresultStatus(res) != PGRES_TUPLES_OK)
       		{
        	syslog(LOG_ERR, "Postresql 查询失败: %s", PQerrorMessage(conn));
    	    PQclear(res);
   	    	PQfinish(conn);
    	    conn = NULL;
       		return;
	        }

		if (PQntuples(res))
			{
			strncpy(sensor_id, PQgetvalue(res, 0, 0), 50);
			PQclear(res);
			}
		else
			{
			// Insert new sensor id
			PQclear(res);
			res = PQexecParams(conn, "insert into sensors (sensor_name, last_connection) VALUES ($1, now());",
				1,       /* one param */
    	        NULL,    /* let the backend deduce param type */
   	    	    paramValues,
       	    	NULL,    /* don't need param lengths since text */
				NULL,    /* default to all text params */
            	0);      /* ask for binary results */

    		if (PQresultStatus(res) != PGRES_COMMAND_OK)
        		{
	        	syslog(LOG_ERR, "Postresql 插入失败: %s", PQerrorMessage(conn));
	    	    PQclear(res);
    	    	PQfinish(conn);
	    	    conn = NULL;
        		return;
		        }
			PQclear(res);
			res = PQexecParams(conn, "select sensor_id from sensors where sensor_name = $1;",
				1,       /* one param */
        	    NULL,    /* let the backend deduce param type */
   	        	paramValues,
	       	    NULL,    /* don't need param lengths since text */
    	       	NULL,    /* default to all text params */
        	    0);      /* ask for binary results */
		
	   		if (PQresultStatus(res) != PGRES_TUPLES_OK)
    	   		{
        		syslog(LOG_ERR, "Postresql 查询失败: %s", PQerrorMessage(conn));
    	    	PQclear(res);
	   	    	PQfinish(conn);
    		    conn = NULL;
       			return;
	        	}
			strncpy(sensor_id, PQgetvalue(res, 0, 0), 50);
			PQclear(res);
			}	
		}

	// Begin transaction

	// **** Perform inserts
	res = PQexecParams(conn, "BEGIN;",
			0,       /* zero param */
    	    NULL,    /* let the backend deduce param type */
   	    	NULL,
	    	NULL,    /* don't need param lengths since text */
			NULL,    /* default to all text params */
       		0);      /* ask for binary results */

 	if (PQresultStatus(res) != PGRES_COMMAND_OK)
    	{
	    syslog(LOG_ERR, "Postresql 开始事务失败: %s", PQerrorMessage(conn));
	    PQclear(res);
    	PQfinish(conn);
	    conn = NULL;
        return;
		}							
	PQclear(res);

	strncpy(Values[0], sensor_id, 50);

	res = PQexecParams(conn, "update sensors set last_connection = now() where sensor_id = $1;",
			1,       /* one param */
    	    NULL,    /* let the backend deduce param type */
   	    	paramValues,
	    	NULL,    /* don't need param lengths since text */
			NULL,    /* default to all text params */
       		0);      /* ask for binary results */

 	if (PQresultStatus(res) != PGRES_COMMAND_OK)
    	{
	    syslog(LOG_ERR, "Postresql 更新失败: %s", PQerrorMessage(conn));
	    PQclear(res);
    	PQfinish(conn);
	    conn = NULL;
        return;
		}							
	PQclear(res);

	Values[0][49] = '\0';
	snprintf(Values[1], 50, "%llu", config.interval);
	for (counter=0; counter < IpCount; counter++)
		{
        IPData = &IncData[counter];

		if (IPData->ip == 0)
			{
			// This optimization allows us to quickly draw totals graphs for a sensor
			sql1 = "INSERT INTO bd_tx_total_log (sensor_id, sample_duration, ip, total, icmp, udp, tcp, ftp, http, p2p) VALUES($1, $2, $3, $4, $5, $6, $7, $8, $9, $10);";
			sql2 = "INSERT INTO bd_rx_total_log (sensor_id, sample_duration, ip, total, icmp, udp, tcp, ftp, http, p2p) VALUES($1, $2, $3, $4, $5, $6, $7, $8, $9, $10);";
			}
		else
			{
			sql1 = "INSERT INTO bd_tx_log (sensor_id, sample_duration, ip, total, icmp, udp, tcp, ftp, http, p2p) VALUES($1, $2, $3, $4, $5, $6, $7, $8, $9, $10);";
			sql2 = "INSERT INTO bd_rx_log (sensor_id, sample_duration, ip, total, icmp, udp, tcp, ftp, http, p2p) VALUES($1, $2, $3, $4, $5, $6, $7, $8, $9, $10);"; 
			}

        HostIp2CharIp(IPData->ip, Values[2]);

		Stats = &(IPData->Send);
		if (Stats->total > 512) // Don't log empty sets
			{
			// Log data in kilobytes
			snprintf(Values[3], 50, "%llu", (long long unsigned int)((((double)Stats->total)/1024.0) + 0.5));
			snprintf(Values[4], 50, "%llu", (long long unsigned int)((((double)Stats->icmp)/1024.0) + 0.5));
			snprintf(Values[5], 50, "%llu", (long long unsigned int)((((double)Stats->udp)/1024.0) + 0.5));
			snprintf(Values[6], 50, "%llu", (long long unsigned int)((((double)Stats->tcp)/1024.0) + 0.5));
			snprintf(Values[7], 50, "%llu", (long long unsigned int)((((double)Stats->ftp)/1024.0) + 0.5));
			snprintf(Values[8], 50, "%llu", (long long unsigned int)((((double)Stats->http)/1024.0) + 0.5));
			snprintf(Values[9], 50, "%llu", (long long unsigned int)((((double)Stats->p2p)/1024.0) + 0.5));

			res = PQexecParams(conn, sql1,
				10,       /* nine param */
	            NULL,    /* let the backend deduce param type */
    	        paramValues,
        	    NULL,    /* don't need param lengths since text */
            	NULL,    /* default to all text params */
	            1);      /* ask for binary results */

    		if (PQresultStatus(res) != PGRES_COMMAND_OK)
        		{
	        	syslog(LOG_ERR, "Postresql 插入失败: %s", PQerrorMessage(conn));
	    	    PQclear(res);
    	    	PQfinish(conn);
	    	    conn = NULL;
        		return;
		        }
			PQclear(res);
			}
		Stats = &(IPData->Receive);
		if (Stats->total > 512) // Don't log empty sets
			{
			snprintf(Values[3], 50, "%llu", (long long unsigned int)((((double)Stats->total)/1024.0) + 0.5));
			snprintf(Values[4], 50, "%llu", (long long unsigned int)((((double)Stats->icmp)/1024.0) + 0.5));
			snprintf(Values[5], 50, "%llu", (long long unsigned int)((((double)Stats->udp)/1024.0) + 0.5));
			snprintf(Values[6], 50, "%llu", (long long unsigned int)((((double)Stats->tcp)/1024.0) + 0.5));
			snprintf(Values[7], 50, "%llu", (long long unsigned int)((((double)Stats->ftp)/1024.0) + 0.5));
			snprintf(Values[8], 50, "%llu", (long long unsigned int)((((double)Stats->http)/1024.0) + 0.5));
			snprintf(Values[9], 50, "%llu", (long long unsigned int)((((double)Stats->p2p)/1024.0) + 0.5));

			res = PQexecParams(conn, sql2,
				10,       /* seven param */
            	NULL,    /* let the backend deduce param type */
	            paramValues,
    	        NULL,    /* don't need param lengths since text */
        	    NULL,    /* default to all text params */
            	1);      /* ask for binary results */

	    	if (PQresultStatus(res) != PGRES_COMMAND_OK)
    	    	{
	    	    syslog(LOG_ERR, "Postresql 插入失败: %s", PQerrorMessage(conn));
    	    	PQclear(res);
	        	PQfinish(conn);
		        conn = NULL;
        		return;
	        	}
			PQclear(res);
			}		
		}
	// Commit transaction
	res = PQexecParams(conn, "COMMIT;",
			0,       /* zero param */
    	    NULL,    /* let the backend deduce param type */
   	    	NULL,
	    	NULL,    /* don't need param lengths since text */
			NULL,    /* default to all text params */
       		0);      /* ask for binary results */

 	if (PQresultStatus(res) != PGRES_COMMAND_OK)
    	{
	    syslog(LOG_ERR, "Postresql 提交失败: %s", PQerrorMessage(conn));
	    PQclear(res);
    	PQfinish(conn);
	    conn = NULL;
        return;
		}							
	PQclear(res);
#else
	syslog(LOG_ERR, "选择了Postgresql日志记录，但Postgresql支持未编译到二进制文件中。请查看随本软件分发的README文档。");
#endif
	}

void StoreIPDataInDatabase(struct IPData IncData[])
	{
	if (config.output_database == DB_PGSQL)
		StoreIPDataInPostgresql(IncData);
	else if (config.output_database == DB_SQLITE)
 		sqliteStoreIPData(IncData);
	}

void StoreIPDataInCDF(struct IPData IncData[])
	{
	struct IPData *IPData;
	unsigned int counter;
	FILE *cdf;
	struct Statistics *Stats;
	char IPBuffer[50];
	char logfile[] = "/tmp/Bandwidthd_html/htdocs/log.1.0.cdf";
	
	logfile[32] = config.tag;	

   	cdf = fopen(logfile, "at");

	for (counter=0; counter < IpCount; counter++)
		{
		IPData = &IncData[counter];
		HostIp2CharIp(IPData->ip, IPBuffer);
		fprintf(cdf, "%s,%lu,", IPBuffer, IPData->timestamp);
		Stats = &(IPData->Send);
		fprintf(cdf, "%llu,%llu,%llu,%llu,%llu,%llu,%llu,", Stats->total, Stats->icmp, Stats->udp, Stats->tcp, Stats->ftp, Stats->http, Stats->p2p); 
		Stats = &(IPData->Receive);
		fprintf(cdf, "%llu,%llu,%llu,%llu,%llu,%llu,%llu\n", Stats->total, Stats->icmp, Stats->udp, Stats->tcp, Stats->ftp, Stats->http, Stats->p2p); 		
		}
	fclose(cdf);
	}

void _StoreIPDataInRam(struct IPData *IPData)
	{
	struct IPDataStore *DataStore;
	struct DataStoreBlock *DataStoreBlock;

	if (!IPDataStore) // we need to create the first entry
		{
		// Allocate Datastore for this IP
	    IPDataStore = malloc(sizeof(struct IPDataStore));
			
		IPDataStore->ip = IPData->ip;
		IPDataStore->Next = NULL;
					
		// Allocate it's first block of storage
		IPDataStore->FirstBlock = malloc(sizeof(struct DataStoreBlock));
		IPDataStore->FirstBlock->LatestTimestamp = 0;

		IPDataStore->FirstBlock->NumEntries = 0;
		IPDataStore->FirstBlock->Data = calloc(IPDATAALLOCCHUNKS, sizeof(struct IPData));
		IPDataStore->FirstBlock->Next = NULL;																		
        if (!IPDataStore->FirstBlock || ! IPDataStore->FirstBlock->Data)
            {
            syslog(LOG_ERR, "无法分配数据存储！正在退出！");
            exit(1);
            }
		}

	DataStore = IPDataStore;

	// Take care of first case
	while (DataStore) // Is not null
		{
		if (DataStore->ip == IPData->ip) // then we have the right store
			{
			DataStoreBlock = DataStore->FirstBlock;

			while(DataStoreBlock) // is not null
				{
				if (DataStoreBlock->NumEntries < IPDATAALLOCCHUNKS) // We have a free spot
					{
					memcpy(&DataStoreBlock->Data[DataStoreBlock->NumEntries++], IPData, sizeof(struct IPData));
					DataStoreBlock->LatestTimestamp = IPData->timestamp;
					return;
					}
        	    else
					{
					if (!DataStoreBlock->Next) // there isn't another block, add one
						{
	                    DataStoreBlock->Next = malloc(sizeof(struct DataStoreBlock));
						DataStoreBlock->Next->LatestTimestamp = 0;
						DataStoreBlock->Next->NumEntries = 0;
						DataStoreBlock->Next->Data = calloc(IPDATAALLOCCHUNKS, sizeof(struct IPData));
						DataStoreBlock->Next->Next = NULL;																				
						}

					DataStoreBlock = DataStoreBlock->Next;
					}
				}						

			return;
			}
		else
			{
			if (!DataStore->Next) // there is no entry for this ip, so lets make one.
				{
				// Allocate Datastore for this IP
    	        DataStore->Next = malloc(sizeof(struct IPDataStore));
				
			    DataStore->Next->ip = IPData->ip;
				DataStore->Next->Next = NULL;
					
				// Allocate it's first block of storage
				DataStore->Next->FirstBlock = malloc(sizeof(struct DataStoreBlock));
				DataStore->Next->FirstBlock->LatestTimestamp = 0;
				DataStore->Next->FirstBlock->NumEntries = 0;
				DataStore->Next->FirstBlock->Data = calloc(IPDATAALLOCCHUNKS, sizeof(struct IPData));
				DataStore->Next->FirstBlock->Next = NULL;																		
				}
	
			DataStore = DataStore->Next;			
			}
		}
	}

void StoreIPDataInRam(struct IPData IncData[])
	{
	unsigned int counter;

    for (counter=0; counter < IpCount; counter++)
		_StoreIPDataInRam(&IncData[counter]);
	}

void CommitData(time_t timestamp)
    {
	static int MayGraph = TRUE;
    unsigned int counter;
	struct stat StatBuf;
	char logname1[] = "/tmp/Bandwidthd_html/htdocs/log.1.5.cdf";
	char logname2[] = "/tmp/Bandwidthd_html/htdocs/log.1.4.cdf";
	// Set the timestamps
	for (counter=0; counter < IpCount; counter++)
        IpTable[counter].timestamp = timestamp;

	// Output modules
	// Only call this from first thread
	if (config.output_database && config.tag == '1')
		StoreIPDataInDatabase(IpTable);

	if (config.output_cdf)
		{
		// TODO: This needs to be moved into the forked section, but I don't want to 
		//	deal with that right now (Heavy disk io may make us drop packets)
		StoreIPDataInCDF(IpTable);

		if (RotateLogs >= config.range/RANGE1) // We set this++ on HUP
			{
			logname1[32] = config.tag;
			logname2[32] = config.tag;
			logname2[34] = '5';

			if (!stat(logname2, &StatBuf)) // File exists
				unlink(logname2);
			logname1[34] = '4';
			if (!stat(logname1, &StatBuf)) // File exists
				rename(logname1, logname2);
			logname1[34] = '3';
			logname2[34] = '4';			
			if (!stat(logname1, &StatBuf)) // File exists
				rename(logname1, logname2);
            logname1[34] = '2';
            logname2[34] = '3';			
			if (!stat(logname1, &StatBuf)) // File exists
				rename(logname1, logname2);
            logname1[34] = '1';
            logname2[34] = '2';			
			if (!stat(logname1, &StatBuf)) // File exists
				rename(logname1, logname2);
            logname1[34] = '0';
            logname2[34] = '1';			
			if (!stat(logname1, &StatBuf)) // File exists
				rename(logname1, logname2); 
			fclose(fopen(logname1, "at")); // Touch file
			RotateLogs = FALSE;
			}
		}

	if (config.graph)
		{
		StoreIPDataInRam(IpTable);

		// Reap a couple zombies
		if (waitpid(-1, NULL, WNOHANG))  // A child was reaped
			MayGraph = TRUE;

		if (GraphIntervalCount%config.skip_intervals == 0 && MayGraph)
			{
			MayGraph = FALSE;
			/* If WriteOutWebpages fails, reenable graphing since there won't
			 * be any children to reap.
			 */
			if (WriteOutWebpages(timestamp))
				MayGraph = TRUE;
			}
		else if (GraphIntervalCount%config.skip_intervals == 0)
			syslog(LOG_INFO, "上次图形绘制未完成... 跳过当前运行");

		DropOldData(timestamp);
		}
    }


int RCDF_Test(char *filename)
	{
	// Determine if the first date in the file is before the cutoff
	// return FALSE on error
	FILE *cdf;
	char ipaddrBuffer[16];
	time_t timestamp;

	if (!(cdf = fopen(filename, "rt"))) 
		return FALSE;
	fseek(cdf, 10, SEEK_END); // fseek to near end of file
	while (fgetc(cdf) != '\n') // rewind to last newline
		{
		if (fseek(cdf, -2, SEEK_CUR) == -1)
			break;
		}
	if(fscanf(cdf, " %15[0-9.],%lu,", ipaddrBuffer, &timestamp) != 2)
		{
		syslog(LOG_ERR, "%s 已损坏，跳过", filename); 
		return FALSE;
		}
	fclose(cdf);
	if (timestamp < time(NULL) - config.range)
		return FALSE; // There is no data in this file from before cutoff
	else
		return TRUE; // This file has data from before cutoff
	}


void RCDF_PositionStream(FILE *cdf)
	{
    time_t timestamp;
	time_t current_timestamp;
	char ipaddrBuffer[16];

	current_timestamp = time(NULL);

	fseek(cdf, 0, SEEK_END);
	timestamp = current_timestamp;
	while (timestamp > current_timestamp - config.range)
		{
		// What happenes if we seek past the beginning of the file?
		if (fseek(cdf, -IP_NUM*75*(config.range/config.interval)/20,SEEK_CUR))
			{ // fseek returned error, just seek to beginning
			fseek(cdf, 0, SEEK_SET);
			return;
			}
		while (fgetc(cdf) != '\n' && !feof(cdf)); // Read to next line
		ungetc('\n', cdf);  // Just so the fscanf mask stays identical
        if(fscanf(cdf, " %15[0-9.],%lu,", ipaddrBuffer, &timestamp) != 2)
			{
			syslog(LOG_ERR, "扫描数据开始位置时发生未知错误...\n");
			return;	
			}
		}
	while (fgetc(cdf) != '\n' && !feof(cdf));
	ungetc('\n', cdf); 
	}

void RCDF_Load(FILE *cdf)
	{
    time_t timestamp;
	time_t current_timestamp = 0;
	struct in_addr ipaddr;
	struct IPData *ip=NULL;
	char ipaddrBuffer[16];
	unsigned long int Counter = 0;
	unsigned long int IntervalsRead = 0;

    for(Counter = 0; !feof(cdf) && !ferror(cdf); Counter++)
	    {
		if(fscanf(cdf, " %15[0-9.],%lu,", ipaddrBuffer, &timestamp) != 2) 
			goto End_RecoverDataFromCdf;

		if (!timestamp) // First run through loop
			current_timestamp = timestamp;

		if (timestamp != current_timestamp)
			{ // Dump to datastore
			StoreIPDataInRam(IpTable);
			IpCount = 0; // Reset Traffic Counters
			current_timestamp = timestamp;
			IntervalsRead++;
			}    		
		inet_aton(ipaddrBuffer, &ipaddr);
		ip = FindIp(ntohl(ipaddr.s_addr));
		ip->timestamp = timestamp;

        if (fscanf(cdf, "%llu,%llu,%llu,%llu,%llu,%llu,%llu,",
            &ip->Send.total, &ip->Send.icmp, &ip->Send.udp,
            &ip->Send.tcp, &ip->Send.ftp, &ip->Send.http, &ip->Send.p2p) != 7
          || fscanf(cdf, "%llu,%llu,%llu,%llu,%llu,%llu,%llu",
            &ip->Receive.total, &ip->Receive.icmp, &ip->Receive.udp,
            &ip->Receive.tcp, &ip->Receive.ftp, &ip->Receive.http, &ip->Receive.p2p) != 7)
			goto End_RecoverDataFromCdf;		
		}

End_RecoverDataFromCdf:
	StoreIPDataInRam(IpTable);
	syslog(LOG_INFO, "已完成恢复 %lu 条记录", Counter);	
	DropOldData(time(NULL)); // Dump the extra data
    if(!feof(cdf))
       syslog(LOG_ERR, "解析日志文件的部分内容失败，放弃该文件");
	IpCount = 0; // Reset traffic counters
    fclose(cdf);
	}

void RecoverDataFromCDF(void)
	{
	FILE *cdf;
	char index[] = "012345";
    char logname1[] = "/tmp/Bandwidthd_html/htdocs/log.1.0.cdf";
    char logname2[] = "/tmp/Bandwidthd_html/htdocs/log.1.1.cdf";
	int Counter;
	int First = FALSE;

	logname1[32] = config.tag;
	logname2[32] = config.tag;

	for (Counter = 5; Counter >= 0; Counter--)
		{
		logname1[34] = index[Counter];
		if (RCDF_Test(logname1))
			break;
		}
	
	First = TRUE;
	for (; Counter >= 0; Counter--)
		{
		logname1[34] = index[Counter];
		if ((cdf = fopen(logname1, "rt")))
			{
			syslog(LOG_INFO, "正在从 %s 恢复", logname1);
			if (First)
				{
				RCDF_PositionStream(cdf);
				First = FALSE;
				}
			RCDF_Load(cdf);
			}
		}
	}

// ****** FindIp **********
// ****** Returns or allocates an Ip's data structure

inline struct IPData *FindIp(uint32_t ipaddr)
    {
    unsigned int counter;
    
    for (counter=0; counter < IpCount; counter++)
        if (IpTable[counter].ip == ipaddr)
            return (&IpTable[counter]);
    
    if (IpCount >= IP_NUM)
        {
        syslog(LOG_ERR, "IP_NUM 值过低，丢弃 IP....");
       	return(NULL);
        }
	
    memset(&IpTable[IpCount], 0, sizeof(struct IPData));

    IpTable[IpCount].ip = ipaddr;
    return (&IpTable[IpCount++]);
    }

size_t ICGrandTotalDataPoints = 0;

__attribute__ ((gnu_inline)) char inline *HostIp2CharIp(unsigned long ipaddr, char *buffer)
    {
	struct in_addr in_addr;
	char *s;

	in_addr.s_addr = htonl(ipaddr);	
    s = inet_ntoa(in_addr);
	strncpy(buffer, s, 16);
	buffer[15] = '\0';
	return(buffer);
/*  uint32_t ip = *(uint32_t *)ipaddr;

	sprintf(buffer, "%d.%d.%d.%d", (ip << 24)  >> 24, (ip << 16) >> 24, (ip << 8) >> 24, (ip << 0) >> 24);
*/
    }

// Add better error checking

int fork2()
    {
    pid_t pid;

    if (!(pid = fork()))
        {
        if (!fork())
        	{
#ifdef PROFILE
				// Got this incantation from a message board.  Don't forget to set
				// GMON_OUT_PREFIX in the shell
				extern void _start(void), etext(void);
				syslog(LOG_INFO, "调用分析器启动...");
				monstartup((u_long) &_start, (u_long) &etext);
#endif
            return(0);
            }        

        _exit(0);
        }
    
    waitpid(pid, NULL, 0);
    return(1);
    }

