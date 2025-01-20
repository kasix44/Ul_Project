# Przykładowy Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2

OBJ = main.o hive.o sync.o pszczelarz.o krolowa.o pszczola.o

all: ul

ul: $(OBJ)
	$(CC) $(CFLAGS) -o ul $(OBJ)

main.o: main.c hive.h
	$(CC) $(CFLAGS) -c main.c

hive.o: hive.c hive.h
	$(CC) $(CFLAGS) -c hive.c

sync.o: sync.c hive.h
	$(CC) $(CFLAGS) -c sync.c

pszczelarz.o: pszczelarz.c hive.h
	$(CC) $(CFLAGS) -c pszczelarz.c

krolowa.o: krolowa.c hive.h
	$(CC) $(CFLAGS) -c krolowa.c

pszczola.o: pszczola.c hive.h
	$(CC) $(CFLAGS) -c pszczola.c

clean:
	rm -f *.o ul
