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

Graph* init_graph_list(int count){
    Graph* graph_list = malloc(count * sizeof(Graph));
    for (int i = 0; i < count; i++) {
        graph_list[i].num_edges = 0;
        graph_list[i].num_vertices = 0;
        graph_list[i].xadj_capacity = 2048;
        graph_list[i].adjncy_capacity = 2048;
        graph_list[i].xadj = malloc(graph_list[i].xadj_capacity * sizeof(int));
        graph_list[i].adjncy = malloc(graph_list[i].adjncy_capacity * sizeof(int));
    }

    return graph_list;
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

int main(int argc, char **argv)
{
    // Parse command-line arguments
    if (parse_arguments(argc, argv) != 0) {
        return 1;
    }


    // Get parsed arguments
    struct arguments *args = get_arguments();

    // Open input file
    FILE *input_file = fopen(args->input, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: Could not open input file %s\n", args->input);
        return 1;
    }

    LineContainer* container = create_line_container();
        // Read lines
    char* line;
    while ((line = read_line(input_file)) != NULL) {
        if (add_line(container, line) != 0) {
            // Handle error
            fclose(input_file);
            return 1;
        }
    }
   


    int num_graphs = container->num_lines - 4;
    if (num_graphs <= 0) {
        fprintf(stderr, "Not enough lines to process graphs\n");
        return 1;
    }
    Graph* graph_list = init_graph_list(num_graphs);

    read_mltp_graphs(graph_list, container);
    for(int i=0; i<container->num_lines-4;i++){
        printf("Num edges of %d graph: %d\n",i ,graph_list[i].num_edges);
    }
    
    // Output file handling
    FILE *output_file = args->output ? fopen(args->output, "w") : stdout;
    if (output_file == NULL) {
        fprintf(stderr, "Error: Could not open output file %s\n", args->output);
        fclose(input_file);
        return 1;
    }

    // Format handling
    printf("Output format: %s\n", args->format);
    
    // Close files
    fclose(input_file);
    if (output_file != stdout) {
        fclose(output_file);
    }

    return 0;
}
