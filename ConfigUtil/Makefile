C=g++
CFLAGS=-c -Wall
OBJS=main.o ini.o config.o

all: orangegate

install: all
	/usr/bin/install -D --mode=755 orangegate /usr/local/bin/orangegate

orangegate: $(OBJS)
	$(C) $(OBJS) -lwiringPi -o orangegate

main.o: main.c
	$(C) $(CFLAGS) -o main.o main.c

config.o: config.c
	$(C) $(CFLAGS) -o config.o config.c

clean:
	rm -f orangegate $(OBJS)
