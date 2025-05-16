#include <iostream>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <bitset>
using namespace std;

struct Node {
    char character;
    int freq;
    Node* left;
    Node* right;
    
    Node(char c, int f) : character(c), freq(f), left(nullptr), right(nullptr) {}
};

// Custom comparison function for priority queue (min-heap)
struct Compare {
    bool operator()(Node* left, Node* right) {
        return left->freq > right->freq; // Min-heap based on frequency
    }
};

// Function to assign Huffman codes to characters
void assignCodes(Node* root, const string& code, unordered_map<char, string>& huffmanCodes) {
    if (!root) return;
    if (!root->left && !root->right) {
        huffmanCodes[root->character] = code; // Leaf node
    }
    assignCodes(root->left, code + "0", huffmanCodes);
    assignCodes(root->right, code + "1", huffmanCodes);
}

// Function to compress the file
void compressFile(const string& inputFile, const string& outputFile) {
    // Open input file
    ifstream input(inputFile, ios::binary);
    if (!input) {
        cout << "Error opening input file." << endl;
        return;
    }

    // Count the frequency of each character
    unordered_map<char, int> freqMap;
    char ch;
    while (input.get(ch)) {
        freqMap[ch]++;
    }
    input.close();

    // Build the Huffman tree
    priority_queue<Node*, vector<Node*>, Compare> minHeap;
    for (const auto& pair : freqMap) {
        minHeap.push(new Node(pair.first, pair.second));
    }

    while (minHeap.size() > 1) {
        Node* left = minHeap.top(); minHeap.pop();
        Node* right = minHeap.top(); minHeap.pop();
        Node* parent = new Node('\0', left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        minHeap.push(parent);
    }
    
    Node* root = minHeap.top(); // Root of the Huffman tree
    
    // Generate Huffman codes for each character
    unordered_map<char, string> huffmanCodes;
    assignCodes(root, "", huffmanCodes);

    // Compress the input file
    input.open(inputFile, ios::binary);
    ofstream output(outputFile, ios::binary);
    string compressedBits = "";
    while (input.get(ch)) {
        compressedBits += huffmanCodes[ch];
    }

    // Add padding to make the bit stream a multiple of 8
    int padding = 8 - compressedBits.length() % 8;
    compressedBits = string(padding, '0') + compressedBits;

    // Write the padding and the compressed data to the output file
    output.put(static_cast<char>(padding)); // Write the padding
    for (size_t i = 0; i < compressedBits.length(); i += 8) {
        bitset<8> byte(compressedBits.substr(i, 8));
        output.put(static_cast<char>(byte.to_ulong()));
    }

    input.close();
    output.close();
    cout << "File compressed successfully!" << endl;
}

// Function to decompress the file
void decompressFile(const string& inputFile, const string& outputFile) {
    // Open the compressed file
    ifstream input(inputFile, ios::binary);
    if (!input) {
        cout << "Error opening input file." << endl;
        return;
    }

    // Read padding and the compressed bits
    int padding;
    input.get(reinterpret_cast<char&>(padding)); // Read padding
    string compressedBits = "";
    char byte;
    while (input.get(byte)) {
        bitset<8> bits(byte);
        compressedBits += bits.to_string();
    }

    // Remove the padding
    compressedBits = compressedBits.substr(padding);

    // Rebuild the Huffman tree (assumed to be saved or known)
    // For simplicity, assuming we know the codebook already (it should be transmitted with the compressed file in a real application)

    unordered_map<string, char> reverseHuffmanCodes;
    reverseHuffmanCodes["0"] = 'a'; // Placeholder for actual codebook

    // Decompress the data
    string code = "";
    ofstream output(outputFile, ios::binary);
    for (char bit : compressedBits) {
        code += bit;
        if (reverseHuffmanCodes.find(code) != reverseHuffmanCodes.end()) {
            output.put(reverseHuffmanCodes[code]);
            code = "";
        }
    }

    input.close();
    output.close();
    cout << "File decompressed successfully!" << endl;
}

int main() {
    string inputFile = "input.txt";  // Input file for compression
    string compressedFile = "output.bin"; // Compressed output file
    string decompressedFile = "decompressed.txt"; // Decompressed output file

    // Compress and decompress the file
    compressFile(inputFile, compressedFile);
    decompressFile(compressedFile, decompressedFile);

    return 0;
}
