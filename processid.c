#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "processid.h"
#include "arg_parser.h"

static Graph* g_graph_list = NULL;  // Przechowuje listę grafów
static int g_num_graphs = 0;        // Przechowuje liczbę grafów


char* read_line(FILE* file) {
    int buffer_size = 1024;
    char* line = malloc(buffer_size);
    if(line == NULL){
        fprintf(stderr, "Memory allocation failed.");
        return NULL;
    }
    int curr_len = 0;
    char c;
    while((c = fgetc(file)) != EOF && c!='\n' ){
        if(curr_len + 1 >= buffer_size){
            buffer_size *= 2;
            char* new_line = realloc(line, buffer_size);
            if(new_line == NULL){
                fprintf(stderr, "Memory allocation failed.");
                free(line);
                return NULL;
            }
            line = new_line;
        }
        
        line[curr_len++] = c;
    }
    if(curr_len == 0 && c == EOF){
        free(line);
        return NULL;
    }
    line[curr_len] = '\0';

    return line;

}

LineContainer* create_line_container(){
    LineContainer* container = malloc(sizeof(LineContainer));
    container->capacity = 10;
    container->lines = malloc(container->capacity * sizeof(char*));
    container->num_lines = 0;
    return container;
}

int add_line(LineContainer* container, char* line){
    if(container->num_lines >= container->capacity) {
        container->capacity *= 2;
        char** new_lines = realloc(container->lines, container->capacity * sizeof(char*));

        container->lines = new_lines;
    }
    container->lines[container->num_lines++] = line;
    return 0;
}

Graph* init_graph_list(int count) {
    g_graph_list = malloc(count * sizeof(Graph));
    g_num_graphs = count;
    
    if (g_graph_list == NULL) {
        fprintf(stderr, "Memory allocation failed for graph list\n");
        exit(1);
    }
    
    for (int i = 0; i < count; i++) {
        g_graph_list[i].num_edges = 0;
        g_graph_list[i].num_vertices = 0;
        g_graph_list[i].xadj_capacity = 2048;
        g_graph_list[i].adjncy_capacity = 2048;
        g_graph_list[i].xadj = malloc(g_graph_list[i].xadj_capacity * sizeof(int));
        g_graph_list[i].adjncy = malloc(g_graph_list[i].adjncy_capacity * sizeof(int));
        
        if (g_graph_list[i].xadj == NULL || g_graph_list[i].adjncy == NULL) {
            fprintf(stderr, "Memory allocation failed for graph arrays\n");
            exit(1);
        }
    }
    
    return g_graph_list;
}


Graph assign_values(LineContainer* container, int num_line) {
    Graph graph;
    graph.num_vertices = 0;
    graph.num_edges = 0;
    graph.xadj_capacity = 2048;
    graph.adjncy_capacity = 2048;
    graph.xadj = malloc(graph.xadj_capacity * sizeof(int));
    graph.adjncy = malloc(graph.adjncy_capacity * sizeof(int));
    
    if (graph.xadj == NULL || graph.adjncy == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Count vertices from line 1
    char* line = container->lines[1];
    graph.num_vertices++;
    for(int i=0; line[i] != '\0'; i++){
        if(line[i] == ';') graph.num_vertices++;
    }
    
    // Process adjncy data
    char* adjncy = strdup(container->lines[3]);
    if (adjncy == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    char* token = strtok(adjncy, ";");
    int adjIdx = 0;

    while(token != NULL){
        if (adjIdx >= graph.adjncy_capacity) {
            graph.adjncy_capacity *= 2;
            int* new_adjncy = realloc(graph.adjncy, graph.adjncy_capacity * sizeof(int));
            if (new_adjncy == NULL) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(adjncy);
                exit(1);
            }
            graph.adjncy = new_adjncy;
        }
        graph.adjncy[adjIdx] = atoi(token); 
        adjIdx++;
        token = strtok(NULL, ";");
    }
    free(adjncy);
    graph.num_edges = adjIdx;

    // Process xadj data - using the specific line number passed
    char* xadj = strdup(container->lines[num_line]);
    if (xadj == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    token = strtok(xadj, ";");
    int xadjId = 0;
    while(token != NULL){
        if(xadjId >= graph.xadj_capacity){
            graph.xadj_capacity *= 2;
            int* new_xadj = realloc(graph.xadj, graph.xadj_capacity * sizeof(int));
            if (new_xadj == NULL) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(xadj);
                exit(1);
            }
            graph.xadj = new_xadj;
        }
        graph.xadj[xadjId] = atoi(token);
        xadjId++;
        token = strtok(NULL, ";");
    }
    free(xadj);

    return graph;
}

void read_mltp_graphs(Graph* graph_list, LineContainer* container){
    for(int i=4; i<container->num_lines; i++){
       graph_list[i-4] = assign_values(container, i); 
    }
}





