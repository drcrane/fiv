CC=gcc
CFLAGS=-g -O2 --std=c11 -Wpedantic -Wall -Iinclude -Igeninc -Isrc `pkg-config --cflags pixman-1 freetype2`

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
OBJECTS=${COBJECTS} obj/xdg-shell-protocol.o obj/zxdg-decoration-unstable-v1.o
LDFLAGS=-Llib/ -l${LIBNAME} -lwayland-client `pkg-config --libs pixman-1 freetype2`

all: appbin/fiv

${OBJDIR}:
	mkdir -p ${OBJDIR}

${LIBDIR}:
	mkdir -p ${LIBDIR}

appbin/:
	mkdir -p appbin/

testbin/:
	mkdir -p testbin/

gensrc/xdg-shell-protocol.c: ${WAYLAND_PROTOCOLS}stable/xdg-shell/xdg-shell.xml
	wayland-scanner private-code $< $@

geninc/xdg-shell-protocol.h: ${WAYLAND_PROTOCOLS}stable/xdg-shell/xdg-shell.xml
	wayland-scanner client-header $< $@

gensrc/zxdg-decoration-unstable-v1.c: ${WAYLAND_PROTOCOLS}unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
	wayland-scanner private-code $< $@

geninc/zxdg-decoration-unstable-v1.h: ${WAYLAND_PROTOCOLS}unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
	wayland-scanner client-header $< $@

${OBJDIR}%.o: gensrc/%.c geninc/%.h | ${OBJDIR}
	${CC} -c ${CFLAGS} -o $@ $<

${OBJDIR}%.o: ${SRCDIR}%.c geninc/xdg-shell-protocol.h gensrc/xdg-shell-protocol.c geninc/zxdg-decoration-unstable-v1.h gensrc/zxdg-decoration-unstable-v1.c | ${OBJDIR}
	${CC} -c ${CFLAGS} -o $@ $<

lib/lib${LIBNAME}.a: ${OBJECTS}
	ar -rcs $@ ${OBJECTS}

appbin/fiv: appsrc/fiv.c ${LIBDIR}lib${LIBNAME}.a | appbin/
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS}

clean:
	rm -f obj/*.o
	rm -f lib/*.a
	rm -f appbin/*
	rm -f gensrc/*.c
	rm -f geninc/*.h

