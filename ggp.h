#ifndef GGP_H
#define GGP_H

typedef struct {
    int num_vertices;
    int num_edges;
    int* xadj;     // Indices into adjncy
    int* adjncy;   // Adjacency lists
} Graph;

void greedy_partition(Graph* graph, int* partition, float imbalance, int num_parts);

#endif