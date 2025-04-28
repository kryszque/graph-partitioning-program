#include <stdio.h>
#include <stdlib.h>
#include "processid.h"
#include "output_file.h"


void doubleSize(Line l){
    l.capacity_content *= 2;
    l.content = realloc(l.content, l.capacity_content);
    if(!l.content){
        fprintf(stderr, "REALOKACJA PAMIECI DLA LINE NIEUDANA!!\n");
        exit(EXIT_FAILURE);
    }
}

void doubleIndecies(Line l){
    l.capacity_indecies *= 2;
    l.indecies = realloc(l.indecies, l.capacity_indecies);
    if(!l.indecies){
        fprintf(stderr, "REALOKACJA PAMIECI DLA LINE NIEUDANA!!\n");
        exit(EXIT_FAILURE);
    }
}

void getResult_txt(Graph* graph, LineContainer* container, int* partition, int num_parts, char* output_file){
    Line line4;
    Line line5;

    FILE *result_txt = fopen(output_file, "a");
    if(!result_txt){
        fprintf(stderr, "NIE MOZNA OTWORZYC/UTWORZYC PLIKU Z DANYMI WYJSCIOWYMI!!\n");
        exit(EXIT_FAILURE);
    }
    //pierwsze 3 linijki mozna skopiowac z pliku wejsciowego
    //4 linijka adjncy
    //5 linijka xadj
    fprintf(result_txt, "%s", container->lines[0]);
    fprintf(result_txt, "%s", container->lines[1]);
    fprintf(result_txt, "%s", container->lines[2]);
    for(int part = 0; part < num_parts; part++){ //dla kazdej czesci (osobnego grafu)
        line4.capacity_content = line5.capacity_content = line5.capacity_indecies = 128;
        line4.content = (int*)malloc(line4.capacity_content * sizeof(int));
        line5.content = (int*)malloc(line5.capacity_content * sizeof(int));
        line5.size_content = line5.size_indicies = line4.size_content = 0;
        for(int v = 0; v < graph->num_vertices; v++){//przeszukiwanie wszystkich wierzcholkow
            if(partition[v] != part) continue;
            int count_v = 0;
            int temp = -1;
            
            for(int i = graph->xadj[v]; i < graph->xadj[v+1]; i++){ //sprawdzic czy sasiedzi tez sa w partition[part]
                if(partition[graph->adjncy[i]]==part){//jesli sasiad nalezy do danej partycji
                    temp = line4.size_content; //zmienna przechowywujaca size line4 przed dodaniem sasiadow
                    if(line5.capacity_indecies == line5.size_indicies){
                        doubleIndecies(line5);
                    }
                    line5.indecies[line5.size_indicies] = v;//index wierzcholka ktorego sasiadow rozpatrujemy 
                    line5.size_indicies++;
                    if(line4.size_content == line4.capacity_content){
                        doubleSize(line4);
                    }
                    line4.content[line4.size_content] = graph->adjncy[i];
                    line4.size_content++;
                    count_v++;
                }
            }

            if(line5.size_content == line5.capacity_content){
                doubleSize(line5);
            }
            line5.content[line5.size_content] = temp;
            line5.size_content++;
            
            if(line5.size_content == line5.capacity_content){
                    doubleSize(line5);
            }
            line5.content[line5.size_content] = temp + count_v;
            line5.size_content++;
            
        }
        for(int num_l4 = 0; num_l4<line4.size_content; num_l4++){//wypisywanie linii 4
            if(num_l4 != line4.size_content-1){
                fprintf(result_txt, "%d;", line4.content[num_l4]);
            }
            else{
                fprintf(result_txt, "%d", line4.content[num_l4]);
            }
        }
        int ctr_l5 = 0;
        for(int num_l5 = 0; num_l5 < line5.indecies[line5.size_indicies]; num_l5++){//wypisywanie linii 5
            if(num_l5 == line5.indecies[ctr_l5] && num_l5 != line5.indecies[line5.size_indicies]-1){
                fprintf(result_txt, "%d;", line5.content[ctr_l5]);
                ctr_l5++;
            }
            else if(num_l5 == line5.indecies[ctr_l5] && num_l5 == line5.indecies[line5.size_indicies]-1){
                fprintf(result_txt, "%d", line5.content[ctr_l5]);
                ctr_l5++;
            }
            else if(num_l5 != line5.indecies[ctr_l5] && num_l5 != line5.indecies[line5.size_indicies]-1){
                fprintf(result_txt, "-1;");
                ctr_l5++;
            }
            else if(num_l5 != line5.indecies[ctr_l5] && num_l5 == line5.indecies[line5.size_indicies]-1){
                fprintf(result_txt, "-1");
                ctr_l5++;
            }
        }
        free(line4.content);
        free(line5.content);
        free(line5.indecies);
    }
    fclose(result_txt);
}
