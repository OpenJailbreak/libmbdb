AM_CFLAGS = $(libcrypto_CFLAGS) $(libcrippy_CFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(libcrypto_CFLAGS) $(libcrippy_LIBS)

bin_PROGRAMS=mbdbtool
mbdbtool_SOURCES = mbdbtool.c
					
mbdbtool_CFLAGS = $(AM_CFLAGS)
mbdbtool_LDFLAGS = $(AM_LDFLAGS)
mbdbtool_LDADD = $(top_srcdir)/src/libmbdb-1.0.la

