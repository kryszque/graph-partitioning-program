#ifndef GGP_H
#define GGP_H
#include "processid.h"

typedef struct{
    int size;
    int* vertices;
    int capacity;
}BoundryList;

typedef struct {
    int vertex;
    int best_destination;
    int cut_change; 
} VertexMove;

void double_capacity(BoundryList* boundry, int* partition);
void create_boundry(Graph* graph, int* partition, BoundryList* boundry, int current_part);
void update_boundry(Graph* graph, int* partition, BoundryList* boundry, int removed_vertex);
int calculate_cut_increase(Graph* graph, int* partition, int v, int current_part);
int find_best_vertex(Graph* graph, BoundryList* boundry, int* partition, int current_part);
int count_edge_cuts(Graph* graph, int* partition);
int calculate_cut_change(Graph* graph, int* partition, int v, int from_part, int to_part);
void refine_partitions(Graph* graph, int* partition, int num_parts, float imbalance);
int* greedy_partition(Graph* graph, float imbalance, int num_parts);
int multi_start_greedy_partition(Graph* graph, int* best_partition, float imbalance, int num_parts, int num_tries);
void display_partition(Graph* graph, int* best_partition, int num_parts, int best_cuts);

#endif