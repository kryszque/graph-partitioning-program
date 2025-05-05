#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processid.h"
#include "output_file.h"

// Changed to pass pointer to Line structure
void doubleSize(Line* l) {
    l->capacity_content *= 2;
    l->content = realloc(l->content, l->capacity_content * sizeof(int));
    if(!l->content) {
        fprintf(stderr, "REALOKACJA PAMIECI DLA LINE NIEUDANA!!\n");
        exit(EXIT_FAILURE);
    }
}

// Changed to pass pointer to Line structure
void doubleIndecies(Line* l) {
    l->capacity_indecies *= 2;
    l->indecies = realloc(l->indecies, l->capacity_indecies * sizeof(int));
    if(!l->indecies) {
        fprintf(stderr, "REALOKACJA PAMIECI DLA LINE NIEUDANA!!\n");
        exit(EXIT_FAILURE);
    }
}

void getResult_txt(Graph* graph, LineContainer* container, int* partition, int num_parts, char* output_file) {
    Line line4;
    Line line5;

    FILE *result_txt = fopen(output_file, "w");
    if(!result_txt) {
        fprintf(stderr, "NIE MOZNA OTWORZYC/UTWORZYC PLIKU Z DANYMI WYJSCIOWYMI!!\n");
        exit(EXIT_FAILURE);
    }
    //pierwsze 3 linijki mozna skopiowac z pliku wejsciowego
    //4 linijka adjncy
    //5 linijka xadj
    fprintf(result_txt, "%s\n", container->lines[0]);
    fprintf(result_txt, "%s\n", container->lines[1]);
    fprintf(result_txt, "%s\n", container->lines[2]);
    
    for(int part = 0; part < num_parts; part++) { //dla kazdej czesci (osobnego grafu)
        line4.capacity_content = 128;
        line5.capacity_content = 128;
        line5.capacity_indecies = 128;
        
        line4.content = (int*)malloc(line4.capacity_content * sizeof(int));
        line5.content = (int*)malloc(line5.capacity_content * sizeof(int));
        // Initialize line5.indecies which was missing
        line5.indecies = (int*)malloc(line5.capacity_indecies * sizeof(int));
        
        if (!line4.content || !line5.content || !line5.indecies) {
            fprintf(stderr, "ALOKACJA PAMIECI NIEUDANA!!\n");
            exit(EXIT_FAILURE);
        }
        
        line5.size_content = 0;
        line5.size_indicies = 0;
        line4.size_content = 0;
        
        for(int v = 0; v < graph->num_vertices; v++) { //przeszukiwanie wszystkich wierzcholkow
            if(partition[v] != part) continue;
            int count_v = 0;
            int temp = -1;
            
            for(int i = graph->xadj[v]; i < graph->xadj[v+1]; i++) { //sprawdzic czy sasiedzi tez sa w partition[part]
                if(partition[graph->adjncy[i]] == part) { //jesli sasiad nalezy do danej partycji
                    temp = line4.size_content; //zmienna przechowywujaca size line4 przed dodaniem sasiadow
                    if(line5.capacity_indecies == line5.size_indicies) {
                        doubleIndecies(&line5); // Pass address
                    }
                    line5.indecies[line5.size_indicies] = v; //index wierzcholka ktorego sasiadow rozpatrujemy 
                    line5.size_indicies++;
                    if(line4.size_content == line4.capacity_content) {
                        doubleSize(&line4); // Pass address
                    }
                    line4.content[line4.size_content] = graph->adjncy[i];
                    line4.size_content++;
                    count_v++;
                }
            }

            if(line5.size_content == line5.capacity_content) {
                doubleSize(&line5); // Pass address
            }
            line5.content[line5.size_content] = temp;
            line5.size_content++;
            
            if(line5.size_content == line5.capacity_content) {
                doubleSize(&line5); // Pass address
            }
            line5.content[line5.size_content] = temp + count_v;
            line5.size_content++;
        }
        
        // Print line4 content
        for(int num_l4 = 0; num_l4 < line4.size_content; num_l4++) {
            if(num_l4 != line4.size_content - 1) {
                fprintf(result_txt, "%d;", line4.content[num_l4]);
            } else {
                fprintf(result_txt, "%d", line4.content[num_l4]);
            }
        }
        
        fprintf(result_txt, "\n");
        for(int i = 0; i < line5.size_content; i += 2) {
            if(i > 0) fprintf(result_txt, ";");
            fprintf(result_txt, "%d", line5.content[i]);
        }
        
        // Free memory for each iteration
        free(line4.content);
        free(line5.content);
        free(line5.indecies);
    }
    
    fclose(result_txt);
}

