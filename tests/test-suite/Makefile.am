include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS =							\
	test-suite							\
	$(NULL)

TEST_PROGS +=								\
	test-suite							\
	$(NULL)

AM_CPPFLAGS =								\
	$(PERFKIT_AGENT_CFLAGS)						\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/cut-n-paste					\
	-DAGENT="\"$(top_builddir)/perfkit-agent/perfkit-agent\""	\
	-DAGENT_DIR="\"$(top_builddir)/perfkit-agent\""			\
	$(DBUS_CFLAGS)							\
	$(WARNINGS)							\
	$(NULL)

AM_LDFLAGS =								\
	$(PERFKIT_AGENT_LIBS)						\
	$(DBUS_LIBS)							\
	$(top_builddir)/perfkit-agent/libperfkit-agent.la		\
	$(NULL)

test_suite_SOURCES = test-suite.c
