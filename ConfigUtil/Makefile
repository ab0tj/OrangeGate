C=g++
CFLAGS=-c -Wall
OBJS=main.o ini.o config.o mcu.o beacon.o util.o

all: orangegate

install: all
	/usr/bin/install -D --mode=755 orangegate /usr/local/bin/orangegate

orangegate: $(OBJS)
	$(C) $(OBJS) -lwiringPi -lcrypt -lrt -pthread -o orangegate

main.o: main.cpp
	$(C) $(CFLAGS) -o main.o main.cpp

config.o: config.cpp
	$(C) $(CFLAGS) -o config.o config.cpp

mcu.o: mcu.cpp
	$(C) $(CFLAGS) -o mcu.o mcu.cpp

beacon.o: beacon.cpp
	$(C) $(CFLAGS) -o beacon.o beacon.cpp

util.o: util.cpp
	$(C) $(CFLAGS) -o util.o util.cpp

clean:
	rm -f orangegate $(OBJS)
