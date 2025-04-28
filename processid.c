#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "processid.h"
#include "arg_parser.h"

//static Graph* g_graph_list = NULL;  // Przechowuje listę grafów
//static int g_num_graphs = 0;        // Przechowuje liczbę grafów


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

Graph* init_graph(){
    Graph* graph = malloc(sizeof(Graph));
    graph->num_edges = 0;
    graph->num_vertices = 0;
    graph->xadj_capacity = 2048;
    graph->adjncy_capacity = 2048;
    graph->xadj = malloc(graph->xadj_capacity * sizeof(int));
    graph->adjncy = malloc(graph->adjncy_capacity * sizeof(int));

    return graph;
}


Graph* assign_values(LineContainer* container, Graph* graph, int num_line){
    char* line = container->lines[1];
    graph->num_vertices++;
    for(int i=0; line[i] != '\0'; i++){
        if(line[i] == ';') graph->num_vertices++;
    }
    char* adjncy = strdup(container->lines[3]);

    char* token = strtok(adjncy, ";");
    int adjIdx = 0;

    while(token != NULL){
        if (adjIdx >= graph->adjncy_capacity) {
            graph->adjncy_capacity = 2;
            graph->adjncy = realloc(graph->adjncy, graph->adjncy_capacity *sizeof(int));
        }
       graph->adjncy[adjIdx] = atoi(token); 
       adjIdx++;
       token = strtok(NULL, ";");
    }
    free(adjncy);
    graph->num_edges = adjIdx;

    char* xadj = strdup(container->lines[num_line]);
    token = strtok(xadj, ";");
    int xadjId = 0;
    while(token !=NULL){
        if(xadjId >= graph->xadj_capacity){
            graph->xadj_capacity = 2;
            graph->xadj = realloc(graph->xadj, graph->xadj_capacity *sizeof(int));
        }
        graph->xadj[xadjId] = atoi(token);
        xadjId++;
        token = strtok(NULL, ";");
    }
    free(xadj);

    return graph;
}




