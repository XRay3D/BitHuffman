#pragma once

#include "huffman.h"

//We modify the data type to hold pointers to Huffman tree nodes
#define TYPE htNode*

#define MAX_SZ 256

struct pQueueNode {
    TYPE val;
    int priority;
    struct pQueueNode* next;
};

struct pQueue {
    int size;
    pQueueNode* first;
};

void initPQueue(pQueue** queue);
void addPQueue(pQueue** queue, TYPE val, int priority);
TYPE getPQueue(pQueue** queue);
