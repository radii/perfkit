include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS =							\
	test-pk-connection						\
	$(NULL)

TEST_PROGS +=								\
	test-pk-connection						\
	$(NULL)

AM_CPPFLAGS =								\
	$(PERFKIT_CFLAGS)						\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	-DPK_PROTOCOLS_DIR="\"$(top_builddir)/perfkit/protocols/.libs\""\
	$(NULL)

AM_LDFLAGS =								\
	$(PERFKIT_LIBS)							\
	$(top_builddir)/perfkit/libperfkit-1.0.la			\
	$(NULL)

test_pk_connection_SOURCES = test-pk-connection.c
