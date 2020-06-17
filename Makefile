CC=gcc
CFLAGS =-c -Wall -O3
LD=$(CC)
LDFLAGS=-lm

TARGET=dctquant.out
OBJECTS=$(patsubst %.c, %.o, $(wildcard *.c))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)
