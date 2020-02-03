#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QDebug>

#include "huffman.h"
#include "pQueue.h"

void traverseTree(htNode* treeNode, hlTable** table, int k, char code);

void traverseTree(htNode* treeNode, hlTable** table, int k, char code)
{
    //If we reach the end we introduce the code in the table
    if (treeNode->left == nullptr && treeNode->right == nullptr) {
        //qDebug() << "k" << k << static_cast<int>(code);
        //        code[k] = '\0';
        hlNode* aux = static_cast<hlNode*>(malloc(sizeof(hlNode)));
        //        aux->code = (char*)malloc(sizeof(char) * (strlen(code) + 1));
        //        strcpy(aux->code, code);
        aux->code = code;
        aux->size = k;
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
        code &= ~(1 << k);
        //code[k] = '0';
        traverseTree(treeNode->left, table, k + 1, code);
    }
    //We concatenate a 1 for each step to the right
    if (treeNode->right != nullptr) {
        code |= (1 << k);
        //code[k] = '1';
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
    char code = '\0';
    //k will memories the level on which the traversal is
    int k = 0;

    //We traverse the tree and calculate the codes
    traverseTree(huffmanTree->root, &table, k, code);
    return table;
}

htTree* buildTree(const QString& inputString)
{
    //The array in which we calculate the frequency of the symbols
    //Knowing that there are only 256 posibilities of combining 8 bits
    //(256 ASCII characters)
    int probability[256];

    //We initialize the array
    for (int i = 0; i < 256; i++)
        probability[i] = 0;

    //We consider the symbol as an array index and we calculate how many times each symbol appears

    for (int i = 0; i < inputString.size(); i++)
        probability[static_cast<unsigned char>(inputString[i].toLatin1())]++;

    //The queue which will hold the tree nodes
    pQueue* huffmanQueue;
    initPQueue(&huffmanQueue);

    //We create nodes for each symbol in the string
    for (int i = 0; i < 256; i++) {
        if (probability[i] != 0) {
            htNode* aux = static_cast<htNode*>(malloc(sizeof(htNode)));
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

QByteArray encode(hlTable* table, const QString& stringToEncode)
{
    QByteArray str;
    hlNode* traversal;

    qDebug() << "Encoding";
    qDebug() << "Input string :" << stringToEncode.size() << stringToEncode;
    //printf("\nEncoding\nInput string : %s\nEncoded string : \n", stringToEncode);

    uchar j = 0;

    //For each element of the string traverse the table
    //and once we find the symbol we output the code for it

    uint64_t c = 0;

    for (int i = 0; i < stringToEncode.size(); i++) {
        traversal = table->first;
        while (traversal->symbol != stringToEncode[i])
            traversal = traversal->next;
        {
            uchar code = traversal->code;
            //qDebug() << QString("%1 %2").arg(code, 8, 2, QChar('0')).arg(traversal->size);
            //str.append(traversal->code);
            //if ((j + traversal->size) <= 8) {
            c |= code << j;
            j += traversal->size;
            //qDebug() << QString("%1 A").arg(c, 8, 2, QChar('0'));
            //} else {
            //                uchar k = 8 - j;
            //                uchar t = traversal->size - k;
            //                c |= code >> t;
            //                //qDebug() << QString("%1 B1").arg(c, 8, 2, QChar('0'));
            //                str.append(c);
            //                qDebug() << QString("%1").arg(c, 8, 2, QChar('0'));
            //                c = code;
            //                c = (c << (8 - k)) & 0xFF;
            //                c >>= 8 - (k);
            //                j = t;
            qDebug() << QString("                 %1 B2").arg(c, 32, 2, QChar('0'));
            //            }
            //qDebug() << "";
        }
        //printf("%s", traversal->code);
    }
    //printf("\n");
    qDebug() << "Encoded string :" << str.size() << str.toHex().toUpper();
    return str;
}

QString decode(htTree* tree, const QByteArray& stringToDecode)
{
    QString str;
    htNode* traversal = tree->root;

    qDebug() << "Decoding";
    qDebug() << "Input string :" << stringToDecode;

    //printf("\nDecoding\nInput string : %s\nDecoded string : \n", stringToDecode);

    //For each "bit" of the string to decode
    //we take a step to the left for 0
    //or ont to the right for 1
    for (int i = 0; i < stringToDecode.size(); i++) {
        const char std = stringToDecode[i];
        //        for (int j = 0; j < 8; ++j) {
        //            if (traversal->left == nullptr && traversal->right == nullptr) {
        //                str.append(traversal->symbol);
        //                qDebug() << "Decoded string :" << str;
        //                //printf("%c", traversal->symbol);
        //                traversal = tree->root;
        //            }

        //            if ((std & ~(0b10000000 >> j)) == 0)
        //                traversal = traversal->left;

        //            if ((std & (0b10000000 >> j)) == 1)
        //                traversal = traversal->right;
        //        }

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
