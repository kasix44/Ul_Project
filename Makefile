CC     = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
TARGET = ul

SRC = main.c hive.c sync.c \
      pszczelarz.c krolowa.c pszczola.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c hive.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)
