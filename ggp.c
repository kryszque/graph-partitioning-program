#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <omp.h>
#include <time.h>
#include "ggp.h"
#include "processid.h"

void double_capacity(BoundryList* boundry, int* partition){
    int* new_vertices = realloc(boundry->vertices, boundry->capacity * 2 * sizeof(int));
    if (!new_vertices) {
        fprintf(stderr, "REALOKACJA BOUNDRY NIEUDANA!\n");
        free(boundry->vertices);
        free(partition);
        exit(EXIT_FAILURE);
    }
    boundry->vertices = new_vertices;
    boundry->capacity = boundry->capacity * 2;
}

void create_boundry(Graph* graph, int* partition, BoundryList* boundry, int current_part){
    #pragma omp parallel
    {
        //kazdy watek ma lokalne buffory
        int local_capacity = 128;
        int* local_vertices = (int*)malloc(local_capacity * sizeof(int));
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
    }
}

void update_boundry(Graph* graph, int* partition, BoundryList* boundry, int current_part, int removed_vertex){
    int removed_vertex_index = -1;
    for(int j = 0; j < boundry->size; j++){ //znajdywany jest index usunietego wierzcholka w strukturze boundry
        if(boundry->vertices[j] == removed_vertex){
            removed_vertex_index = j;
            break;
        }
    }
    if (removed_vertex_index == -1) {
        printf("WIERZCHOLEK %d NIE ZNALEZIONY W BOUNDRY LIST\n", removed_vertex);
        return;
    }
    //sledzony jest stan znalezionego wierzcholka do zastapienia usunietego w boundry
    int replacement_found = 0;
    //sprawdzanie wszystkich sasiadow usunietego wierzcholka w partycji 0
    for (int i = graph->xadj[removed_vertex]; i < graph->xadj[removed_vertex + 1]; i++) {
        int neighbour = graph->adjncy[i];
        if (partition[neighbour] == 0) {
            //sprawdzanie czy dany sasiad juz nie jest w granicy
            int already_in_boundary = 0;
            for (int k = 0; k < boundry->size; k++) {
                if (boundry->vertices[k] == neighbour) {
                    already_in_boundary = 1;
                    break;
                }
            }
            if (!already_in_boundary) {
                if (!replacement_found) {
                    //pierwszy znaleziony wierzcholek zastepuje usuniety w liscie
                    boundry->vertices[removed_vertex_index] = neighbour;
                    replacement_found = 1;
                } else {
                    //nastepne dodane sa na koncu listy
                    if (boundry->size == boundry->capacity)
                        double_capacity(boundry, partition);
                    boundry->vertices[boundry->size++] = neighbour;
                }
            }
        }
    }
    //jesli nie znaleziono sasiadow usuwany jest tylko usuniety wierzcholek
    if (!replacement_found) {
        for (int k = removed_vertex_index; k < boundry->size - 1; k++) {
            boundry->vertices[k] = boundry->vertices[k + 1];
        }
        boundry->size--;
    }
}

