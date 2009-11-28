LT_OBJDIR = .libs
plugin_LTLIBRARIES = libmemory.la

WARNINGS = -Wall -Werror -Wold-style-definition -Wdeclaration-after-statement \
	-Wredundant-decls -Wmissing-noreturn -Wshadow -Wcast-align -Wwrite-strings \
	-Winline -Wformat-nonliteral -Wformat-security -Wswitch-enum -Wswitch-default \
	-Winit-self -Wmissing-include-dirs -Wundef -Waggregate-return \
	-Wmissing-format-attribute -Wnested-externs

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(PERFKIT_DAEMON_CFLAGS) \
	$(WARNINGS) \
	$(NULL)

plugindir = $(libdir)/perfkit-daemon/plugins/
plugin_DATA = $(PLUGIN_FILES)

libmemory_la_SOURCES = memory.c
libmemory_la_LDFALGS = -module -avoid-version
libmemory_la_LIBADD = $(PERFKIT_DAEMON_LIBS)

PLUGIN_FILES = \
	$(builddir)/$(LT_OBJDIR)/memory.pkd-plugin \
	$(NULL)

$(PLUGIN_FILES):
	@mkdir -p $(builddir)/$(LT_OBJDIR)
	@cp $(srcdir)/memory.pkd-plugin $(builddir)/$(LT_OBJDIR)/memory.pkd-plugin

CLEANFILES = $(PLUGIN_FILES)
