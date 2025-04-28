CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -fopenmp
FLAGS = -O2 -Wall -Wextra -std=c11

# Nazwa programu
TARGET = G_DIV

# Reguły budowania
$(TARGET): ggp.o processid.o arg_parser.o output_file.o
	$(CC) ggp.o processid.o  arg_parser.o output_file.o -o $(TARGET) $(CFLAGS)

ggp.o: ggp.c ggp.h
	$(CC) $(CFLAGS) -c ggp.c -o ggp.o

arg_parser.o: arg_parser.c 
	$(CC) $(FLAGS) -c arg_parser.c -o arg_parser.o

processid.o: processid.c processid.h arg_parser.h
	$(CC) $(FLAGS) -c processid.c -o processid.o

output_file.o: output_file.c output_file.h processid.h
	$(CC) $(FLAGS) -c output_file.c -o output_file.o

main.o: main.c output_file.h processid.h arg_parser.h ggp.h
	$(CC) $(FLAGS) -c main.c -o main.o
clean:
	rm -f *.o $(TARGET)
