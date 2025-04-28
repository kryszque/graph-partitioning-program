#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "processid.h"
#include "arg_parser.h"
#include "ggp.h"
#include "output_file.h"


int main(int argc, char **argv) {
    srand(time(NULL));
    printf("test1\n");
    // Parsowanie argumentów wiersza poleceń
    if (parse_arguments(argc, argv) != 0) {
        return 1;
    }
    struct arguments* args = get_arguments();
      printf("test2\n");
    // Otwarcie pliku wejściowego
    FILE* input_file = fopen(args->input, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: Could not open input file %s\n", args->input);
        return 1;
    }

    // Wczytanie linii z pliku
    LineContainer* container = create_line_container();
    char* line;
    while ((line = read_line(input_file)) != NULL) {
        add_line(container, line);
    }
    fclose(input_file);
    printf("test3\n");
    ////////////////////////////////
    // Inicjalizacja tablicy grafów
    printf("DEBUG: About to initialize graph_list\n");
    GraphList* graph_list = malloc(sizeof(GraphList));
    if (graph_list == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for graph list.\n");
        // Clean up container
        for (int i = 0; i < container->num_lines; i++) {
            free(container->lines[i]);
        }
        free(container->lines);
        free(container);
        return 1;
    }

    printf("DEBUG: Container num_lines: %d\n", container->num_lines);
    graph_list->num_graphs = container->num_lines - 4;
    printf("DEBUG: Setting num_graphs to %d\n", graph_list->num_graphs);

    if (graph_list->num_graphs <= 0) {
        fprintf(stderr, "Error: Not enough lines to process graphs.  Num Lines: %d\n", container->num_lines);
        free(graph_list);
        // Clean up container
        for (int i = 0; i < container->num_lines; i++) {
            free(container->lines[i]);
        }
        free(container->lines);
        free(container);
        return 1;
    }

    printf("DEBUG: About to call init_graph_list with count %d\n", graph_list->num_graphs);
    graph_list->graphs = init_graph_list(graph_list->num_graphs);
    if (graph_list->graphs == NULL) {
        fprintf(stderr, "Error: init_graph_list failed.\n");
        free(graph_list);
        // Clean up container
        for (int i = 0; i < container->num_lines; i++) {
            free(container->lines[i]);
        }
        free(container->lines);
        free(container);
        return 1;
    }
    printf("DEBUG: init_graph_list returned, graph_list->graphs = %p\n", (void*)graph_list->graphs);

    printf("test3.5\n");

    // Przetwarzanie grafów
    printf("DEBUG: About to call read_mltp_graphs\n");
    read_mltp_graphs(graph_list, container);
    printf("DEBUG: read_mltp_graphs completed\n");
    ///////////////////////////////
    printf("test4\n");

    // Wyświetlenie podstawowych informacji o grafach
    printf("Graph List contains %d graphs:\n", graph_list->num_graphs);
    for (int i = 0; i < graph_list->num_graphs; i++) {
        printf("Graph %d: %d vertices, %d edges\n",
               i, graph_list->graphs[i].num_vertices, graph_list->graphs[i].num_edges);
    }
    printf("test5\n");

    // Dane do podziału
    double imabalance = args->margin;
    int num_parts = args->parts;
    int num_tries = 10;
    char decision = args->format[0];
    char* output_file = args->output;    

    //podzial grafów
    for(int graph = 0; graph < graph_list->num_graphs; graph++){
        if(graph_list->graphs[graph].num_vertices < 10000){
            int* best_partition = (int*)calloc(graph_list->graphs[graph].num_vertices, sizeof(int));
            int best_cuts = multi_start_greedy_partition(&graph_list->graphs[graph], best_partition, imabalance,
            num_parts, num_tries);
            //wyswietlanie podziału
            display_partition(&graph_list->graphs[graph], best_partition, num_parts, best_cuts);
            switch (decision)
            {
            case 't':
                getResult_txt(&graph_list->graphs[graph], container, best_partition, num_parts, output_file);
                break;
            
            case 'b':
                break;
            
            default:
                break;
            }
            free(best_partition);
        }
        else{
            //metis
        }
    }
    printf("test6\n");


    // Zwolnienie zasobów
    for (int i = 0; i < graph_list->num_graphs; i++) {
        free(graph_list->graphs[i].xadj);
        free(graph_list->graphs[i].adjncy);
    }
    free(graph_list->graphs);
    
    for (int i = 0; i < container->num_lines; i++) {
        free(container->lines[i]);
    }
    free(container->lines);
    free(container);
    
    return 0;
}
