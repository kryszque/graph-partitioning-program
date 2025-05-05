#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ggp.h"
//funkcja do tworzenia grafu w formacie csr
Graph* generate_graph(int num_vertices, int num_edges) {
    Graph* g = (Graph*)malloc(sizeof(Graph));
    if (!g) {
        printf("Memory allocation for graph structure failed!\n");
        exit(EXIT_FAILURE);
    }

    g->num_vertices = num_vertices;
    g->num_edges = 0;
    g->xadj = (int*)malloc((num_vertices + 1) * sizeof(int));
    g->adjncy = (int*)malloc(2 * num_edges * sizeof(int)); // razy 2 dla nieskierowanego grafu

    if (!g->xadj || !g->adjncy) {
        printf("Memory allocation for graph data failed!\n");
        free(g);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= num_vertices; i++) {
        g->xadj[i] = 0;
    }

    int** adjacency_matrix = (int**)calloc(num_vertices, sizeof(int*));
    for (int i = 0; i < num_vertices; i++) {
        adjacency_matrix[i] = (int*)calloc(num_vertices, sizeof(int));
    }

    srand(time(NULL));

    while (g->num_edges < num_edges) {
        int u = rand() % num_vertices;
        int v = rand() % num_vertices;
        
        if (u == v || adjacency_matrix[u][v] == 1)
            continue; // Unikamy pętli i podwójnych krawędzi

        adjacency_matrix[u][v] = 1;
        adjacency_matrix[v][u] = 1;
        g->num_edges++;
    }

    int idx = 0;
    for (int i = 0; i < num_vertices; i++) {
        g->xadj[i] = idx;
        for (int j = 0; j < num_vertices; j++) {
            if (adjacency_matrix[i][j]) {
                g->adjncy[idx++] = j;
            }
        }
    }
    g->xadj[num_vertices] = idx; // Zakończenie

    for (int i = 0; i < num_vertices; i++) {
        free(adjacency_matrix[i]);
    }
    free(adjacency_matrix);

    return g;
}

void free_graph(Graph* graph) {
    if (graph) {
        free(graph->xadj);
        free(graph->adjncy);
        free(graph);
    }
}
int main(int argc, char **argv){
    srand(time(NULL));
    Graph* g = generate_graph(39741, 105078);
    int *best_partition = (int*)calloc(g->num_vertices,sizeof(int));
    double imbalance = 0.1;
    int num_parts = 7;
    int num_tries = 10;
    int best_cuts = multi_start_greedy_partition(g,best_partition, imbalance, num_parts, num_tries);
    display_partition(g, best_partition, num_parts, best_cuts);
    free(best_partition);
    free_graph(g);
    return 0;
}