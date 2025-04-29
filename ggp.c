#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <omp.h>
#include <time.h>
#include "ggp.h"
#include "processid.h"

void double_capacity(BoundryList* boundry, int* partition){
    // Check for integer overflow
    if (boundry->capacity > INT_MAX / 2) {
        fprintf(stderr, "CAPACITY TOO LARGE FOR DOUBLING\n");
        return; // Handle gracefully instead of exit
    }

    size_t new_capacity = boundry->capacity * 2;
    int* new_vertices = realloc(boundry->vertices, new_capacity * sizeof(int));

    if (!new_vertices) {
        fprintf(stderr, "REALOKACJA BOUNDRY NIEUDANA!\n");
        return; // Handle gracefully instead of exit
    }

    boundry->vertices = new_vertices;
    boundry->capacity = new_capacity;
}

void create_boundry(Graph* graph, int* partition, BoundryList* boundry, int current_part){
    memset(partition, 0, graph->num_vertices * sizeof(int));
    #pragma omp parallel
    {
        //kazdy watek ma lokalne buffory
        int local_capacity = 128;
        int* local_vertices = (int*)calloc(local_capacity, sizeof(int));
        int local_size = 0;

        #pragma omp for nowait
        for(int i = 0; i < graph->num_vertices; i++){
            if(partition[i] == current_part){
                for(int j = graph->xadj[i]; j < graph->xadj[i+1]; j++){
                    int neighbour = graph->adjncy[j];
                    if(partition[neighbour] == 0){
                        if(local_size == local_capacity){
                            local_capacity *= 2;
                            local_vertices = (int*)realloc(local_vertices, local_capacity * sizeof(int));
                            if (!local_vertices) {
                                fprintf(stderr, "NIEUDANA ALOKACJA PAMIECI DLA LOCAL_VERTICES!!!\n");
                                free(local_vertices);
                                free(boundry->vertices);
                                free(partition);
                                exit(EXIT_FAILURE);
                            }
                        }
                        local_vertices[local_size++] = neighbour;
                    }
                }
            }
        }
        //teraz zsynchronizowana sekcja - dodawane sa wyniki do glownej globalnej listy
        #pragma omp critical
        {
            for(int k = 0; k < local_size; k++){
                if(boundry->size == boundry->capacity){
                    double_capacity(boundry, partition);
                }
                boundry->vertices[boundry->size++] = local_vertices[k];
            }
        }
        free(local_vertices);
    }
}

void update_boundry(Graph* graph, int* partition, BoundryList* boundry, int removed_vertex){
    // First verify if the vertex is valid
    if (removed_vertex < 0 || removed_vertex >= graph->num_vertices) {
        return;
    }
    
    int removed_vertex_index = -1;
    
    // Find the vertex in the boundary list with bounds checking
    for(int j = 0; j < boundry->size; j++) {
        if (j >= boundry->capacity) break; // Safety check
        
        if(boundry->vertices[j] == removed_vertex) {
            removed_vertex_index = j;
            break;
        }
    }
    
    if (removed_vertex_index == -1) {
        // Vertex not in boundary, silently return
        return;
    }
    
    int replacement_found = 0;
    
    // Ensure proper bounds checking for xadj access
    if (removed_vertex + 1 >= graph->num_vertices) return;
    
    int start_idx = graph->xadj[removed_vertex];
    int end_idx = graph->xadj[removed_vertex + 1];
    
    // Validate indices
    if (start_idx < 0 || end_idx > graph->xadj[graph->num_vertices] || start_idx > end_idx) {
        return;
    }
    
    for (int i = start_idx; i < end_idx; i++) {
        // Additional bounds check
        if (i < 0 || i >= graph->xadj[graph->num_vertices]) continue;
        
        int neighbour = graph->adjncy[i];
        
        // Validate neighbor index
        if (neighbour < 0 || neighbour >= graph->num_vertices) continue;
        
        if (partition[neighbour] == 0) {
            // Check if already in boundary with safer bounds checking
            int already_in_boundary = 0;
            for (int k = 0; k < boundry->size; k++) {
                if (k >= boundry->capacity) break; // Safety check
                
                if (boundry->vertices[k] == neighbour) {
                    already_in_boundary = 1;
                    break;
                }
            }
            
            if (!already_in_boundary) {
                if (!replacement_found) {
                    // Replace at the found index, with bounds check
                    if (removed_vertex_index < boundry->capacity) {
                        boundry->vertices[removed_vertex_index] = neighbour;
                        replacement_found = 1;
                    }
                } else {
                    // Add to the end, with proper capacity management
                    if (boundry->size >= boundry->capacity) {
                        double_capacity(boundry, partition);
                    }
                    
                    // Final safety check before writing
                    if (boundry->size < boundry->capacity) {
                        boundry->vertices[boundry->size++] = neighbour;
                    }
                }
            }
        }
    }
    
    // Handle removal when no replacement was found
    if (!replacement_found) {
        // Safe removal with bounds checking
        for (int k = removed_vertex_index; k < boundry->size - 1; k++) {
            if (k >= boundry->capacity || k+1 >= boundry->capacity) break;
            boundry->vertices[k] = boundry->vertices[k + 1];
        }
        
        if (boundry->size > 0) {
            boundry->size--;
        }
    }
}

