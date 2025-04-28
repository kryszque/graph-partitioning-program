#include <stdio.h>
#include <stdlib.h>
#include "processid.h"
#include "arg_parser.h"
#include "ggp.h"
#include "output_file.h"


int main(int argc, char **argv) {
    srand(time(NULL));
    // Parsowanie argumentów wiersza poleceń
    if (parse_arguments(argc, argv) != 0) {
        return 1;
    }
    struct arguments* args = get_arguments();

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
    
    // Utworzenie struktury GraphList
    GraphList graph_list;
    graph_list.num_graphs = container->num_lines - 4;
    
    if (graph_list.num_graphs <= 0) {
        fprintf(stderr, "Error: Not enough lines to process graphs\n");
        return 1;
    }
    
    // Inicjalizacja tablicy grafów
    graph_list.graphs = init_graph_list(graph_list.num_graphs);
    read_mltp_graphs(graph_list.graphs, container);
    
    // Wyświetlenie podstawowych informacji o grafach
    printf("Graph List contains %d graphs:\n", graph_list.num_graphs);
    for (int i = 0; i < graph_list.num_graphs; i++) {
        printf("Graph %d: %d vertices, %d edges\n", 
               i, graph_list.graphs[i].num_vertices, graph_list.graphs[i].num_edges);
    }
    
    //dane do podzialu
    double imabalance = args->margin;
    int num_parts = args->parts;
    int num_tries = 10;
    char decision = args->format[0];
    char* output_file = args->output;
    //podzial grafów
    for(int graph = 0; graph < graph_list.num_graphs; graph++){
        if(graph_list.graphs[graph].num_vertices < 10000){
            int* best_partition = (int*)calloc(graph_list.graphs[graph].num_vertices, sizeof(int));
            int best_cuts = multi_start_greedy_partition(&graph_list.graphs[graph], best_partition, imabalance,
            num_parts, num_tries);
            //wyswietlanie podziału
            display_partition(&graph_list.graphs[graph], best_partition, num_parts, best_cuts);
            switch (decision)
            {
            case 't':
                getResult_txt(&graph_list.graphs[graph], container, best_partition, num_parts, output_file);
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



    // Zwolnienie zasobów
    for (int i = 0; i < graph_list.num_graphs; i++) {
        free(graph_list.graphs[i].xadj);
        free(graph_list.graphs[i].adjncy);
    }
    free(graph_list.graphs);
    
    for (int i = 0; i < container->num_lines; i++) {
        free(container->lines[i]);
    }
    free(container->lines);
    free(container);
    
    return 0;
}
