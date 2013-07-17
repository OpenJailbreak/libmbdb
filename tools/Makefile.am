bin_PROGRAMS=mbdbtool
mbdbtool_SOURCES= \
					mbdbtool.c \
					../src/mbdb.c \
					../src/mbdb_record.c \
					../src/backup.c \
					../src/backup_file.c
					
mbdbtool_CFLAGS = $(libcrypto_CFLAGS) $(libcrippy_CFLAGS) -I$(top_srcdir)/include
mbdbtool_LDFLAGS = $(libcrypto_LIBS) $(libcrippy_LIBS)
mbdbtool_LIBS = $(top_srcdir)/src/libmbdb-1.0.la

