#ifndef OUTPUT_FILE_H
#define OUTPUT_FILE_H
#include "processid.h"

typedef struct{
    int* content;
    int size_indicies;
    int size_content;
    int capacity_content;
    int capacity_indecies;
    int* indecies;
} Line;

void doubleSize(Line l);
void doubleIndecies(Line l);
void getResult_txt(Graph* graph, LineContainer* container, int* partition, int num_parts, char* output_file);

#endif