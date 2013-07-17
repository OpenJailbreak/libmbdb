#!/bin/sh
aclocal
libtoolize
autoheader
autoconf
automake --add-missing
autoreconf -i
./configure
