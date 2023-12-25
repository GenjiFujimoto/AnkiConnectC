P = libankiconnectc.so
SRC = ankiconnectc.c
OBJ = $(SRC:.c=.o)

PREFIX = /usr

PKGCONFIG = $(shell which pkg-config)
GLIB_FLAGS = $(shell $(PKGCONFIG) --cflags glib-2.0)
GLIB_LIBS = $(shell $(PKGCONFIG) --libs glib-2.0)

CFLAGS   = -std=gnu11 -O3 -g -pedantic -Wall $(GLIB_FLAGS) -shared -fPIC 
LDFLAGS  = $(GLIB_LIBS) -lcurl -g -shared -fPIC

all: options $(P)

options:
	@echo popup build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

.c.o:
	$(CC) -c $(CFLAGS) $<

$(P): ${OBJ}
	$(CC) -o $(P) ${OBJ} ${LDFLAGS}

clean:
	rm -f $(P) ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/local/include
	mkdir -p ${DESTDIR}${PREFIX}/local/lib
	cp -f $(P) ${DESTDIR}${PREFIX}/lib
	cp -f ankiconnectc.h ${DESTDIR}${PREFIX}/local/include

uninstall:
	rm -f ${DESTDIR}${PREFIX}/lib/$(P)
	rm -f ${DESTDIR}${PREFIX}/local/include/ankiconnectc.h

.PHONY: all options clean install uninstall