int calculate_cut_increase(Graph* graph, int* partition, int v, int current_part){
    if (v < 0 || v + 1 >= graph->num_vertices)
        return 0;
    int cut_increase = 0;
    //dla danego wierzcholka (kandydata do dodania do aktualnej partycji) sprawdzane są partycje jego sąsiadów 
    //i liczona ilość przeciętych krawędzi w momencie przeniesienia wierzchołka
    for(int i = graph->xadj[v]; i < graph->xadj[v+1]; i++){
        int neighbour = graph->adjncy[i];
        if (neighbour < 0 || neighbour >= graph->num_vertices) continue;
        if(partition[neighbour] == current_part){
            cut_increase --;
        }
        else{
            cut_increase ++;
        }
    }
    return cut_increase;
}

static unsigned int simple_rand(unsigned int* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return *seed;
}

// Funkcja do szukania najlepszego wierzchołka do dodania
int find_best_vertex(Graph* graph, BoundryList* boundry, int* partition, int current_part) {
    int best_vertex = -1;
    int min_cut_increase = INT_MAX;

        int local_best_vertex = -1;
        int local_min_cut_increase = INT_MAX;
        unsigned int seed = (unsigned int)(time(NULL) ^ omp_get_thread_num());
        for (int i = 0; i < boundry->size; i++) {
            int v = boundry->vertices[i];
            int cut_increase = calculate_cut_increase(graph, partition, v, current_part);

            if (cut_increase < local_min_cut_increase || 
                (cut_increase == local_min_cut_increase && (simple_rand(&seed) % 2 == 0))) {
                local_min_cut_increase = cut_increase;
                local_best_vertex = v;
            }
        }
        //redukcja lokalnych wynikow do globalnych

        
            if (local_min_cut_increase < min_cut_increase ||
                (local_min_cut_increase == min_cut_increase && (simple_rand(&seed) % 2 == 0))) {
                min_cut_increase = local_min_cut_increase;
                best_vertex = local_best_vertex;
            }

    return best_vertex;
}

int count_edge_cuts(Graph* graph, int* partition){
    int cuts = 0;
    //obliczona zostaje calkowita ilosc przecietych krawedzi po podziale
    #pragma omp parallel for reduction(+:cuts) schedule(dynamic)
    for(int i = 0; i < graph->num_vertices; i++){
        for(int j = graph->xadj[i]; j < graph->xadj[i+1]; j++){
            int neighbour = graph->adjncy[j];
            if(partition[i] != partition[neighbour])
                cuts ++;
        }
    }
    return cuts/2;
}

