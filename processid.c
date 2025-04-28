#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "processid.h"
#include "arg_parser.h"


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
    printf("DEBUG: Initializing graph list with %d graphs\n", count);
    Graph* graph_list = malloc(count * sizeof(Graph));
    
    if (graph_list == NULL) {
        fprintf(stderr, "Memory allocation failed for graph list\n");
        exit(1);
    }
    
    for (int i = 0; i < count; i++) {
        printf("DEBUG: Initializing graph %d\n", i);
        graph_list[i].num_edges = 0;
        graph_list[i].num_vertices = 0;
        graph_list[i].xadj_capacity = 2048;
        graph_list[i].adjncy_capacity = 2048;
        graph_list[i].xadj = malloc(graph_list[i].xadj_capacity * sizeof(int));
        graph_list[i].adjncy = malloc(graph_list[i].adjncy_capacity * sizeof(int));
        
        if (graph_list[i].xadj == NULL || graph_list[i].adjncy == NULL) {
            fprintf(stderr, "Memory allocation failed for graph arrays\n");
            exit(1);
        }
    }
    
    printf("DEBUG: Graph list initialization complete\n");
    return graph_list;
}


void assign_values(LineContainer* container, GraphList* graph_list, int line_index, int graph_index) {
    printf("DEBUG: Assigning values for graph %d using line %d\n", graph_index, line_index);
    
    // Validate inputs
    if (graph_index < 0 || graph_index >= graph_list->num_graphs) {
        fprintf(stderr, "ERROR: Invalid graph index %d (max: %d)\n", graph_index, graph_list->num_graphs - 1);
        return;
    }
    
    if (line_index < 0 || line_index >= container->num_lines) {
        fprintf(stderr, "ERROR: Invalid line index %d (max: %d)\n", line_index, container->num_lines - 1);
        return;
    }
    
    // Get the current graph
    Graph* current_graph = &(graph_list->graphs[graph_index]);
    printf("DEBUG: Got graph pointer at index %d\n", graph_index);
    
    // Make sure we're working with a valid line for vertex count
    if (1 >= container->num_lines) {
        fprintf(stderr, "ERROR: Container doesn't have enough lines for vertex info (line 1)\n");
        return;
    }
    
    // Count vertices from line 1 (should be the same for all graphs)
    char* line = container->lines[1];
    printf("DEBUG: Counting vertices from line: %s\n", line);
    
    current_graph->num_vertices = 1; // Start with 1 vertex
    for(int i = 0; line[i] != '\0'; i++) {
        if(line[i] == ';') current_graph->num_vertices++;
    }
    printf("DEBUG: Counted %d vertices\n", current_graph->num_vertices);
    
    // Make sure we're working with a valid line for adjacency
    if (3 >= container->num_lines) {
        printf("ERROR: Container doesn't have enough lines for adjacency info (line 3)\n");
        return;
    }
    
    // Process adjncy data from line 3 (should be the same for all graphs)
    printf("Przed strdup");
    char* adjncy = strdup(container->lines[3]);
    printf("Po strdup");
    if (adjncy == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    char* token = strtok(adjncy, ";");
    int adjIdx = 0;

    while(token != NULL) {
        if (adjIdx >= current_graph->adjncy_capacity) {
            current_graph->adjncy_capacity *= 2;
            int* new_adjncy = realloc(current_graph->adjncy, current_graph->adjncy_capacity * sizeof(int));
            if (new_adjncy == NULL) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(adjncy);
                exit(1);
            }
            current_graph->adjncy = new_adjncy;
        }
        current_graph->adjncy[adjIdx] = atoi(token); 
        adjIdx++;
        token = strtok(NULL, ";");
    }
    free(adjncy);
    current_graph->num_edges = adjIdx;
    printf("DEBUG: Processed %d edges\n", current_graph->num_edges);

    // Process xadj data from the specified line
    printf("DEBUG: Processing xadj data from line: %s\n", container->lines[line_index]);
    char* xadj = strdup(container->lines[line_index]);
    if (xadj == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    token = strtok(xadj, ";");
    int xadjId = 0;
    while(token != NULL) {
        if(xadjId >= current_graph->xadj_capacity) {
            current_graph->xadj_capacity *= 2;
            int* new_xadj = realloc(current_graph->xadj, current_graph->xadj_capacity * sizeof(int));
            if (new_xadj == NULL) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(xadj);
                exit(1);
            }
            current_graph->xadj = new_xadj;
        }
        current_graph->xadj[xadjId] = atoi(token);
        xadjId++;
        token = strtok(NULL, ";");
    }
    free(xadj);
    printf("DEBUG: Processed %d xadj values for graph %d\n", xadjId, graph_index);
}

void read_mltp_graphs(GraphList* graph_list, LineContainer* container) {
    for(int i=0; i<graph_list->num_graphs; i++) {
        assign_values(container, graph_list, i+4, i); // Pass both the line index and graph index
    }
}





