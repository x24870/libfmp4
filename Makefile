CC = gcc
OS=$(shell uname | tr '[:upper:]' '[:lower:]')

LIB_ARCHIVE_NAME = libfmp4.a

CFLAGS = -Wall -O3 -flto -fPIC
ifeq ($(OS),linux)
	CFLAGS += -s
	CFLAGS += -I/usr/include/openssl
else ifeq ($(OS),darwin)
	CFLAGS += -I/usr/local/opt/openssl/include
	CFLAGS += -I/opt/homebrew/include
	CFLAGS += -I/opt/homebrew/Cellar/openssl@1.1/1.1.1k/include
endif
CFLAGS += -I./cJSON

OBJS = fmp4.o \
	   transport.o \
	   cJSON/cJSON.o \
	   websocket.o \
	   evowebsocket.o


.PHONY: all static clean

all: static

static: $(OBJS)
	ar rcs $(LIB_ARCHIVE_NAME) $(OBJS)
	ranlib $(LIB_ARCHIVE_NAME)

%.o: %.c %.h
	$(CC) -c -o $@ $(CFLAGS) $< $(LDFLAGS)

clean:
	rm -f $(LIB_ARCHIVE_NAME) $(OBJS)