int calculate_cut_change(Graph* graph, int* partition, int v, int from_part, int to_part) {
    // Proper bounds checking
    if (v < 0 || v >= graph->num_vertices || v + 1 >= graph->num_vertices)
        return 0;

    int cut_change = 0;

    // Verify array bounds explicitly before accessing
    int start_idx = graph->xadj[v];
    int end_idx = graph->xadj[v+1];

    // Validate index range
    if (start_idx < 0 || end_idx > graph->xadj[graph->num_vertices] || start_idx > end_idx) {
        return 0; // Invalid range, return safely
    }

    for (int i = start_idx; i < end_idx; i++) {
        // Additional bounds check on adjacency list
        if (i < 0 || i >= graph->xadj[graph->num_vertices]) continue;

        int neighbor = graph->adjncy[i];
        // Validate neighbor index
        if (neighbor < 0 || neighbor >= graph->num_vertices) continue;

        if (partition[neighbor] == from_part) {
            // Was not a cut, will become a cut
            cut_change++;
        }
        else if (partition[neighbor] == to_part) {
            // Was a cut, will no longer be a cut
            cut_change--;
        }
    }

    return cut_change;
}

int compare_vertex_moves(const void* a, const void* b) {
    VertexMove* va = (VertexMove*)a;
    VertexMove* vb = (VertexMove*)b;
    return va->cut_change - vb->cut_change;
}

