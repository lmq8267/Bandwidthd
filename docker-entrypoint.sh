#!/bin/sh  
set -e  

log_info() {  
    echo "[INFO] $1"  
}  

log_warn() {  
    echo "[WARN] $1"  
}  

# 环境变量默认值  
DB_NAME=${DB_NAME:-bandwidthd}  
DB_USER=${DB_USER:-your_username}  
DB_PASSWORD=${DB_PASSWORD:-your_password}  
DB_HOST=${DB_HOST:-127.0.0.1}  # 如果你只需要前端，不需要数据库，可以设置此环境变量为外部数据库主机地址  
DB_PORT=${DB_PORT:-5432}        # 如果你只需要前端，不需要数据库，可以设置此环境变量为外部数据库主机端口
POSTGRES_LISTEN_ADDRESSES=${POSTGRES_LISTEN_ADDRESSES:-*}  
INIT_FLAG_FILE="/var/lib/postgresql/data/.initialized"

log_info "=== 数据库配置 ==="  
log_info "数据库名: $DB_NAME"  
log_info "数据账户: $DB_USER"  
log_info "数据密码: $DB_PASSWORD"  
# log_info "数据库监听地址: $POSTGRES_LISTEN_ADDRESSES" 
log_info "数据库主机: $DB_HOST"  
log_info "数据库端口: $DB_PORT"  

# 生成配置内容 
CONFIG_CONTENT="<?php  
if (!isset(\$config_conf_key)) {  
    die(\"Error\");  
}  
  
define(\"DFLT_WIDTH\", 900);  
define(\"DFLT_HEIGHT\", 256);  
define(\"DFLT_INTERVAL\", 172800);  
  
\$db_connect_string = \"host=$DB_HOST port=$DB_PORT dbname=$DB_NAME user=$DB_USER password=$DB_PASSWORD\";  
?>"  
  
# 检查配置文件是否存在且内容相同  
if [ -f /var/www/html/config.php ]; then  
    CURRENT_CONFIG=$(cat /var/www/html/config.php)  
    if [ "$CURRENT_CONFIG" = "$CONFIG_CONTENT" ]; then  
        log_info "config.php 配置已存在"  
    else  
        log_info "配置已变化,更新 config.php..."  
        echo "$CONFIG_CONTENT" > /var/www/html/config.php  
        chown www-data:www-data /var/www/html/config.php  
        chmod 644 /var/www/html/config.php  
    fi  
else  
    log_info "首次生成 config.php..."  
    echo "$CONFIG_CONTENT" > /var/www/html/config.php  
    chown www-data:www-data /var/www/html/config.php  
    chmod 644 /var/www/html/config.php  
fi
    
# 初始化 PostgreSQL 数据目录（仅首次启动）
if [ ! -f "$INIT_FLAG_FILE" ]; then  

    log_info "首次初始化 PostgreSQL 数据目录..."  
    mkdir -p /var/lib/postgresql/data  
    chown -R postgres:postgres /var/lib/postgresql  

    su - postgres -c "/usr/bin/initdb -D /var/lib/postgresql/data"  

    echo "listen_addresses = '$POSTGRES_LISTEN_ADDRESSES'" >> /var/lib/postgresql/data/postgresql.conf  
    echo "host all all 0.0.0.0/0 md5" >> /var/lib/postgresql/data/pg_hba.conf  

    log_info "PostgreSQL 初始化完成"  
    touch "$INIT_FLAG_FILE"
fi  

# 启动临时 PostgreSQL 进程用于初始化数据库
log_info "启动临时 PostgreSQL 进行初始化..."  
su - postgres -c "/usr/bin/pg_ctl -D /var/lib/postgresql/data -o \"-F -k /run/postgresql\" -w start"

# 等待 PostgreSQL 可用
until su - postgres -c "psql -c '\q'" >/dev/null 2>&1; do
    echo "[INFO] 等待 PostgreSQL 启动..."
    sleep 1
done

# 创建数据库用户和数据库（仅首次）
if [ "$INIT_DB" = "true" ]; then  
    log_info "创建数据库用户和数据库..."  
      
    su - postgres -c "psql -tc \"SELECT 1 FROM pg_roles WHERE rolname='$DB_USER';\"" | grep -q 1 || \
        su - postgres -c "psql -c \"CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';\"" || log_warn "创建用户失败"

    su - postgres -c "psql -tc \"SELECT 1 FROM pg_database WHERE datname='$DB_NAME';\"" | grep -q 1 || \
        su - postgres -c "psql -c \"CREATE DATABASE $DB_NAME OWNER $DB_USER;\"" || log_warn "创建数据库失败"

    # 检查表是否存在
    table_exists=$(su - postgres -c "psql -d $DB_NAME -tAc \"SELECT COUNT(*) FROM information_schema.tables WHERE table_name='sensors';\"")
    if [ "$table_exists" -eq "0" ]; then  
        log_info "建立数据库表 schema..."  
        su - postgres -c "psql -d $DB_NAME < /var/www/html/schema.postgresql"  
        log_info "数据库表创建成功" 
          
        log_info "授予数据库权限..."  
        su - postgres -c "psql -d $DB_NAME -c \"GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO $DB_USER;\""  
        su - postgres -c "psql -d $DB_NAME -c \"GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO $DB_USER;\""  
        # log_info "权限授予成功"
    else  
        log_info "数据库表已存在,跳过"  
    fi  
fi  

# 停止临时 PostgreSQL
log_info "停止临时 PostgreSQL..."  
su - postgres -c "/usr/bin/pg_ctl -D /var/lib/postgresql/data -m fast stop"

log_info "=== 启动服务 ==="  
exec /usr/bin/supervisord -c /etc/supervisord.conf
