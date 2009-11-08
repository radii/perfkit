include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = test-pkd-channels
TEST_PROGS += test-pkd-channels
INCLUDES = -I$(top_srcdir)/perfkit-daemon
LDADD = $(top_builddir)/perfkit-daemon/libperfkit-daemon.la
AM_CFLAGS = $(PERFKIT_DAEMON_CFLAGS)
AM_LDFLAGS = $(PERFKIT_DAEMON_LIBS)

test_pkd_channels_SOURCES = test-pkd-channels.c
