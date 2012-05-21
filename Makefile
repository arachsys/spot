PREFIX =
BINDIR = ${PREFIX}/bin
DESTDIR =

CC = gcc
CFLAGS = -g -std=gnu99 -Os -Wall -Wno-pointer-sign
LDFLAGS = -Wl,--as-needed,--gc-sections -pthread
LDLIBS = -lcrypto -ldl -lm -lresolv -lz

despotify/%.o: CFLAGS = -g -std=gnu99 -Os -fdata-sections -ffunction-sections

spotcat: spotcat.o $(patsubst %.c, %.o, $(wildcard despotify/*.c))

clean:
	rm -f -- spotcat tags *.o despotify/*.o

install: spotcat
	mkdir -p ${DESTDIR}${BINDIR}
	install -m 0755 -s $< ${DESTDIR}${BINDIR}

tags:
	ctags -R

.PHONY: clean install tags
