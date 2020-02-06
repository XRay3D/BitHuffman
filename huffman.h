#pragma once

#include <QString>
//    try {
//        uint64_t testStr = 0xFEDCBA9876543210;
//        //We build the tree depending on the string
//        htTree* codeTree = buildTree(testStr);
//        //We build the table depending on the Huffman tree
//        hlTable* codeTable = buildTable(codeTree);
//        //We encode using the Huffman table
//        //encode(codeTable, testStr);
//        //We decode using the Huffman tree
//        //We can decode string that only use symbols from the initial string
//        qDebug() << (testStr == decode(codeTree, encode(codeTable, testStr)));
//        //        for (int i = 0; i < std::numeric_limits<int>::max(); ++i) {
//        //            if (i == decode(codeTree, encode(codeTable, i)))
//        //                qDebug() << i;
//        //            else
//        //                throw i;
//        //        }
//        //Output : 0011 1110 1011 0001 0010 1010 1100 1111 1000 1001
//    } catch (int e) {
//        qDebug() << "err" << e;
//        exit(0);
//    }
//    exit(0);
enum {
    Section = 2,
    Size = 1 << Section,
    Mask = (1 << Section) - 1
};

//The Huffman tree node definition
struct htNode {
    char symbol = 0;
    htNode* left = nullptr;
    htNode* right = nullptr;
};

/*
	We "encapsulate" the entire tree in a structure
	because in the future we might add fields like "size"
	if we need to. This way we don't have to modify the entire
	source code.
*/
struct htTree {
    htNode* root;
};

//The Huffman table nodes (linked list implementation)
struct hlNode {
    char symbol;
    uchar code;
    int size;
    struct hlNode* next;
};

//Incapsularea listei
struct hlTable {
    hlNode* first;
    hlNode* last;
};

int size(const uint64_t input);

htTree* buildTree(uint64_t inputString);
hlTable* buildTable(htTree* huffmanTree);
uint64_t encode(hlTable* table, const uint64_t stringToEncode);
uint64_t decode(htTree* tree, const uint64_t stringToDecode);
