lib_LTLIBRARIES = libmbdb-1.0.la

libmbdb_1_0_la_SOURCES = \
						mbdb.c \
						mbdb_record.c \
						backup.c \
						backup_file.c
						
libmbdb_1_0_la_LDFLAGS = $(libcrippy_LIBS) $(libcrypto_LIBS)  $(libcrippy_LDFLAGS) $(libcrypto_LDFLAGS)
libmbdb_1_0_la_LIBS = $(libcrippy_LIBS) $(libcrypto_LIBS)
libmbdb_1_0_la_CFLAGS = $(libcrippy_CFLAGS) $(libcrypto_CFLAGS) -I$(top_srcdir)/include

