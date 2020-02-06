#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QDebug>

#include "huffman.h"
#include "pQueue.h"

void traverseTree(htNode* treeNode, hlTable*& table, int k, uchar code);

void traverseTree(htNode* treeNode, hlTable*& table, int k, uchar code)
{
    //If we reach the end we introduce the code in the table
    if (treeNode->left == nullptr && treeNode->right == nullptr) {
        hlNode* aux = new hlNode;
        qDebug() << QString("%1").arg(code, k, 2, QChar('0')) << static_cast<int>(treeNode->symbol);
        aux->code = code;
        aux->size = k;
        aux->symbol = treeNode->symbol;
        aux->next = nullptr;
        if (table->first == nullptr) {
            table->first = aux;
            table->last = aux;
        } else {
            table->last->next = aux;
            table->last = aux;
        }
        return;
    }

    //We concatenate a 0 for each step to the left
    if (treeNode->left != nullptr) {
        code &= ~(1 << k); //code[k] = '0';
        traverseTree(treeNode->left, table, k + 1, code);
    }
    //We concatenate a 1 for each step to the right
    if (treeNode->right != nullptr) {
        code |= (1 << k); //code[k] = '1';
        traverseTree(treeNode->right, table, k + 1, code);
    }
}

hlTable* buildTable(htTree* huffmanTree)
{
    //We initialize the table
    hlTable* table = new hlTable;
    table->first = nullptr;
    table->last = nullptr;

    //Auxiliary variables
    uchar code = 0;
    //k will memories the level on which the traversal is
    int k = 0;

    //We traverse the tree and calculate the codes
    traverseTree(huffmanTree->root, table, k, code);
    return table;
}

htTree* buildTree(uint64_t inputString)
{
    //The array in which we calculate the frequency of the symbols
    //Knowing that there are only Size posibilities of combining 8 bits
    //(Size ASCII characters)
    int probability[Size]{ 0 }; //We initialize the array

    //We consider the symbol as an array index and we calculate how many times each symbol appears
    for (uint i = 0; i < 64 / Section; ++i)
        ++probability[(inputString >>= Section) & Mask];

    //The queue which will hold the tree nodes
    pQueue* huffmanQueue;
    initPQueue(&huffmanQueue);

    //We create nodes for each symbol in the string
    for (int i = 0; i < Size; i++) {
        if (probability[i] != 0) {
            htNode* aux = new htNode;
            aux->left = nullptr;
            aux->right = nullptr;
            aux->symbol = static_cast<char>(i);
            addPQueue(&huffmanQueue, aux, probability[i]);
        }
    }

    //We apply the steps described in the article to build the tree
    while (huffmanQueue->size != 1) {
        int priority = huffmanQueue->first->priority;
        priority += huffmanQueue->first->next->priority;
        htNode* left = getPQueue(&huffmanQueue);
        htNode* right = getPQueue(&huffmanQueue);
        htNode* newNode = new htNode;
        newNode->left = left;
        newNode->right = right;
        addPQueue(&huffmanQueue, newNode, priority);
    }

    //We create the tree
    htTree* tree = new htTree;

    tree->root = getPQueue(&huffmanQueue);

    return tree;
}

static int s = 0;

uint64_t encode(hlTable* table, const uint64_t stringToEncode)
{
    QByteArray str;
    hlNode* traversal;

    //qDebug() << "Encoding";
    //qDebug() << "Input string :  " << QString("%1").arg(stringToEncode, 1, 2, QChar('0'));
    //printf("\nEncoding\nInput string : %s\nEncoded string : \n", stringToEncode);

    //For each element of the string traverse the table
    //and once we find the symbol we output the code for it
    uint64_t data = 0;
    s = size(stringToEncode);
    //qDebug() << s;
    int64_t k = 0;
    for (int i = 0; i < 64 / Section; ++i /* += Section*/) {
        traversal = table->first;
        while (traversal->symbol != ((stringToEncode >> i * Section) & Mask))
            traversal = traversal->next;
        data |= static_cast<uint64_t>(traversal->code) << k;
        k += traversal->size;
    }
    s = size(data);
    //qDebug() << s;

    //printf("\n");
    //qDebug() << "Encoded string :" << QString("%1").arg(data, 1, 2, QChar('0'));
    return data;
}

uint64_t decode(htTree* tree, const uint64_t stringToDecode)
{
    uint64_t str = 0;
    htNode* traversal = tree->root;

    //qDebug() << "Decoding";

    //qDebug() << "Input string :  " << QString("%1").arg(stringToDecode, 1, 2, QChar('0'));
    //For each "bit" of the string to decode
    //we take a step to the left for 0 or ont to the right for 1
    int c = 0;
    for (uint64_t i = 0x1; c < 64 / Section; i <<= 1) {
        if (traversal->left == nullptr && traversal->right == nullptr) {
            str |= static_cast<uint64_t>(traversal->symbol) << c * Section;
            traversal = tree->root;
            ++c;
        }

        if (stringToDecode & i) // 1
            traversal = traversal->right;
        else //        if (stringToDecode[i] == '0')
            traversal = traversal->left;
    }

    if (traversal->left == nullptr && traversal->right == nullptr) {
        str |= static_cast<uint64_t>(traversal->symbol) << c * Section;
        traversal = tree->root;
    }
    //qDebug() << "Decoded string :" << QString("%1").arg(str, 1, 2, QChar('0'));
    //qDebug() << size(str);
    return str;
}

int size(uint64_t input)
{
    int s = 0;
    for (int i = 0; i < 64 / Section; ++i) {
        if ((input >>= Section) & Mask)
            s = i;
    }
    return s;
}
