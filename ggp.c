#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ggp.h"

void update_boundry(Graph* graph, int* partition, int* boundry, int current_part){
    //resetuje tablice boundry (moze da sie jakos zoptymalizowac, zeby nie usuwac calosci za kazdym razem)
    memset(boundry,0,graph->num_vertices*sizeof(int));

    //dla kazdego wierzcholka danej partycji do boundry dopisywany jest kazdy jego sasiad nalezacy do partycji 0
    for(int i = 0; i < graph->num_vertices; i++){
        if(partition[i] == current_part){
            for(int j = graph->xadj[i]; j < graph->xadj[i+1]; j++){
                int neighbour = graph->adjncy[j];
                if(partition[neighbour] == 0)
                    boundry[neighbour] = 1;
            }
        }
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

int count_edge_cuts(Graph* graph, int* partition){
    int cuts = 0;
    //obliczona zostaje calkowita ilosc przecietych krawedzi po podziale
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
    int vertices_per_part = graph->num_vertices / num_parts;
    int max_part_size = (int)(vertices_per_part * (1.0 + imbalance));
    int* boundry = (int*)calloc(graph->num_vertices, sizeof(int));

    //petla kolejnych partycji podzialu
    for(int part = 1; part < num_parts; part++){
        //znajdywanie dostepnych wierzcholkow dla danej partycji 
        int available_vertices = 0;
        for(int i = 0; i < graph->num_vertices; i++){
            if(partition[i] == 0) available_vertices++;
        }
        //brak wierzcholkow dla danej partycji
        if(available_vertices == 0){
            fprintf(stderr, "BRAK WIERZCHOLKOW DO PODZIALU DLA PARTYCJI %d", part);
            return NULL;
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
            }
            count++;
        }
        
        partition[start] = part;
        int current_size = 1;
        int target_size = vertices_per_part;
        if(target_size > available_vertices)
            target_size = available_vertices;
        if(target_size > max_part_size)
            target_size = max_part_size;
        //tworzenie granicy wzgledem poczatkowego wierzcholka
        update_boundry(graph, partition, boundry, part);
        //petla wypelniajaca partycje
        while(current_size < target_size){
            int best_vertex = -1;
            int min_cut_increase = INT_MAX;
            
            //szukanie najlepszego wierzcholka do dodania do aktualnej partycji 
            for(int i = 0; i < graph->num_vertices; i++){
                if(boundry[i]){
                    int cut_increase = calculate_cut_increase(graph, partition, i, part);
                    if(cut_increase < min_cut_increase || (cut_increase == min_cut_increase && rand()%2 == 0)){
                        min_cut_increase = cut_increase;
                        best_vertex = i;
                    }
                }
            }

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
            update_boundry(graph, partition, boundry, part);
        }
    }
    free(boundry);
    return partition;
}

void multi_start_greedy_partition(Graph* graph, int* best_partition, float imbalance, int num_parts, int num_tries){
    int* current_partition = NULL;
    int best_cuts = INT_MAX;

    //dla podanej ilosci prob uruchamiany jest ggp i zachowywany jest najlepszy wynik
    for(int try = 0; try < num_tries; try++){
        if(current_partition != NULL)
            free(current_partition);
        current_partition = greedy_partition(graph, imbalance, num_parts);
        int cuts = count_edge_cuts(graph, current_partition);
        if(cuts < best_cuts){
            best_cuts = cuts;
            memcpy(best_partition, current_partition, graph->num_vertices*sizeof(int));
        }
    }
    free(current_partition);
}