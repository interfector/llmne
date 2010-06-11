# LLMNE

VERSION = 0.1.6

# includes and libs
LIBS =

# flags
CFLAGS = -Wall -O3 ${LIBS} -DVERSION=\"${VERSION}\" -I./include

SRC = src/main.c src/util.c
BIN = llmne

all:
	gcc -o $(BIN) $(SRC) $(CFLAGS)

clean:
	rm -f $(BIN)

install: all
	cp -f $(BIN) /usr/local/bin/
