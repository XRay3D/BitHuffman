#include <QString>

#pragma once

//The Huffman tree node definition
struct htNode {
    char symbol;
    struct htNode *left, *right;
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
    char code;
    int size;
    struct hlNode* next;
};

//Incapsularea listei
struct hlTable {
    hlNode* first;
    hlNode* last;
};

htTree* buildTree(const QString& inputString);
hlTable* buildTable(htTree* huffmanTree);
QByteArray encode(hlTable* table, const QString& stringToEncode);
QString decode(htTree* tree, const QByteArray& stringToDecode);
