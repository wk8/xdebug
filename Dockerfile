# d build . -t xdebug_dev && d run -v $(pwd):/app --name xdebug_dev -it xdebug_dev
# d rm -f xdebug_dev
# d exec -it xdebug_dev bash

FROM wk88/php-func-finder:404ef8b10798 AS func-finder

WORKDIR /app

COPY wk.php .
COPY po.php .

RUN /func_finder/dump_funcs.php . --target /tmp/funcs.txt

###

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

COPY --from=func-finder /tmp/funcs.txt /etc/funcs.txt

# then
# phpize && ./configure --enable-xdebug --with-php-config=$(which php-config) && make -j 4 && make install && echo 'done'
# PHUCK_OFF_ENABLED=1 php -dzend_extension="$(php-config --extension-dir)/xdebug.so" po.php