void writeResultBinary(Graph* graph, LineContainer* container, int* partition, int num_parts, char* output_file) {
    Line line4;
    Line line5;
    char binary_filename[256];
    

    if (strstr(output_file, ".") != NULL) {
        strcpy(binary_filename, output_file);
        char* dot = strrchr(binary_filename, '.');
        strcpy(dot, ".bin");
    } else {
        sprintf(binary_filename, "%s.bin", output_file);
    }
    
    FILE *binary_file = fopen(binary_filename, "wb");
    if(!binary_file) {
        fprintf(stderr, "NIE MOZNA OTWORZYC/UTWORZYC PLIKU BINARNEGO!!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 3; i++) {
        size_t len = strlen(container->lines[i]);
        fwrite(&len, sizeof(size_t), 1, binary_file);
        fwrite(container->lines[i], sizeof(char), len, binary_file);
    }
    
    for(int part = 0; part < num_parts; part++) {
        line4.capacity_content = 128;
        line5.capacity_content = 128;
        line5.capacity_indecies = 128;
        
        line4.content = (int*)malloc(line4.capacity_content * sizeof(int));
        line5.content = (int*)malloc(line5.capacity_content * sizeof(int));
        line5.indecies = (int*)malloc(line5.capacity_indecies * sizeof(int));
        
        if (!line4.content || !line5.content || !line5.indecies) {
            fprintf(stderr, "ALOKACJA PAMIECI NIEUDANA!!\n");
            exit(EXIT_FAILURE);
        }
        
        line5.size_content = 0;
        line5.size_indicies = 0;
        line4.size_content = 0;
        
        for(int v = 0; v < graph->num_vertices; v++) {
            if(partition[v] != part) continue;
            int count_v = 0;
            int temp = -1;
            
            for(int i = graph->xadj[v]; i < graph->xadj[v+1]; i++) {
                if(partition[graph->adjncy[i]] == part) {
                    temp = line4.size_content;
                    if(line5.capacity_indecies == line5.size_indicies) {
                        doubleIndecies(&line5);
                    }
                    line5.indecies[line5.size_indicies] = v;
                    line5.size_indicies++;
                    if(line4.size_content == line4.capacity_content) {
                        doubleSize(&line4);
                    }
                    line4.content[line4.size_content] = graph->adjncy[i];
                    line4.size_content++;
                    count_v++;
                }
            }

            if(line5.size_content == line5.capacity_content) {
                doubleSize(&line5);
            }
            line5.content[line5.size_content] = temp;
            line5.size_content++;
            
            if(line5.size_content == line5.capacity_content) {
                doubleSize(&line5);
            }
            line5.content[line5.size_content] = temp + count_v;
            line5.size_content++;
        }
        
        // Write partition number
        fwrite(&part, sizeof(int), 1, binary_file);
        
        // Write line4 data (adjncy equivalent) in binary format
        fwrite(&line4.size_content, sizeof(int), 1, binary_file);
        fwrite(line4.content, sizeof(int), line4.size_content, binary_file);
        
        // Write line5 data (xadj equivalent) in binary format
        fwrite(&line5.size_content, sizeof(int), 1, binary_file);
        fwrite(line5.content, sizeof(int), line5.size_content, binary_file);
        
        // Free memory for each iteration
        free(line4.content);
        free(line5.content);
        free(line5.indecies);
    }
    
    fclose(binary_file);
    printf("Binary data successfully written to %s\n", binary_filename);
}