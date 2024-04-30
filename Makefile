CC=gcc
CFLAGS=-g -O2 -MMD --std=c11 -Wpedantic -Wall -Iinclude -Igeninc -Isrc `pkg-config --cflags pixman-1 freetype2 xkbcommon wayland-client`

# pkg-config --variable=pkgdatadir wayland-protocols
WAYLAND_PROTOCOLS=/usr/share/wayland-protocols/

SRCDIR=src/
OBJDIR=obj/
LIBDIR=lib/
LIBNAME=fiv
CSOURCES=$(wildcard ${SRCDIR}*.c)
COBJECTS=$(patsubst ${SRCDIR}%.c,${OBJDIR}%.o,${CSOURCES})
GENSOURCES=$(wildcard gensrc/*.c)
GENOBJECTS=$(patsubst gensrc/%.c,${OBJDIR}%o,${GENSOURCES})
DEPS=$(wildcard $(OBJDIR)*.d)
OBJECTS=${COBJECTS} obj/xdg-shell-protocol.o obj/xdg-decoration-unstable-v1-protocol.o
LDFLAGS=-Llib/ -l${LIBNAME} `pkg-config --libs pixman-1 freetype2 xkbcommon wayland-client`

all: appbin/fiv testbin/imageloader_tests

geninc/:
	mkdir -p geninc/

gensrc/:
	mkdir -p gensrc/

${OBJDIR}:
	mkdir -p ${OBJDIR}

${LIBDIR}:
	mkdir -p ${LIBDIR}

appbin/:
	mkdir -p appbin/

testbin/:
	mkdir -p testbin/

gensrc/xdg-shell-protocol.c: ${WAYLAND_PROTOCOLS}stable/xdg-shell/xdg-shell.xml | gensrc/
	wayland-scanner private-code $< $@

geninc/xdg-shell-client-protocol.h: ${WAYLAND_PROTOCOLS}stable/xdg-shell/xdg-shell.xml | geninc/
	wayland-scanner client-header $< $@

gensrc/xdg-decoration-unstable-v1-protocol.c: ${WAYLAND_PROTOCOLS}unstable/xdg-decoration/xdg-decoration-unstable-v1.xml | gensrc/
	wayland-scanner private-code $< $@

geninc/xdg-decoration-unstable-v1-client-protocol.h: ${WAYLAND_PROTOCOLS}unstable/xdg-decoration/xdg-decoration-unstable-v1.xml | geninc/
	wayland-scanner client-header $< $@

${OBJDIR}%.o: gensrc/%.c geninc/xdg-shell-client-protocol.h geninc/xdg-decoration-unstable-v1-client-protocol.h | ${OBJDIR}
	${CC} -c ${CFLAGS} -o $@ $<

${OBJDIR}%.o: ${SRCDIR}%.c geninc/xdg-shell-client-protocol.h gensrc/xdg-shell-protocol.c geninc/xdg-decoration-unstable-v1-client-protocol.h gensrc/xdg-decoration-unstable-v1-protocol.c | ${OBJDIR}
	${CC} -c ${CFLAGS} -o $@ $<

-include $(DEPS)

lib/lib${LIBNAME}.a: ${OBJECTS} | ${LIBDIR}
	ar -rcs $@ ${OBJECTS}

appbin/fiv: appsrc/fiv.c ${LIBDIR}lib${LIBNAME}.a | appbin/
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS}

testbin/%: testsrc/%.c ${LIBDIR}lib${LIBNAME}.a | testbin/
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS}

clean:
	rm -f obj/*.o
	rm -f obj/*.d
	rm -f lib/*.a
	rm -f appbin/*
	rm -f testbin/*
	rm -f gensrc/*.c
	rm -f geninc/*.h