void refine_partitions(Graph* graph, int* partition, int num_parts, float imbalance) {
    // Calculate partition size constraints
    int vertices_per_part = graph->num_vertices / num_parts;
    int max_part_size = (int)(vertices_per_part * (1.0 + imbalance));
    int min_part_size = (int)(vertices_per_part * (1.0 - imbalance));
    if (min_part_size < 1) min_part_size = 1;
    
    // Calculate current partition sizes - Using dynamically allocated array
    int* part_sizes = (int*)malloc(num_parts * sizeof(int));
    if (!part_sizes) {
        fprintf(stderr, "Memory allocation failed for part_sizes\n");
        return;
    }
    memset(part_sizes, 0, sizeof(int) * num_parts);
    
    for (int i = 0; i < graph->num_vertices; i++) {
        // Validate partition index before accessing
        if (partition[i] >= 0 && partition[i] < num_parts) {
            part_sizes[partition[i]]++;
        } else {
            // Fix invalid partitions instead of failing
            partition[i] = 0; // Reset to default partition
            part_sizes[0]++;
            fprintf(stderr, "Fixed invalid partition index: %d for vertex %d\n", partition[i], i);
        }
    }

    // Determine maximum moves per iteration (more flexible based on imbalance)
    int max_moves_per_iteration = (int)(vertices_per_part * imbalance) + 1;
    if (max_moves_per_iteration < 1) max_moves_per_iteration = 1;
    
    // --- Phase 1: Balance extremely overloaded partitions first
    int phase1_iterations = 0;
    int max_phase1_iterations = 10;
    
    while (phase1_iterations++ < max_phase1_iterations) {
        int moves_made_total = 0;
        
        // Handle severely overloaded partitions first
        for (int p = 0; p < num_parts; p++) {
            if (part_sizes[p] > max_part_size) {
                // Define max size for vertex move arrays based on overload
                int excess_vertices = part_sizes[p] - max_part_size;
                int move_array_size = excess_vertices * 2; // Use a reasonable multiplier
                if (move_array_size > graph->num_vertices) 
                    move_array_size = graph->num_vertices;
                
                // Dynamically allocate move arrays to prevent stack overflow
                VertexMove* moves = (VertexMove*)malloc(move_array_size * sizeof(VertexMove));
                if (!moves) {
                    fprintf(stderr, "Memory allocation failed for moves array\n");
                    free(part_sizes);
                    return;
                }
                
                int move_count = 0;
                
                #pragma omp parallel
                {
                    // Thread-local arrays for collecting moves - dynamically allocated
                    VertexMove* private_moves = NULL;
                    int* neighbor_parts = NULL;
                    
                    // Proper allocation with error handling
                    private_moves = (VertexMove*)malloc(move_array_size * sizeof(VertexMove));
                    if (!private_moves) {
                        fprintf(stderr, "Thread memory allocation failed\n");
                    } else {
                        // Initialize to prevent uninitialized values
                        memset(private_moves, 0, move_array_size * sizeof(VertexMove));
                        
                        // Allocate thread-local neighbor_parts array
                        neighbor_parts = (int*)malloc(num_parts * sizeof(int));
                        if (!neighbor_parts) {
                            fprintf(stderr, "Thread memory allocation failed for neighbor_parts\n");
                            free(private_moves);
                            private_moves = NULL;
                        } else {
                            memset(neighbor_parts, 0, num_parts * sizeof(int));
                        }
                    }
                    
                    int private_move_count = 0;
                    
                    #pragma omp for nowait
                    for (int v = 0; v < graph->num_vertices; v++) {
                        // Skip if thread failed to allocate memory
                        if (!private_moves || !neighbor_parts) continue;
                        
                        if (partition[v] != p) continue;
                        
                        int best_dest = -1;
                        int best_cut_change = INT_MAX;
                        
                        // Reset the map of neighboring partitions for this vertex
                        memset(neighbor_parts, 0, sizeof(int) * num_parts);
                        
                        // Verify array bounds before accessing
                        if (v+1 < graph->num_vertices) {
                            int start_idx = graph->xadj[v];
                            int end_idx = graph->xadj[v+1];
                            
                            // Validate index range
                            if (start_idx < 0 || end_idx > graph->xadj[graph->num_vertices] || start_idx > end_idx) {
                                continue; // Skip this vertex if indices are invalid
                            }
                            
                            for (int j = start_idx; j < end_idx; j++) {
                                // Ensure we're accessing valid adjacency data
                                if (j < 0 || j >= graph->xadj[graph->num_vertices]) continue;
                                
                                int neighbor = graph->adjncy[j];
                                // Ensure we have a valid neighbor index
                                if (neighbor < 0 || neighbor >= graph->num_vertices) continue;
                                
                                // Ensure valid partition index
                                int neighbor_part = partition[neighbor];
                                if (neighbor_part >= 0 && neighbor_part < num_parts) {
                                    neighbor_parts[neighbor_part]++;
                                }
                            }
                        }
                        
                        // Find best destination partition
                        for (int d = 0; d < num_parts; d++) {
                            if (d == p) continue;
                            if (part_sizes[d] >= max_part_size) continue;
                            
                            // Calculate gain based on actual cut changes
                            int cut_change = calculate_cut_change(graph, partition, v, p, d);
                            
                            // Prioritize moves to partitions where the vertex has more neighbors
                            if (cut_change < best_cut_change || 
                                (cut_change == best_cut_change && best_dest != -1 && 
                                 neighbor_parts[d] > neighbor_parts[best_dest])) {
                                best_cut_change = cut_change;
                                best_dest = d;
                            }
                        }
                        
                        if (best_dest != -1 && private_move_count < move_array_size) {
                            private_moves[private_move_count++] = (VertexMove){v, best_dest, best_cut_change};
                        }
                    }
                    
                    // Critical section to merge thread-local results
                    #pragma omp critical
                    {
                        if (private_moves) { // Only if memory allocation succeeded
                            for (int i = 0; i < private_move_count && move_count < move_array_size; i++) {
                                moves[move_count++] = private_moves[i];
                            }
                        }
                    }
                    
                    // Free thread-local resources
                    if (neighbor_parts) free(neighbor_parts);
                    if (private_moves) free(private_moves);
                }
                
                if (move_count > 0) {
                    qsort(moves, move_count, sizeof(VertexMove), compare_vertex_moves);
                    
                    int moves_to_make = (part_sizes[p] - max_part_size) < move_count ? 
                                        (part_sizes[p] - max_part_size) : move_count;
                    
                    for (int i = 0; i < moves_to_make; i++) {
                        int v = moves[i].vertex;
                        int dest = moves[i].best_destination;
                        
                        // Ensure destination partition won't exceed max size
                        if (part_sizes[dest] >= max_part_size) continue;
                        
                        // Verify vertex index
                        if (v >= 0 && v < graph->num_vertices) {
                            partition[v] = dest;
                            part_sizes[p]--;
                            part_sizes[dest]++;
                            moves_made_total++;
                        }
                        
                        if (part_sizes[p] <= max_part_size) break;
                    }
                }
                
                // Free dynamically allocated moves array
                free(moves);
                
                // Break if this partition is now within bounds
                if (part_sizes[p] <= max_part_size) break;
            }
        }
        
        if (moves_made_total == 0) {
            break; // No moves made, all partitions within max size
        }
    }
    
    // --- Phase 2: Optimize cut edges while maintaining balance
    int max_refine_iterations = 10;
    int global_improvement = 1;
    
    for (int iter = 0; iter < max_refine_iterations && global_improvement; iter++) {
        global_improvement = 0;
        
        // First, identify underloaded partitions that could accept more vertices
        int* can_grow = (int*)malloc(num_parts * sizeof(int));
        if (!can_grow) {
            fprintf(stderr, "Memory allocation failed for can_grow\n");
            free(part_sizes);
            return;
        }
        memset(can_grow, 0, sizeof(int) * num_parts);
        
        for (int p = 0; p < num_parts; p++) {
            can_grow[p] = (part_sizes[p] < max_part_size);
        }
        
        // Process partitions in order (can be randomized for better results)
        for (int src_part = 0; src_part < num_parts; src_part++) {
            // Skip partitions at minimum size
            if (part_sizes[src_part] <= min_part_size) continue;
            
            // Estimate max potential moves from this partition
            int max_potential_moves = part_sizes[src_part] - min_part_size;
            if (max_potential_moves > graph->num_vertices / num_parts)
                max_potential_moves = graph->num_vertices / num_parts;
            
            if (max_potential_moves <= 0) continue; // Skip if no moves possible
            
            // Dynamically allocate move arrays
            VertexMove* moves = (VertexMove*)malloc(max_potential_moves * sizeof(VertexMove));
            if (!moves) {
                fprintf(stderr, "Memory allocation failed for moves array in phase 2\n");
                free(can_grow);
                free(part_sizes);
                return;
            }
            
            int move_count = 0;
            
            #pragma omp parallel
            {
                // Thread-local arrays - dynamically allocated
                VertexMove* private_moves = NULL;
                int* connections = NULL;
                
                private_moves = (VertexMove*)malloc(max_potential_moves * sizeof(VertexMove));
                if (!private_moves) {
                    fprintf(stderr, "Thread memory allocation failed in phase 2\n");
                } else {
                    // Zero initialize
                    memset(private_moves, 0, max_potential_moves * sizeof(VertexMove));
                    
                    connections = (int*)malloc(num_parts * sizeof(int));
                    if (!connections) {
                        fprintf(stderr, "Thread memory allocation failed for connections\n");
                        free(private_moves);
                        private_moves = NULL;
                    } else {
                        memset(connections, 0, num_parts * sizeof(int));
                    }
                }
                
                int private_move_count = 0;
                
                #pragma omp for nowait
                for (int v = 0; v < graph->num_vertices; v++) {
                    // Skip if memory allocation failed
                    if (!private_moves || !connections) continue;
                    
                    if (partition[v] != src_part) continue;
                    
                    int best_dest = -1;
                    int best_cut_change = 0; // Only consider moves that improve cuts
                    
                    // Reset connections array
                    memset(connections, 0, sizeof(int) * num_parts);
                    
                    // Bounds check before accessing xadj
                    if (v+1 < graph->num_vertices) {
                        int start_idx = graph->xadj[v];
                        int end_idx = graph->xadj[v+1];
                        
                        // Validate index range
                        if (start_idx < 0 || end_idx > graph->xadj[graph->num_vertices] || start_idx > end_idx) {
                            continue; // Skip this vertex if indices are invalid
                        }
                        
                        // Count connections to each partition
                        for (int j = start_idx; j < end_idx; j++) {
                            // Verify adjacency index
                            if (j < 0 || j >= graph->xadj[graph->num_vertices]) continue;
                            
                            int neighbor = graph->adjncy[j];
                            // Verify neighbor index
                            if (neighbor < 0 || neighbor >= graph->num_vertices) continue;
                            
                            int neighbor_part = partition[neighbor];
                            if (neighbor_part >= 0 && neighbor_part < num_parts) {
                                connections[neighbor_part]++;
                            }
                        }
                        
                        // Consider all possible destination partitions
                        for (int dest_part = 0; dest_part < num_parts; dest_part++) {
                            if (dest_part == src_part) continue;
                            if (!can_grow[dest_part]) continue;
                            
                            // Calculate gain when moving v from src_part to dest_part
                            int cut_change = calculate_cut_change(graph, partition, v, src_part, dest_part);
                            
                            if (cut_change < best_cut_change) {
                                best_cut_change = cut_change;
                                best_dest = dest_part;
                            }
                        }
                    }
                    
                    if (best_dest != -1 && best_cut_change < 0 && private_move_count < max_potential_moves) {
                        private_moves[private_move_count++] = (VertexMove){v, best_dest, best_cut_change};
                    }
                }
                
                #pragma omp critical
                {
                    if (private_moves) { // Only if allocation succeeded
                        for (int i = 0; i < private_move_count && move_count < max_potential_moves; i++) {
                            moves[move_count++] = private_moves[i];
                        }
                    }
                }
                
                // Free thread-local resources
                if (connections) free(connections);
                if (private_moves) free(private_moves);
            }
            
            if (move_count > 0) {
                // Sort moves by cut improvement (most negative first)
                qsort(moves, move_count, sizeof(VertexMove), compare_vertex_moves);
                
                // Limit moves per iteration to control convergence
                int moves_to_make = (move_count < max_moves_per_iteration) ? 
                                   move_count : max_moves_per_iteration;
                
                // Apply the best moves that reduce edge cuts
                for (int i = 0; i < moves_to_make; i++) {
                    if (moves[i].cut_change >= 0) break; // Stop if no more improvements
                    
                    int v = moves[i].vertex;
                    int dest = moves[i].best_destination;
                    
                    // Safety checks
                    if (part_sizes[src_part] <= min_part_size) break;
                    if (part_sizes[dest] >= max_part_size) continue;
                    
                    // Verify vertex index
                    if (v >= 0 && v < graph->num_vertices) {
                        // Apply the move
                        partition[v] = dest;
                        part_sizes[src_part]--;
                        part_sizes[dest]++;
                        global_improvement = 1;
                        
                        // Update can_grow status if destination becomes full
                        if (part_sizes[dest] >= max_part_size) {
                            can_grow[dest] = 0;
                        }
                    }
                }
            }
            
            // Free moves array
            free(moves);
        }
        
        // Free can_grow array
        free(can_grow);
    }
    
    // Phase 3: Final balancing pass
    // Try to balance partition sizes without increasing cuts too much
    int balance_iterations = 0;
    int max_balance_iterations = 5;
    
    while (balance_iterations++ < max_balance_iterations) {
        // Find smallest and largest partitions
        int min_idx = 0, max_idx = 0;
        for (int p = 1; p < num_parts; p++) {
            if (part_sizes[p] < part_sizes[min_idx]) min_idx = p;
            if (part_sizes[p] > part_sizes[max_idx]) max_idx = p;
        }
        
        // Check if we need to balance
        if (part_sizes[max_idx] <= max_part_size && part_sizes[min_idx] >= min_part_size) {
            break; // Already balanced within constraints
        }
        
        // Try to move vertices from largest to smallest partition
        if (part_sizes[max_idx] > vertices_per_part && part_sizes[min_idx] < vertices_per_part) {
            int max_move_candidates = part_sizes[max_idx] - vertices_per_part;
            if (max_move_candidates <= 0) continue;
            
            // Dynamically allocate moves array
            VertexMove* moves = (VertexMove*)malloc(max_move_candidates * sizeof(VertexMove));
            if (!moves) {
                fprintf(stderr, "Memory allocation failed for moves array in phase 3\n");
                free(part_sizes);
                return;
            }
            
            int move_count = 0;
            
            #pragma omp parallel
            {
                VertexMove* private_moves = NULL;
                
                private_moves = (VertexMove*)malloc(max_move_candidates * sizeof(VertexMove));
                if (!private_moves) {
                    fprintf(stderr, "Thread memory allocation failed in phase 3\n");
                } else {
                    memset(private_moves, 0, max_move_candidates * sizeof(VertexMove));
                }
                
                int private_move_count = 0;
                
                #pragma omp for nowait
                for (int v = 0; v < graph->num_vertices; v++) {
                    if (!private_moves) continue; // Skip if allocation failed
                    
                    if (partition[v] != max_idx) continue;
                    
                    int cut_change = calculate_cut_change(graph, partition, v, max_idx, min_idx);
                    
                    if (private_move_count < max_move_candidates) {
                        private_moves[private_move_count++] = (VertexMove){v, min_idx, cut_change};
                    }
                }
                
                #pragma omp critical
                {
                    if (private_moves) { // Only if allocation succeeded
                        for (int i = 0; i < private_move_count && move_count < max_move_candidates; i++) {
                            moves[move_count++] = private_moves[i];
                        }
                    }
                }
                
                // Free thread-local resources
                if (private_moves) free(private_moves);
            }
            
            if (move_count > 0) {
                qsort(moves, move_count, sizeof(VertexMove), compare_vertex_moves);
                
                int moves_needed = (part_sizes[max_idx] - vertices_per_part) < 
                                 (vertices_per_part - part_sizes[min_idx]) ?
                                 (part_sizes[max_idx] - vertices_per_part) :
                                 (vertices_per_part - part_sizes[min_idx]);
                
                moves_needed = moves_needed < move_count ? moves_needed : move_count;
                
                for (int i = 0; i < moves_needed; i++) {
                    int v = moves[i].vertex;
                    // Verify vertex index
                    if (v >= 0 && v < graph->num_vertices) {
                        partition[v] = min_idx;
                        part_sizes[max_idx]--;
                        part_sizes[min_idx]++;
                    }
                }
            }
            
            // Free moves array
            free(moves);
        }
    }
    
    // Free dynamically allocated part_sizes array
    free(part_sizes);
}

