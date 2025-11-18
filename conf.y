 %{
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bandwidthd.h"

extern unsigned int SubnetCount;
extern struct SubnetData SubnetTable[];
extern struct config config;

int bdconfig_lex(void);
int LineNo = 1;

void bdconfig_error(const char *str)
    {
    fprintf(stderr, "语法错误 \"%s\" 在第 %d 行\n", str, LineNo);
	syslog(LOG_ERR, "语法错误 \"%s\" 在第 %d 行", str, LineNo);
	exit(1);
    }

int bdconfig_wrap()
	{
	return(1);
	}
%}

%token TOKJUNK TOKSUBNET TOKDEV TOKSLASH TOKSKIPINTERVALS TOKGRAPHCUTOFF 
%token TOKPROMISC TOKOUTPUTCDF TOKRECOVERCDF TOKGRAPH TOKNEWLINE TOKFILTER
%token TOKMETAREFRESH TOKPGSQLCONNECTSTRING TOKSENSORID
%token TOKSQLITEFILENAME
%union
{
    int number;
    char *string;
}

%token <string> IPADDR
%token <number> NUMBER
%token <string> STRING
%token <number> STATE
%type <string> string
%%

commands: /* EMPTY */
    | commands command
    ;

command:
	subnet
	|
	device
	|
	skip_intervals
	|
	graph_cutoff
	|
	promisc
	|
	output_cdf
	|
	recover_cdf
	|
	graph
	|
	newline
	|
	filter
	|
	meta_refresh
	|
	pgsql_connect_string
	|
	sqlite_filename
 	|
	sensor_id
	;

subnet:
	subneta
	|
	subnetb
	;

newline:
	TOKNEWLINE
	{
	LineNo++;
	}
	;

subneta:
    TOKSUBNET IPADDR IPADDR
    {
        struct in_addr ip_struct, mask_struct;
        unsigned int parsed_ip, parsed_mask;
        char ipbuf[INET_ADDRSTRLEN];
        char maskbuf[INET_ADDRSTRLEN];

        // 解析子网 IP
        if (inet_pton(AF_INET, $2, &ip_struct) != 1) {
            syslog(LOG_ERR, "无效的子网 IP 地址: %s", $2);
            YYERROR;
        }

        // 解析子网掩码
        if (inet_pton(AF_INET, $3, &mask_struct) != 1) {
            syslog(LOG_ERR, "无效的子网掩码: %s", $3);
            YYERROR;
        }

        // 转换为主机字节序
        parsed_ip = ntohl(ip_struct.s_addr);
        parsed_mask = ntohl(mask_struct.s_addr);


        // 写入表（ip = network address）
        SubnetTable[SubnetCount].ip = parsed_ip & parsed_mask;
        SubnetTable[SubnetCount].mask = parsed_mask;

        // 使用 inet_ntop 将网络序值格式化到独立缓冲区，避免静态缓冲被覆盖
        {
            struct in_addr tmp;
            tmp.s_addr = htonl(SubnetTable[SubnetCount].ip);
            if (inet_ntop(AF_INET, &tmp, ipbuf, sizeof(ipbuf)) == NULL) {
                strncpy(ipbuf, "UNKNOWN", sizeof(ipbuf)); ipbuf[sizeof(ipbuf)-1] = '\0';
            }
            tmp.s_addr = htonl(SubnetTable[SubnetCount].mask);
            if (inet_ntop(AF_INET, &tmp, maskbuf, sizeof(maskbuf)) == NULL) {
                strncpy(maskbuf, "UNKNOWN", sizeof(maskbuf)); maskbuf[sizeof(maskbuf)-1] = '\0';
            }
        }

        // 使用独立缓冲区的字符串写入日志（安全）
        syslog(LOG_INFO, "监控子网 %s 子网掩码 %s", ipbuf, maskbuf);

        // 完成后再自增
        SubnetCount++;
    }
    ;

subnetb:
    TOKSUBNET IPADDR TOKSLASH NUMBER
    {
        struct in_addr ip_struct;
        unsigned int parsed_ip, Mask;
        char ipbuf[INET_ADDRSTRLEN];
        char maskbuf[INET_ADDRSTRLEN];

        // 检查 CIDR 前缀
        if ($4 < 0 || $4 > 32) {
            syslog(LOG_ERR, "无效的 CIDR 前缀长度 %d (必须在 0-32 之间)", $4);
            YYERROR;
        }

        // 解析 IP 地址
        if (inet_pton(AF_INET, $2, &ip_struct) != 1) {
            syslog(LOG_ERR, "无效的 IP 地址: %s", $2);
            YYERROR;
        }

        parsed_ip = ntohl(ip_struct.s_addr);
        Mask = ($4 == 0) ? 0 : (0xFFFFFFFF << (32 - $4));

        // 写入表（注意顺序：先写入再打印）
        SubnetTable[SubnetCount].mask = Mask;
        SubnetTable[SubnetCount].ip = parsed_ip & Mask;

        // 使用 inet_ntop 输出到独立缓冲区
        {
            struct in_addr tmp;
            tmp.s_addr = htonl(SubnetTable[SubnetCount].ip);
            if (inet_ntop(AF_INET, &tmp, ipbuf, sizeof(ipbuf)) == NULL) {
                strncpy(ipbuf, "UNKNOWN", sizeof(ipbuf)); ipbuf[sizeof(ipbuf)-1] = '\0';
            }
            tmp.s_addr = htonl(SubnetTable[SubnetCount].mask);
            if (inet_ntop(AF_INET, &tmp, maskbuf, sizeof(maskbuf)) == NULL) {
                strncpy(maskbuf, "UNKNOWN", sizeof(maskbuf)); maskbuf[sizeof(maskbuf)-1] = '\0';
            }
        }

        syslog(LOG_INFO, "监控子网 %s 子网掩码 %s", ipbuf, maskbuf);

        // 完成后再自增
        SubnetCount++;
    }
    ;
string:
    STRING
    {
    $1[strlen($1)-1] = '\0';
    $$ = $1+1;
    }
    ;

device:
	TOKDEV string
	{
	config.dev = $2;
	}
	;

filter:
	TOKFILTER string
	{
	config.filter = $2;
	}
	;

meta_refresh:
	TOKMETAREFRESH NUMBER
	{
	config.meta_refresh = $2;
	}
	;

skip_intervals:
	TOKSKIPINTERVALS NUMBER
	{
	config.skip_intervals = $2+1;
	}
	;

graph_cutoff:
	TOKGRAPHCUTOFF NUMBER
	{
	config.graph_cutoff = $2*1024;
	}
	;

promisc:
	TOKPROMISC STATE
	{
	config.promisc = $2;
	}
	;

output_cdf:
	TOKOUTPUTCDF STATE
	{
	config.output_cdf = $2;
	}
	;

recover_cdf:
	TOKRECOVERCDF STATE
	{
	config.recover_cdf = $2;
	}
	;

graph:
	TOKGRAPH STATE
	{
	config.graph = $2;
	}
	;

pgsql_connect_string:
    TOKPGSQLCONNECTSTRING string
    {
    config.db_connect_string = $2;
	config.output_database = DB_PGSQL;
    }
    ;

sqlite_filename:
    TOKSQLITEFILENAME string
    {
    config.db_connect_string = $2;
    config.output_database = DB_SQLITE;
    }
    ;
 
sensor_id:
    TOKSENSORID string
    {
    config.sensor_id = $2;
    }
    ;
