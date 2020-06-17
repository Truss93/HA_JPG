CC=gcc
CFLAGS =-c -Wall -O3


output: main.o bitmap.o compress.o decompress.o dctquant.o
	$(CC) -o dctquant.out main.o bitmap.o compress.o decompress.o dctquant.o  -lm -fsanitize=address

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm *.o
