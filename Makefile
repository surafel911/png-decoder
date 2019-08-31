CC = clang
CFLAGS = -m64 -Wall -Werror -std=c99
BINARY = png-decoder
INCLUDES = -I include/
LIBDIR = 
LIBRARIES = -l m
TEST = test/*.c
SOURCES = src/*.c

all:
	$(CC) -g $(CFLAGS) $(INCLUDES) $(LIBDIR) $(SOURCES) \
		$(LIBRARIES) $(TEST) -o bin/$(BINARY)

export:
	$(CC) -O3 $(CFLAGS) $(INCLUDES) $(LIBDIR) $(SOURCES) \
		$(LIBRARIES) $(TEST) -o bin/$(BINARY)

