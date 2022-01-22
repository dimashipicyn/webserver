# Basic nginx
# docker build -t nginx .
# docker run -it --rm -p 8080:80 --name web nginx
FROM nginx:latest
RUN chmod 777 /var/www
COPY ./nginx.conf /etc/nginx/conf.d/default.conf
COPY ./index.html /var/www/index.html
