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


char* read_line(FILE* file); 
Graph* init_graph();
LineContainer* create_line_container();
int add_line(LineContainer* container, char* line);
Graph* init_graph_list(int count);
Graph assign_values(LineContainer* container, int num_line);
void read_mltp_graphs(Graph* graph_list, LineContainer* container);



#endif
