#ifndef GGP_H
#define GGP_H

typedef struct {
    int num_vertices;
    int num_edges;
    int* xadj;     // Indices into adjncy
    int* adjncy;   // Adjacency lists
} Graph;

void update_boundry(Graph* graph, int* partition, int* boundry, int current_part);
int calculate_cut_increase(Graph* graph, int* partition, int v, int current_part);
int count_edge_cuts(Graph* graph, int* partition);
int* greedy_partition(Graph* graph, float imbalance, int num_parts);
void multi_start_greedy_partition(Graph* graph, int* best_partition, float imbalance, int num_parts, int num_tries);

#endif