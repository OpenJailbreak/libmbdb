AM_CFLAGS = $(libcrypto_CFLAGS) $(libcrippy_CFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(libcrypto_LIBS) $(libcrippy_LIBS)

bin_PROGRAMS=mbdbtool mbdbtool2

mbdbtool_SOURCES = mbdbtool.c
					
mbdbtool_CFLAGS = $(AM_CFLAGS)
mbdbtool_LDFLAGS = $(AM_LDFLAGS)
mbdbtool_LDADD = $(top_srcdir)/src/libmbdb-1.0.la

mbdbtool2_SOURCES = mbdbtool2.c
mbdbtool2_CFLAGS = $(AM_CFLAGS)
mbdbtool2_LDFLAGS = $(AM_LDFLAGS)
mbdbtool2_LDADD = $(top_srcdir)/src/libmbdb-1.0.la
