LT_OBJDIR = .libs

WARNINGS = -Wall -Werror -Wold-style-definition -Wdeclaration-after-statement \
	-Wredundant-decls -Wmissing-noreturn -Wshadow -Wcast-align -Wwrite-strings \
	-Winline -Wformat-nonliteral -Wformat-security -Wswitch-enum -Wswitch-default \
	-Winit-self -Wmissing-include-dirs -Wundef -Waggregate-return \
	-Wmissing-format-attribute -Wnested-externs

INCLUDES = \
	-I$(top_srcdir) \
	$(PERFKIT_DAEMON_CFLAGS) \
	$(WARNINGS) \
	$(NULL)

LDFLAGS = \
	-avoid-version \
	-module \
	$(NULL)

PLUGIN_FILES = \
	$(builddir)/$(LT_OBJDIR)/memory.pkd-plugin \
	$(NULL)

plugindir = $(libdir)/perfkit-daemon/plugins/
plugin_DATA = $(PLUGIN_FILES)
plugin_LTLIBRARIES = libmemory.la

libmemory_la_SOURCES = memory.c

$(PLUGIN_FILES):
	@mkdir -p $(builddir)/$(LT_OBJDIR)
	@cp $(srcdir)/memory.pkd-plugin $(builddir)/$(LT_OBJDIR)/memory.pkd-plugin

CLEANFILES = $(PLUGIN_FILES)
