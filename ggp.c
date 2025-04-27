#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <omp.h>
#include <time.h>
#include "ggp.h"

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
        printf("Warning: Vertex %d not found in boundary list\n", removed_vertex);
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