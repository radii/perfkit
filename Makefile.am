SUBDIRS =			\
	data			\
	perfkit-agent		\
	perfkit			\
	tools			\
	uber			\
	perfkit-gui		\
	perfkit-profiler	\
	bindings		\
	tests			\
	doc

ACLOCAL_AMFLAGS = -I build/m4
DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-flags --enable-gtk-doc --enable-introspection

perfkitdocdir = ${prefix}/doc/perfkit
perfkitdoc_DATA = \
	README    \
	COPYING   \
	AUTHORS   \
	ChangeLog \
	INSTALL   \
	NEWS      \
	$(NULL)

EXTRA_DIST = $(perfkitdoc_DATA)

dist-hook:
	@if test -d "$(srcdir)/.git"; then                              \
          (cd "$(srcdir)" &&                                            \
           $(top_srcdir)/missing --run git log --stat ) > ChangeLog.tmp \
           && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog              \
           || (rm -f ChangeLog.tmp;                                     \
               echo Failed to generate ChangeLog >&2);                  \
	else                                                            \
	  echo A git checkout is required to generate a ChangeLog >&2;  \
	fi

uninstall-local:
	-rm -r $(perfkitdocdir)
