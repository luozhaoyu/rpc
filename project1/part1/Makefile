CC=cc
CFLAGS=-c -g -Wall -Wextra -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_REENTRANT $(INCLUDE)
INCLUDE=
LDFLAGS=
LIBS=-lrt

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
VENDOR_OBJS=vendor/vendor.o
EXECUTABLE=server


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

$(OBJECTS):
	$(CC) $(CFLAGS) $(SOURCES)

.PHONY: clean all
clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(OBJECTS)
	rm -rf *.so

net: clean
	$(CC) -lrt -D SERVER_MAIN -o server server.c utils.c
	$(CC) -lrt -D CLIENT_MAIN -o client client.c utils.c

default:
