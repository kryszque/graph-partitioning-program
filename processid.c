#include <stdio.h>
#include <stdlib.h>
#include "arg_parser.h"

typedef struct {
    int num_vertices;
    int num_edges;
    int *xadj;
    int *adjncy;
} Graph;


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
    return line;

}

int main(int argc, char **argv)
{
    // Parse command-line arguments
    if (parse_arguments(argc, argv) != 0) {
        return 1;
    }
    Graph* graph = malloc(sizeof(Graph));
    
    graph->xadj = malloc(8192);
    graph->adjncy = malloc(8192);


    // Get parsed arguments
    struct arguments *args = get_arguments();
    printf("input: %s",args->input);

    // Open input file
    FILE *input_file = fopen(args->input, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: Could not open input file %s\n", args->input);
        return 1;
    }

    // Process file based on parsed arguments
    printf("Processing file: %s\n", args->input);
    printf("Number of parts: %d\n", args->parts);
    printf("Margin: %d%%\n", args->margin);
    
    // Output file handling
    FILE *output_file = args->output ? fopen(args->output, "w") : stdout;
    if (output_file == NULL) {
        fprintf(stderr, "Error: Could not open output file %s\n", args->output);
        fclose(input_file);
        return 1;
    }

    // Format handling
    printf("Output format: %s\n", args->format);
    
    char* line;
    line = read_line(input_file);
    printf("%s\n", line);
    int num = line[0] - '0';
    printf("%d", num);
    // Close files
    fclose(input_file);
    if (output_file != stdout) {
        fclose(output_file);
    }

    return 0;
}
