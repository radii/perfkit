include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = \
	test-pkd-channels \
	test-pkd-channel \
	test-pk-connection \
	$(NULL)

TEST_PROGS += \
	test-pkd-channels \
	test-pkd-channel \
	test-pk-connection \
	$(NULL)

test_pkd_channel_SOURCES = test-pkd-channel.c
test_pkd_channel_CPPFLAGS = $(PERFKIT_DAEMON_CFLAGS) -I$(top_srcdir)/perfkit-daemon
test_pkd_channel_LDFLAGS = $(PERFKIT_DAEMON_LIBS)
test_pkd_channel_LDADD = $(top_builddir)/perfkit-daemon/libperfkit-daemon.la

test_pkd_channels_SOURCES = test-pkd-channels.c
test_pkd_channels_CPPFLAGS = $(PERFKIT_DAEMON_CFLAGS) -I$(top_srcdir)/perfkit-daemon
test_pkd_channels_LDFLAGS = $(PERFKIT_DAEMON_LIBS)
test_pkd_channels_LDADD = $(top_builddir)/perfkit-daemon/libperfkit-daemon.la

test_pk_connection_SOURCES = test-pk-connection.c
test_pk_connection_CPPFLAGS = $(PERFKIT_CFLAGS) -I$(top_srcdir)/perfkit
test_pk_connection_LDFLAGS = $(PERFKIT_LIBS)
test_pk_connection_LDADD = $(top_builddir)/perfkit/libperfkit-1.0.la
