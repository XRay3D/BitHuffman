#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QDebug>

#include "huffman.h"
#include "pQueue.h"

void traverseTree(htNode* treeNode, hlTable** table, int k, char code[256])
{
    //If we reach the end we introduce the code in the table
    if (treeNode->left == nullptr && treeNode->right == nullptr) {
        code[k] = '\0';
        hlNode* aux = (hlNode*)malloc(sizeof(hlNode));
        aux->code = (char*)malloc(sizeof(char) * (strlen(code) + 1));
        strcpy(aux->code, code);
        aux->symbol = treeNode->symbol;
        aux->next = nullptr;
        if ((*table)->first == nullptr) {
            (*table)->first = aux;
            (*table)->last = aux;
        } else {
            (*table)->last->next = aux;
            (*table)->last = aux;
        }
    }

    //We concatenate a 0 for each step to the left
    if (treeNode->left != nullptr) {
        code[k] = '0';
        traverseTree(treeNode->left, table, k + 1, code);
    }
    //We concatenate a 1 for each step to the right
    if (treeNode->right != nullptr) {
        code[k] = '1';
        traverseTree(treeNode->right, table, k + 1, code);
    }
}

hlTable* buildTable(htTree* huffmanTree)
{
    //We initialize the table
    hlTable* table = (hlTable*)malloc(sizeof(hlTable));
    table->first = nullptr;
    table->last = nullptr;

    //Auxiliary variables
    char code[256];
    //k will memories the level on which the traversal is
    int k = 0;

    //We traverse the tree and calculate the codes
    traverseTree(huffmanTree->root, &table, k, code);
    return table;
}

htTree* buildTree(const char* inputString)
{
    //The array in which we calculate the frequency of the symbols
    //Knowing that there are only 256 posibilities of combining 8 bits
    //(256 ASCII characters)
    int* probability = (int*)malloc(sizeof(int) * 256);

    //We initialize the array
    for (int i = 0; i < 256; i++)
        probability[i] = 0;

    //We consider the symbol as an array index and we calculate how many times each symbol appears
    for (int i = 0; inputString[i] != '\0'; i++)
        probability[(unsigned char)inputString[i]]++;

    //The queue which will hold the tree nodes
    pQueue* huffmanQueue;
    initPQueue(&huffmanQueue);

    //We create nodes for each symbol in the string
    for (int i = 0; i < 256; i++)
        if (probability[i] != 0) {
            htNode* aux = (htNode*)malloc(sizeof(htNode));
            aux->left = nullptr;
            aux->right = nullptr;
            aux->symbol = (char)i;

            addPQueue(&huffmanQueue, aux, probability[i]);
        }

    //We free the array because we don't need it anymore
    free(probability);

    //We apply the steps described in the article to build the tree
    while (huffmanQueue->size != 1) {
        int priority = huffmanQueue->first->priority;
        priority += huffmanQueue->first->next->priority;

        htNode* left = getPQueue(&huffmanQueue);
        htNode* right = getPQueue(&huffmanQueue);

        htNode* newNode = (htNode*)malloc(sizeof(htNode));
        newNode->left = left;
        newNode->right = right;

        addPQueue(&huffmanQueue, newNode, priority);
    }

    //We create the tree
    htTree* tree = (htTree*)malloc(sizeof(htTree));

    tree->root = getPQueue(&huffmanQueue);

    return tree;
}

QString encode(hlTable* table, const char* stringToEncode)
{
    QString str;
    hlNode* traversal;

    qDebug() << "Encoding";
    qDebug() << "Input string :" << stringToEncode;
    //printf("\nEncoding\nInput string : %s\nEncoded string : \n", stringToEncode);

    //For each element of the string traverse the table
    //and once we find the symbol we output the code for it
    for (int i = 0; stringToEncode[i] != '\0'; i++) {
        traversal = table->first;
        while (traversal->symbol != stringToEncode[i])
            traversal = traversal->next;
        str.append(traversal->code);
        //printf("%s", traversal->code);
    }
    //printf("\n");
    qDebug() << "Encoded string :" << str;
    return str;
}

QString decode(htTree* tree, const char* stringToDecode)
{
    QString str;
    htNode* traversal = tree->root;

    qDebug() << "Decoding";
    qDebug() << "Input string :" << stringToDecode;

    //printf("\nDecoding\nInput string : %s\nDecoded string : \n", stringToDecode);

    //For each "bit" of the string to decode
    //we take a step to the left for 0
    //or ont to the right for 1
    for (int i = 0; stringToDecode[i] != '\0'; i++) {
        if (traversal->left == nullptr && traversal->right == nullptr) {
            str.append(traversal->symbol);
            //printf("%c", traversal->symbol);
            traversal = tree->root;
        }

        if (stringToDecode[i] == '0')
            traversal = traversal->left;

        if (stringToDecode[i] == '1')
            traversal = traversal->right;

        if (stringToDecode[i] != '0' && stringToDecode[i] != '1') {
            qDebug() << "The input string is not coded correctly!";
            //printf("The input string is not coded correctly!\n");
            return {};
        }
    }

    if (traversal->left == nullptr && traversal->right == nullptr) {
        str.append(traversal->symbol);
        //printf("%c", traversal->symbol);
        traversal = tree->root;
    }

    //printf("\n");
    qDebug() << "Decoded string :" << str;
    return str;
}
