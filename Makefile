CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -fopenmp
FLAGS = -O2 -Wall -Wextra -std=c11

# Nazwa programu
TARGET = test

# Reguły budowania
$(TARGET): ggp.o test_ggp.o processid.o arg_parser.o
	$(CC) ggp.o test_ggp.o processid.o  arg_parser.o -o $(TARGET) $(CFLAGS)

ggp.o: ggp.c ggp.h
	$(CC) $(CFLAGS) -c ggp.c -o ggp.o

test_ggp.o: test_ggp.c ggp.h
	$(CC) $(CFLAGS) -c test_ggp.c -o test_ggp.o

arg_parser.o: arg_parser.c arg_parser.h
	$(CC) $(FLAGS) -c arg_parser.c -o arg_parser.o

processid.o: processid.c processid.h
	$(CC) $(FLAGS) -c processid.c -o processid.o
clean:
	rm -f *.o $(TARGET)
