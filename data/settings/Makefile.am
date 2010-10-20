all: gschemas.compiled

SCHEMAS =					\
	org.perfkit.profiler.gschema.xml	\
	$(NULL)

gschemas.compiled: $(SCHEMAS)
	glib-compile-schemas $(top_srcdir)/data/settings

# TODO: Make this shiz install