int* greedy_partition(Graph* graph, float imbalance, int num_parts){
    int* partition = (int*)calloc(graph->num_vertices, sizeof(int));
    BoundryList boundry;
    int vertices_per_part = graph->num_vertices / num_parts;
    int max_part_size = (int)(vertices_per_part + ( graph->num_vertices * imbalance));

    //petla kolejnych partycji podzialu
    for(int part = 1; part < num_parts; part++){
        boundry.vertices = (int*)malloc(20 * sizeof(int));
        boundry.size = 0;
        boundry.capacity = 20;
        //znajdywanie dostepnych wierzcholkow dla danej partycji 
        int available_vertices = 0;
        for(int i = 0; i < graph->num_vertices; i++){
            if(partition[i] == 0) available_vertices++;
        }
        //brak wierzcholkow dla danej partycji
        if(available_vertices == 0){
            fprintf(stderr, "BRAK WIERZCHOLKOW DO PODZIALU DLA PARTYCJI %d - za duzo czesci", part);
            free(partition);
            free(boundry.vertices);
            exit(EXIT_FAILURE);
        }
        //wybor losowego wierzcholka rozpoczynajacego partycje
        int random_index = rand()%available_vertices;
        int start = -1;
        for(int i = 0, count = 0; i < graph->num_vertices; i++){
            if(partition[i] == 0){
                if(count == random_index){
                    start = i;
                    break;
                }
                count++;
            }
        }
        partition[start] = part;
        int current_size = 1;
        int target_size = vertices_per_part;
        if(target_size > available_vertices)
            target_size = available_vertices;
        if(target_size > max_part_size)
            target_size = max_part_size;
        //tworzenie granicy wzgledem poczatkowego wierzcholka
        create_boundry(graph, partition, &boundry, part);
        //petla wypelniajaca partycje
        while(current_size < target_size){
            int best_vertex = find_best_vertex(graph, &boundry, partition, part);
            //gdy nie znaleziono wierzcholka szukanie nowego punktu rozpoczecia (graf rozlaczny)
            if(best_vertex==-1){
                for(int i = 0; i < graph->num_vertices; i++){
                    if(partition[i]==0){
                        best_vertex = i;
                        break;
                    }
                }
            }
            //gdy kompletna lipa z wierzcholkami 
            if(best_vertex == -1)
                break;
            //dodanie wierzcholka przecinajacego najmniej krawedzi do danej partycji
            partition[best_vertex] = part;
            current_size++;
            update_boundry(graph, partition, &boundry, best_vertex);
        }
        free(boundry.vertices);
    }
    refine_partitions(graph, partition, num_parts, imbalance);
    return partition;
}

