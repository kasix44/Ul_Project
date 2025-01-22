CC     = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

all: ul pszczelarz krolowa pszczola

# Program ul (main.c)
ul: main.o hive.o sync.o
	$(CC) $(CFLAGS) -o ul main.o hive.o sync.o

main.o: main.c hive.h
	$(CC) $(CFLAGS) -c main.c

# Program pszczelarz (pszczelarz.c)
pszczelarz: pszczelarz.o hive.o sync.o
	$(CC) $(CFLAGS) -o pszczelarz pszczelarz.o hive.o sync.o

pszczelarz.o: pszczelarz.c hive.h
	$(CC) $(CFLAGS) -c pszczelarz.c

# Program krolowa (krolowa.c)
krolowa: krolowa.o hive.o sync.o
	$(CC) $(CFLAGS) -o krolowa krolowa.o hive.o sync.o

krolowa.o: krolowa.c hive.h
	$(CC) $(CFLAGS) -c krolowa.c

# Program pszczola (pszczola.c)
pszczola: pszczola.o hive.o sync.o
	$(CC) $(CFLAGS) -o pszczola pszczola.o hive.o sync.o

pszczola.o: pszczola.c hive.h
	$(CC) $(CFLAGS) -c pszczola.c

# Wspólne pliki .o
hive.o: hive.c hive.h
	$(CC) $(CFLAGS) -c hive.c

sync.o: sync.c hive.h
	$(CC) $(CFLAGS) -c sync.c

clean:
	rm -f *.o ul pszczelarz krolowa pszczola
