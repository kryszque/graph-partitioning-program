#include <stdio.h>
#include <stdlib.h>
#include "ggp.h"

void greedy_partition(Graph* graph, float imbalance, int num_parts){
    int * partition = (int*)calloc(graph->num_vertices, sizeof(int));
    int vertices_per_part = graph->num_vertices / num_parts;
    int max_part_size = (int)(vertices_per_part * (1.0 + imbalance));

    //petla partycji podzialu
    for(int part = 1; part < num_parts; part){
        //znajdywanie dostepnych wierzcholkow dla danej partycji 
        int available_vertices = 0;
        for(int i = 0; i < graph->num_vertices; i++){
            if(partition[i] == 0) available_vertices++;
        }
        //brak wierzcholkow dla danej partycji
        if(available_vertices == 0){
            fprintf(stderr, "BRAK WIERZCHOLKOW DO PODZIALU DLA PARTYCJI %d", part);
            return;
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
        int *boundry = (int*)calloc(graph->num_vertices, sizeof(int));
        //update_boundry()
        //petla wypelniajaca partycje
        while(current_size < target_size){
            int best_vertex = -1;
            int min_cut_increase = INT_MAX;
            
            //szukanie najlepszego wierzcholka do dodania do aktualnej partycji 
            for(int i = 0; i < graph->num_vertices; i++){
                if(boundry[i]){
                    int cut_increase; //= calculate_cut_increase()
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
            //update_boundry()
        }
        free(boundry);
    }
}