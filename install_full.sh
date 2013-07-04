#!/bin/bash

# Performs the full installation of the PHP extension

phpize \
&& aclocal \
&& libtoolize --force \
&& autoheader \
&& autoconf \
&& ./configure \
&& make \
&& make install

echo "Xdebug installed! Don't forget to restart any Web server you might be running"
