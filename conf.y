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
    struct in_addr addr, addr2;  
    in_addr_t parsed_ip, parsed_mask;  
      
    // 解析 IP 地址  
    parsed_ip = inet_network($2);  
    if (parsed_ip == (in_addr_t)-1) {  
        fprintf(stderr, "无效的 IP 地址: %s 在第 %d 行\n", $2, LineNo);  
        syslog(LOG_ERR, "无效的 IP 地址: %s 在第 %d 行", $2, LineNo);  
        YYERROR;  
    }  
      
    // 解析子网掩码  
    parsed_mask = inet_network($3);  
    if (parsed_mask == (in_addr_t)-1) {  
        fprintf(stderr, "无效的子网掩码: %s 在第 %d 行\n", $3, LineNo);  
        syslog(LOG_ERR, "无效的子网掩码: %s 在第 %d 行", $3, LineNo);  
        YYERROR;  
    }  
      
    SubnetTable[SubnetCount].ip = parsed_ip & parsed_mask;  
    SubnetTable[SubnetCount].mask = parsed_mask;  
  
    addr.s_addr = htonl(SubnetTable[SubnetCount].ip);  
    addr2.s_addr = htonl(SubnetTable[SubnetCount++].mask);  
    syslog(LOG_INFO, "监控子网 %s 子网掩码 %s", inet_ntoa(addr), inet_ntoa(addr2));  
    }  
    ;

subnetb:  
    TOKSUBNET IPADDR TOKSLASH NUMBER  
    {  
    struct in_addr addr, addr2;  
    in_addr_t parsed_ip;  
    unsigned int Mask;  
      
    // 验证 CIDR 前缀长度  
    if ($4 < 0 || $4 > 32) {  
        fprintf(stderr, "无效的 CIDR 前缀长度 %d (必须在 0-32 之间) 在第 %d 行\n", $4, LineNo);  
        syslog(LOG_ERR, "无效的 CIDR 前缀长度 %d 在第 %d 行", $4, LineNo);  
        YYERROR;  
    }  
      
    // 解析 IP 地址并检查错误  
    parsed_ip = inet_network($2);  
    if (parsed_ip == (in_addr_t)-1) {  
        fprintf(stderr, "无效的 IP 地址: %s 在第 %d 行\n", $2, LineNo);  
        syslog(LOG_ERR, "无效的 IP 地址: %s 在第 %d 行", $2, LineNo);  
        YYERROR;  
    }  
      
    // 生成子网掩码  
    Mask = ($4 == 0) ? 0 : (0xFFFFFFFF << (32 - $4));  
      
    SubnetTable[SubnetCount].mask = Mask;  
    SubnetTable[SubnetCount].ip = parsed_ip & Mask;  
      
    addr.s_addr = htonl(SubnetTable[SubnetCount].ip);  
    addr2.s_addr = htonl(SubnetTable[SubnetCount++].mask);  
    syslog(LOG_INFO, "监控子网 %s 子网掩码 %s", inet_ntoa(addr), inet_ntoa(addr2));  
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