int multi_start_greedy_partition(Graph* graph, int* best_partition, float imbalance, int num_parts, int num_tries){
    int best_cuts = INT_MAX;

    #pragma omp parallel
    {
        // Proper initialization of thread-local data
        int* local_partition = NULL;
        int local_best_cuts = INT_MAX;
        
        // Ensure this allocation happens and is initialized
        int* local_best_partition = (int*)malloc(graph->num_vertices * sizeof(int));
        if (local_best_partition) {
            // Zero-initialize to prevent uninitialized value issues
            memset(local_best_partition, 0, graph->num_vertices * sizeof(int));
            
            #pragma omp for nowait
            for(int try = 0; try < num_tries; try++) {
                // Clean up previous iteration's allocation
                if(local_partition != NULL) {
                    free(local_partition);
                    local_partition = NULL;
                }
                
                local_partition = greedy_partition(graph, imbalance, num_parts);

                if(local_partition == NULL) {
                    fprintf(stderr, "CURRENT_PARTITION - NULL!\n");
                    continue; // Skip this iteration rather than exiting
                }

                int cuts = count_edge_cuts(graph, local_partition);
                if(cuts < local_best_cuts) {
                    local_best_cuts = cuts;
                    memcpy(local_best_partition, local_partition, graph->num_vertices*sizeof(int));
                }
            }
            
            // Synchronized update of global best partition
            #pragma omp critical
            {
                if (local_best_cuts < best_cuts) {
                    best_cuts = local_best_cuts;
                    memcpy(best_partition, local_best_partition, graph->num_vertices * sizeof(int));
                }
            }
            
            // Clean up all thread-local allocations
            free(local_best_partition);
        }
        
        if (local_partition != NULL) {
            free(local_partition);
        }
    }
    
    return best_cuts;
}

void display_partition(Graph* graph, int* best_partition, int num_parts, int best_cuts){
    for(int part = 0; part < num_parts; part++){
        int sum_part = 0;
        printf("PARTYCJA NR %d:\n\n", part);
        for(int n = 0; n < graph->num_vertices; n++){
            if(best_partition[n]==part){
                printf("wierzcholek nr %d\n", n);
                sum_part++;
            }
        }
        printf("WIERZCHOLKOW W PARTYCJI NR %d: %d\n\n", part, sum_part);
    }
    printf("ILOSC CIEC: %d", best_cuts);
}