#!/bin/sh
aclocal
glibtoolize
autoheader
autoconf
automake --add-missing
autoreconf -i
./configure
