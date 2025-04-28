#ifndef PROCESSID_H
#define PROCESSID_H
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int num_vertices;
    int num_edges;
    int *xadj;
    int *adjncy;
    int xadj_capacity;
    int adjncy_capacity;
} Graph;

typedef struct {
    char **lines;
    int num_lines;
    int capacity;
} LineContainer;

typedef struct {
    Graph* graphs;      // Wskaźnik do tablicy grafów
    int num_graphs;     // Liczba grafów w tablicy
} GraphList;

char* read_line(FILE* file); 
LineContainer* create_line_container();
int add_line(LineContainer* container, char* line);
Graph* init_graph_list(int count);
void assign_values(LineContainer* container, GraphList* graph_list, int line_index, int graph_index);
void read_mltp_graphs(GraphList* graph_list, LineContainer* container);




#endif
