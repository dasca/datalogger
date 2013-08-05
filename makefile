TARGET = datalogger
LIBS = -lpthread -lsqlite3
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean debug

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ 

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

debug:	CFLAGS += -DDEBUG
debug:	clean
debug:	all

