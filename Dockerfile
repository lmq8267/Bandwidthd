# ---------------------------
# 构建阶段：编译 PHP 扩展 由8.1改成7.4
# ---------------------------
FROM php:7.4-fpm-alpine AS builder

# 安装编译依赖
RUN apk add --no-cache autoconf make gcc g++ musl-dev freetype-dev libjpeg-turbo-dev libpng-dev postgresql-dev

# 配置并编译 GD、PDO_PGSQL、pgsql
RUN docker-php-ext-configure gd --with-freetype --with-jpeg \
    && docker-php-ext-install gd pdo pdo_pgsql pgsql

# ---------------------------
# 运行阶段：精简镜像 由8.1改成7.4
# ---------------------------
FROM php:7.4-fpm-alpine

# 设置时区
ENV TZ=Asia/Shanghai

# 安装运行时依赖
RUN apk add --no-cache tzdata nginx supervisor postgresql postgresql-client libpq freetype libjpeg-turbo libpng

# 拷贝已编译好的 PHP 扩展和配置
COPY --from=builder /usr/local/lib/php/extensions/ /usr/local/lib/php/extensions/
COPY --from=builder /usr/local/etc/php/conf.d/ /usr/local/etc/php/conf.d/

# 初始化 PostgreSQL 数据目录
RUN mkdir -p /var/lib/postgresql/data /run/postgresql \
    && chown -R postgres:postgres /var/lib/postgresql /run/postgresql

# 写入 Nginx 配置
RUN mkdir -p /etc/nginx/http.d && \
    echo 'server {' > /etc/nginx/http.d/default.conf && \
    echo '    listen 80;' >> /etc/nginx/http.d/default.conf && \
    echo '    server_name localhost;' >> /etc/nginx/http.d/default.conf && \
    echo '    root /var/www/html;' >> /etc/nginx/http.d/default.conf && \
    echo '    index index.php index.html;' >> /etc/nginx/http.d/default.conf && \
    echo '    location / {' >> /etc/nginx/http.d/default.conf && \
    echo '        try_files $uri $uri/ /index.php?$query_string;' >> /etc/nginx/http.d/default.conf && \
    echo '    }' >> /etc/nginx/http.d/default.conf && \
    echo '    location ~ \.php$ {' >> /etc/nginx/http.d/default.conf && \
    echo '        include fastcgi_params;' >> /etc/nginx/http.d/default.conf && \
    echo '        fastcgi_pass 127.0.0.1:9000;' >> /etc/nginx/http.d/default.conf && \
    echo '        fastcgi_index index.php;' >> /etc/nginx/http.d/default.conf && \
    echo '        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;' >> /etc/nginx/http.d/default.conf && \
    echo '    }' >> /etc/nginx/http.d/default.conf && \
    echo '}' >> /etc/nginx/http.d/default.conf

# 写入 Supervisor 配置
RUN echo '[supervisord]' > /etc/supervisord.conf && \  
    echo 'nodaemon=true' >> /etc/supervisord.conf && \  
    echo '' >> /etc/supervisord.conf && \  
    echo '[program:php-fpm]' >> /etc/supervisord.conf && \  
    echo 'command=/usr/local/sbin/php-fpm --nodaemonize' >> /etc/supervisord.conf && \  
    echo 'autostart=true' >> /etc/supervisord.conf && \  
    echo 'autorestart=true' >> /etc/supervisord.conf && \  
    echo '' >> /etc/supervisord.conf && \  
    echo '[program:nginx]' >> /etc/supervisord.conf && \  
    echo 'command=/usr/sbin/nginx -g "daemon off;"' >> /etc/supervisord.conf && \  
    echo 'autostart=true' >> /etc/supervisord.conf && \  
    echo 'autorestart=true' >> /etc/supervisord.conf && \  
    echo '' >> /etc/supervisord.conf && \  
    echo '[program:postgresql]' >> /etc/supervisord.conf && \  
    echo 'command=/usr/bin/postgres -D /var/lib/postgresql/data -c listen_addresses=*' >> /etc/supervisord.conf && \
    echo 'user=postgres' >> /etc/supervisord.conf && \  
    echo 'autostart=true' >> /etc/supervisord.conf && \  
    echo 'autorestart=true' >> /etc/supervisord.conf
    
# 复制 PHP 应用
COPY ./php/ /var/www/html/
WORKDIR /var/www/html
RUN chown -R www-data:www-data /var/www/html && chmod -R 755 /var/www/html

# 复制入口脚本
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# 环境变量
ENV DB_NAME=bandwidthd \
    DB_USER=your_username \
    DB_PASSWORD=your_password \
    INIT_DB=true \
    POSTGRES_LISTEN_ADDRESSES=* 

# 暴露端口
EXPOSE 80 5432

# PostgreSQL 数据卷 你需要挂载到本机目录，防止删除更新镜像数据丢失
VOLUME ["/var/lib/postgresql/data"]

# 启动命令
ENTRYPOINT ["docker-entrypoint.sh"]
