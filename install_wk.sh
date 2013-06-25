phpize || exit $?
aclocal || exit $?
libtoolize --force || exit $?
autoheader || exit $?
autoconf || exit $?
./configure || exit $?
make || exit $?

# cd Dropbox/xdebug/
# ./install_wk.sh && make install && /etc/init.d/apache2 restart
