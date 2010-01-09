include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS =							\
	test-suite							\
	$(NULL)

TEST_PROGS +=								\
	test-suite							\
	$(NULL)

AM_CPPFLAGS =								\
	$(PERFKIT_DAEMON_CFLAGS)					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/cut-n-paste					\
	-DDAEMON="\"$(top_builddir)/perfkit-daemon/perfkit-daemon\""	\
	-DDAEMON_DIR="\"$(top_builddir)/perfkit-daemon\""		\
	$(DBUS_CFLAGS)							\
	$(WARNINGS)							\
	$(NULL)

AM_LDFLAGS =								\
	$(PERFKIT_DAEMON_LIBS)						\
	$(DBUS_LIBS)							\
	$(top_builddir)/perfkit-daemon/libperfkit-daemon.la		\
	$(NULL)

test_suite_SOURCES = test-suite.c
