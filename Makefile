CC = clang
CFLAGS = -m64 -Wall -std=c99
BINARY = png-decoder
INCLUDES = -I include/
LIBDIR = 
LIBRARIES = -l m
TEST = test/*.c
SOURCES = src/*.c

all:
	$(CC) -g -D ZLIB_DEBUG $(CFLAGS) $(INCLUDES) $(LIBDIR) $(SOURCES) \
		$(LIBRARIES) $(TEST) -o bin/$(BINARY)

export:
	$(CC) -O3 -Werror $(CFLAGS) $(INCLUDES) $(LIBDIR) $(SOURCES) \
		$(LIBRARIES) $(TEST) -o bin/$(BINARY)

