#bin_PROGRAMS = perfkit-shell perfkit-support
bin_PROGRAMS = perfkit-shell


WARNINGS =								\
	-Wall -Werror -Wold-style-definition				\
	-Wdeclaration-after-statement					\
	-Wredundant-decls -Wmissing-noreturn -Wshadow -Wcast-align	\
	-Wwrite-strings -Winline -Wformat-nonliteral -Wformat-security	\
	-Wswitch-enum -Wswitch-default -Winit-self			\
	-Wmissing-include-dirs -Wundef -Waggregate-return		\
	-Wmissing-format-attribute -Wnested-externs			\
	$(NULL)


AM_CPPFLAGS =								\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DG_LOG_DOMAIN=\"PerfkitTools\"					\
	-DGETTEXT_PACKAGE=\"PerfkitTools\"				\
	-DLOCALEDIR=\""$(localedir)"\"					\
	-I$(top_srcdir)/cut-n-paste					\
	-I$(top_srcdir)							\
	$(PERFKIT_TOOLS_CFLAGS)						\
	$(WARNINGS)							\
	$(NULL)


#
# perfkit-shell
#

perfkit_shell_SOURCES =							\
	$(top_srcdir)/cut-n-paste/egg-fmt.c				\
	$(top_srcdir)/cut-n-paste/egg-line.c				\
	perfkit-shell.c							\
	$(NULL)

perfkit_shell_LDFLAGS =							\
	$(PERFKIT_TOOLS_LIBS)						\
	-lreadline							\
	$(NULL)

perfkit_shell_LDADD = $(top_builddir)/perfkit/libperfkit-1.0.la

test-shell:
	PK_CONNECTIONS_DIR=$(top_builddir)/perfkit/connections/.libs ./perfkit-shell


#
# perfkit-support
#

#
#perfkit_support_SOURCES =						\
#	perfkit-support.c						\
#	$(NULL)
#
#perfkit_support_LDFLAGS =						\
#	$(PERFKIT_TOOLS_LIBS)						\
#	-lreadline							\
#	$(NULL)
#
#perfkit_support_LDADD = $(top_builddir)/perfkit/libperfkit-1.0.la
#
