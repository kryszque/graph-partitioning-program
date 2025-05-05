CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -fopenmp
FLAGS = -O2 -Wall -Wextra -std=c11

# Nazwa programu
TARGETS = G_DIV test

# Foldery
OBJDIR = out
SRC_DIR = src

# Pliki źródłowe
SRCS = $(SRC_DIR)/ggp.c $(SRC_DIR)/processid.c $(SRC_DIR)/output_file.c $(SRC_DIR)/main.c

# Pliki obiektowe (np. out/ggp.o)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

G_DIV: $(OBJS)
	$(CC) $(OBJS) -o $@ $(CFLAGS)

test: $(OBJDIR)/ggp.o $(OBJDIR)/test_ggp.o

# Kompilacja każdego pliku .c do out/*.o
$(OBJDIR)/test_ggp.o: $(SRC_DIR)/test_ggp.c $(SRC_DIR)/ggp.c $(SRC_DIR)/ggp.h
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
$(OBJDIR)/ggp.o: $(SRC_DIR)/ggp.c $(SRC_DIR)/ggp.h $(SRC_DIR)/processid.h
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/processid.o: $(SRC_DIR)/processid.c $(SRC_DIR)/processid.h $(SRC_DIR)/arg_parser.h
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(FLAGS) -c $< -o $@

$(OBJDIR)/output_file.o: $(SRC_DIR)/output_file.c $(SRC_DIR)/output_file.h $(SRC_DIR)/processid.h
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(FLAGS) -c $< -o $@

$(OBJDIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/output_file.h $(SRC_DIR)/processid.h $(SRC_DIR)/arg_parser.h $(SRC_DIR)/ggp.h
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(FLAGS) -c $< -o $@

# Czyszczenie
clean:
	rm -rf $(OBJDIR) $(TARGETS)
