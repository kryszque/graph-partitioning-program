#include <stdio.h>
#include <stdlib.h>
#include "processid.h"
#include "arg_parser.h"


int main(int argc, char **argv) {
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
