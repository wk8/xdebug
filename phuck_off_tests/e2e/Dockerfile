FROM ubuntu:24.04 AS php56-e2e

WORKDIR /app

RUN apt update \
    && apt install -y software-properties-common \
    && add-apt-repository ppa:ondrej/php \
    && apt update \
    && apt install -y php5.6 \
      php5.6-cli \
      php5.6-common \
      php5.6-dev \
      autoconf automake libtool pkg-config \
    && rm -rf /var/lib/apt/lists/*

COPY . /app
COPY phuck_off_tests/e2e/complex_runtime_funcs.txt /etc/funcs.txt

RUN phpize \
    && ./configure --enable-xdebug --with-php-config="$(which php-config)" \
    && make -j 4 \
    && make install

CMD ["/bin/sh", "/app/phuck_off_tests/e2e/run_complex_runtime_e2e.sh"]
