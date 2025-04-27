CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -fopenmp

# Nazwa programu
TARGET = test

# Reguły budowania
$(TARGET): ggp.o test_ggp.o
	$(CC) ggp.o test_ggp.o -o $(TARGET) $(CFLAGS)

ggp.o: ggp.c ggp.h
	$(CC) $(CFLAGS) -c ggp.c -o ggp.o

test_ggp.o: test_ggp.c ggp.h
	$(CC) $(CFLAGS) -c test_ggp.c -o test_ggp.o

clean:
	rm -f *.o $(TARGET)
