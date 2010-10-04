# LLMNE

VERSION = 0.3.5

# includes and libs
LIBS =

# flags
CFLAGS = -Wall -O3 ${LIBS} -DVERSION=\"${VERSION}\" -I./include

SRC = src/main.c src/util.c
BIN = llmne

all:
	gcc -o $(BIN) $(SRC) $(CFLAGS)
	make -C example all

clean:
	make -C example clean 
	rm -f $(BIN)

install: all
	cp -f $(BIN) /usr/local/bin/
