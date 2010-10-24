noinst_LTLIBRARIES = libuber.la

WARNINGS =								\
	-Wall								\
	-Werror								\
	-Wold-style-definition						\
	-Wdeclaration-after-statement					\
	-Wredundant-decls						\
	-Wmissing-noreturn						\
	-Wcast-align							\
	-Wwrite-strings							\
	-Winline							\
	-Wformat-nonliteral						\
	-Wformat-security						\
	-Wswitch-enum							\
	-Wswitch-default						\
	-Winit-self							\
	-Wmissing-include-dirs						\
	-Wundef								\
	-Waggregate-return						\
	-Wmissing-format-attribute					\
	-Wnested-externs						\
	-Wshadow

NOINST_H_FILES =							\
	g-ring.h							\
	uber.h								\
	uber-frame-source.h						\
	uber-graph.h							\
	uber-heat-map.h							\
	uber-label.h							\
	uber-line-graph.h						\
	uber-range.h							\
	uber-scale.h							\
	uber-scatter.h							\
	uber-timeout-interval.h						\
	$(NULL)

libuber_la_SOURCES =							\
	$(NOINST_H_FILES)						\
	g-ring.c							\
	uber-frame-source.c						\
	uber-graph.c							\
	uber-heat-map.c							\
	uber-label.c							\
	uber-line-graph.c						\
	uber-range.c							\
	uber-scale.c							\
	uber-scatter.c							\
	uber-timeout-interval.c						\
	$(NULL)

libuber_la_CPPFLAGS =							\
	-DG_LOG_DOMAIN=\"Uber\"						\
	$(WARNINGS)							\
	$(GTK_CFLAGS)							\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(NULL)
