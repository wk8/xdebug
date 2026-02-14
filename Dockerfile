# d build . -t xdebug_dev && d run -v $(pwd):/app --name xdebug_dev -it xdebug_dev
# d rm -f xdebug_dev

FROM ubuntu:24.04 AS php56-base

WORKDIR /app

RUN apt update \
    && apt install -y software-properties-common \
    && add-apt-repository ppa:ondrej/php \
    && apt update \
    && apt install -y php5.6 \
      php5.6-cli      \
      php5.6-common \
      php5.6-dev \
      autoconf automake libtool pkg-config

# then
# phpize && ./configure --enable-xdebug --with-php-config=$(which php-config) && make -j 4 && make install && echo 'done'
# php -dzend_extension="$(php-config --extension-dir)/xdebug.so" wk.php
