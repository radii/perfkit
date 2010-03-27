include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS =							\
	test-pkd-log							\
	test-pkd-plugins						\
	test-pkd-sample							\
	test-pkd-manifest						\
	test-pkd-encoder						\
	test-pkd-source-simple						\
	$(NULL)

TEST_PROGS +=								\
	test-pkd-log							\
	test-pkd-plugins						\
	test-pkd-sample							\
	test-pkd-manifest						\
	test-pkd-encoder						\
	test-pkd-source-simple						\
	$(NULL)

AM_CPPFLAGS =								\
	$(PERFKIT_DAEMON_CFLAGS)					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(NULL)

AM_LDFLAGS =								\
	$(PERFKIT_DAEMON_LIBS)						\
	$(top_builddir)/perfkit-daemon/libperfkit-daemon.la		\
	$(NULL)

test_pkd_log_SOURCES = test-pkd-log.c
test_pkd_plugins_SOURCES = test-pkd-plugins.c
test_pkd_sample_SOURCES = test-pkd-sample.c $(top_srcdir)/cut-n-paste/egg-buffer.c
test_pkd_manifest_SOURCES = test-pkd-manifest.c
test_pkd_encoder_SOURCES = test-pkd-encoder.c
test_pkd_source_simple_SOURCES = test-pkd-source-simple.c