int calculate_cut_increase(Graph* graph, int* partition, int v, int current_part){
    int cut_increase = 0;
    //dla danego wierzcholka (kandydata do dodania do aktualnej partycji) sprawdzane są partycje jego sąsiadów 
    //i liczona ilość przeciętych krawędzi w momencie przeniesienia wierzchołka
    for(int i = graph->xadj[v]; i < graph->xadj[v+1]; i++){
        int neighbour = graph->adjncy[i];
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

    #pragma omp parallel
    {
        int local_best_vertex = -1;
        int local_min_cut_increase = INT_MAX;
        unsigned int seed = (unsigned int)(time(NULL) ^ omp_get_thread_num());
        #pragma omp for nowait
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
        #pragma omp critical
        {
            if (local_min_cut_increase < min_cut_increase ||
                (local_min_cut_increase == min_cut_increase && (simple_rand(&seed) % 2 == 0))) {
                min_cut_increase = local_min_cut_increase;
                best_vertex = local_best_vertex;
            }
        }
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

// For the refinement phase, calculate cut change when moving v from from_part to to_part
int calculate_cut_change(Graph* graph, int* partition, int v, int from_part, int to_part) {
    int cut_change = 0;
    
    for (int i = graph->xadj[v]; i < graph->xadj[v+1]; i++) {
        int neighbor = graph->adjncy[i];
        
        if (partition[neighbor] == from_part) {
            // Was not a cut, will become a cut
            cut_change++;
        }
        else if (partition[neighbor] == to_part) {
            // Was a cut, will no longer be a cut
            cut_change--;
        }
        // No change for neighbors in other partitions
    }
    
    return cut_change; // Positive means more cuts, negative means fewer cuts
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
    
    // Calculate current partition sizes
    int part_sizes[num_parts];
    memset(part_sizes, 0, sizeof(int) * num_parts);
    for (int i = 0; i < graph->num_vertices; i++) {
        part_sizes[partition[i]]++;
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
                
                VertexMove moves[move_array_size];
                int move_count = 0;
                
                #pragma omp parallel
                {
                    // Thread-local arrays for collecting moves
                    VertexMove private_moves[move_array_size];
                    int private_move_count = 0;
                    
                    #pragma omp for nowait
                    for (int v = 0; v < graph->num_vertices; v++) {
                        if (partition[v] != p) continue;
                        
                        int best_dest = -1;
                        int best_cut_change = INT_MAX;
                        
                        // Create a map of neighboring partitions for this vertex
                        int neighbor_parts[num_parts];
                        memset(neighbor_parts, 0, sizeof(int) * num_parts);
                        
                        for (int j = graph->xadj[v]; j < graph->xadj[v+1]; j++) {
                            int neighbor = graph->adjncy[j];
                            neighbor_parts[partition[neighbor]]++;
                        }
                        
                        // Find best destination partition
                        for (int d = 0; d < num_parts; d++) {
                            if (d == p) continue;
                            if (part_sizes[d] >= max_part_size) continue;
                            
                            // Calculate gain based on actual cut changes
                            int cut_change = calculate_cut_change(graph, partition, v, p, d);
                            
                            // Prioritize moves to partitions where the vertex has more neighbors
                            if (cut_change < best_cut_change || 
                                (cut_change == best_cut_change && neighbor_parts[d] > neighbor_parts[best_dest])) {
                                best_cut_change = cut_change;
                                best_dest = d;
                            }
                        }
                        
                        if (best_dest != -1 && private_move_count < move_array_size) {
                            private_moves[private_move_count++] = (VertexMove){v, best_dest, best_cut_change};
                        }
                    }
                    
                    #pragma omp critical
                    {
                        for (int i = 0; i < private_move_count && move_count < move_array_size; i++) {
                            moves[move_count++] = private_moves[i];
                        }
                    }
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
                        
                        partition[v] = dest;
                        part_sizes[p]--;
                        part_sizes[dest]++;
                        moves_made_total++;
                        
                        if (part_sizes[p] <= max_part_size) break;
                    }
                }
                
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
        int can_grow[num_parts];
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
            
            VertexMove moves[max_potential_moves];
            int move_count = 0;
            
            #pragma omp parallel
            {
                VertexMove private_moves[max_potential_moves];
                int private_move_count = 0;
                
                #pragma omp for nowait
                for (int v = 0; v < graph->num_vertices; v++) {
                    if (partition[v] != src_part) continue;
                    
                    int best_dest = -1;
                    int best_cut_change = 0; // Only consider moves that improve cuts
                    
                    // Track connectivity to each partition
                    int connections[num_parts];
                    memset(connections, 0, sizeof(int) * num_parts);
                    
                    // Count connections to each partition
                    for (int j = graph->xadj[v]; j < graph->xadj[v+1]; j++) {
                        int neighbor = graph->adjncy[j];
                        connections[partition[neighbor]]++;
                    }
                    
                    // Consider all possible destination partitions
                    for (int dest_part = 0; dest_part < num_parts; dest_part++) {
                        if (dest_part == src_part) continue;
                        if (!can_grow[dest_part]) continue;
                        
                        // Calculate gain when moving v from src_part to dest_part
                        int cut_change = connections[dest_part] - (connections[src_part] + 
                                        (graph->xadj[v+1] - graph->xadj[v]) - connections[dest_part] - connections[src_part]);
                        
                        if (cut_change < best_cut_change) {
                            best_cut_change = cut_change;
                            best_dest = dest_part;
                        }
                    }
                    
                    if (best_dest != -1 && best_cut_change < 0 && private_move_count < max_potential_moves) {
                        private_moves[private_move_count++] = (VertexMove){v, best_dest, best_cut_change};
                    }
                }
                
                #pragma omp critical
                {
                    for (int i = 0; i < private_move_count && move_count < max_potential_moves; i++) {
                        moves[move_count++] = private_moves[i];
                    }
                }
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
            VertexMove moves[max_move_candidates];
            int move_count = 0;
            
            #pragma omp parallel
            {
                VertexMove private_moves[max_move_candidates];
                int private_move_count = 0;
                
                #pragma omp for nowait
                for (int v = 0; v < graph->num_vertices; v++) {
                    if (partition[v] != max_idx) continue;
                    
                    int cut_change = calculate_cut_change(graph, partition, v, max_idx, min_idx);
                    
                    if (private_move_count < max_move_candidates) {
                        private_moves[private_move_count++] = (VertexMove){v, min_idx, cut_change};
                    }
                }
                
                #pragma omp critical
                {
                    for (int i = 0; i < private_move_count && move_count < max_move_candidates; i++) {
                        moves[move_count++] = private_moves[i];
                    }
                }
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
                    partition[v] = min_idx;
                    part_sizes[max_idx]--;
                    part_sizes[min_idx]++;
                }
            }
        }
    }
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
            update_boundry(graph, partition, &boundry, part, best_vertex);
        }
        free(boundry.vertices);
    }
    refine_partitions(graph, partition, num_parts, imbalance);
    return partition;
}

int multi_start_greedy_partition(Graph* graph, int* best_partition, float imbalance, int num_parts, int num_tries){
    int* current_partition = NULL;
    int best_cuts = INT_MAX;

    #pragma omp parallel
    {
        int* local_partition = NULL;
        int local_best_cuts = INT_MAX;
        int* local_best_partition = (int*)malloc(graph->num_vertices * sizeof(int));

        #pragma omp for nowait
        for(int try = 0; try < num_tries; try++){
            if(local_partition != NULL){
                free(local_partition);
            }
            local_partition = greedy_partition(graph, imbalance, num_parts);

            if(!local_partition){
                fprintf(stderr, "CURRENT_PARTITION - NULL!\n");
                exit(EXIT_FAILURE);
            }

            int cuts = count_edge_cuts(graph, local_partition);
            if(cuts < local_best_cuts){
                local_best_cuts = cuts;
                memcpy(local_best_partition, local_partition, graph->num_vertices*sizeof(int));
            }
        }
        // Zapisanie najlepszej wersji do globalnej zmiennej
        #pragma omp critical
        {
            if (local_best_cuts < best_cuts) {
                best_cuts = local_best_cuts;
                memcpy(best_partition, local_best_partition, graph->num_vertices * sizeof(int));
            }
        }

        free(local_partition);
        free(local_best_partition);
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