#include<iostream>
#include<fstream>
#include<unordered_map>
#include<queue>
#include<vector>
#include<string>
#include<bitset>
#include<iomanip>
#include<cstdint>
#include<algorithm>
using namespace std;

class BitWriter {
private:
    ofstream& out;
    uint8_t buffer;
    int bitsUsed;
    vector<uint8_t> outputBuffer;
    static const size_t BUFFER_SIZE = 8192; // 8KB buffer
    
public:
    BitWriter(ofstream& output) : out(output), buffer(0), bitsUsed(0) {
        outputBuffer.reserve(BUFFER_SIZE);
    }
    
    void writeBit(bool bit) {
        buffer |= (bit ? 1 : 0) << (7 - bitsUsed);
        bitsUsed++;
        
        if (bitsUsed == 8) {
            outputBuffer.push_back(buffer);
            if (outputBuffer.size() >= BUFFER_SIZE) {
                out.write(reinterpret_cast<const char*>(outputBuffer.data()), outputBuffer.size());
                outputBuffer.clear();
            }
            buffer = 0;
            bitsUsed = 0;
        }
    }
    
    void writeBits(const string& bits) {
        for (char bit : bits) {
            writeBit(bit == '1');
        }
    }
    
    void flush() {
        if (bitsUsed > 0) {
            outputBuffer.push_back(buffer);
        }
        if (!outputBuffer.empty()) {
            out.write(reinterpret_cast<const char*>(outputBuffer.data()), outputBuffer.size());
            outputBuffer.clear();
        }
    }
};

struct Node {
    unsigned char character;
    int freq;
    Node* left;
    Node* right;
    string code;  // Added to store the code for each node
    
    Node(unsigned char c, int f): character(c), freq(f), left(nullptr), right(nullptr) {}
    
    ~Node() {
        delete left;
        delete right;
    }
};

struct Compare {
    bool operator()(Node* a, Node* b) {
        if (a->freq == b->freq) {
            // If frequencies are equal, compare characters to ensure consistent ordering
            if (!a->left && !a->right && !b->left && !b->right) {
                return a->character > b->character;
            }
            // Internal nodes come after leaf nodes
            if (!a->left && !a->right) return false;
            if (!b->left && !b->right) return true;
            return false;
        }
        return a->freq > b->freq;
    }
};

void buildHuffmanCodes(Node* root, string code, unordered_map<unsigned char, string>& huffmanCodes) {
    if (!root) return;
    
    if (!root->left && !root->right) {
        root->code = code;
        huffmanCodes[root->character] = code.empty() ? "0" : code;
        return;
    }
    
    buildHuffmanCodes(root->left, code + "0", huffmanCodes);
    buildHuffmanCodes(root->right, code + "1", huffmanCodes);
}

void printNode(Node* node, string prefix = "") {
    if (!node) return;
    
    if (!node->left && !node->right) {
        if (node->character == '\n') cout << prefix << "\\n";
        else if (node->character == ' ') cout << prefix << "' '";
        else cout << prefix << "'" << node->character << "'";
        cout << " (freq: " << node->freq << ")" << endl;
    } else {
        cout << prefix << "Internal (freq: " << node->freq << ")" << endl;
    }
    
    if (node->left) {
        cout << prefix << "L: ";
        printNode(node->left, prefix + "   ");
    }
    if (node->right) {
        cout << prefix << "R: ";
        printNode(node->right, prefix + "   ");
    }
}

Node* buildHuffmanTree(const unordered_map<unsigned char, int>& freqMap) {
    if (freqMap.empty()) return nullptr;
    
    // Create a vector of nodes and sort them to ensure deterministic ordering
    vector<Node*> nodes;
    for (const auto& pair : freqMap) {
        nodes.push_back(new Node(pair.first, pair.second));
    }
    
    // Sort nodes by frequency and character to ensure deterministic ordering
    sort(nodes.begin(), nodes.end(), [](Node* a, Node* b) {
        if (a->freq == b->freq) {
            return a->character < b->character;
        }
        return a->freq < b->freq;
    });
    
    priority_queue<Node*, vector<Node*>, Compare> minHeap;
    for (Node* node : nodes) {
        minHeap.push(node);
    }
    
    while (minHeap.size() > 1) {
        Node* left = minHeap.top(); minHeap.pop();
        Node* right = minHeap.top(); minHeap.pop();
        Node* parent = new Node('\0', left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        minHeap.push(parent);
    }
    
    return minHeap.empty() ? nullptr : minHeap.top();
}

void displayCompressionStats(const string& inputFile, const string& outputFile, 
                           const unordered_map<unsigned char, string>& huffmanCodes) {
    ifstream input(inputFile, ios::binary | ios::ate);
    ifstream output(outputFile, ios::binary | ios::ate);
    
    if (input.is_open() && output.is_open()) {
        long originalSize = input.tellg();
        long compressedSize = output.tellg();
        
        cout << "\n=== Compression Statistics ===" << endl;
        cout << "Original size: " << originalSize << " bytes" << endl;
        cout << "Compressed size: " << compressedSize << " bytes" << endl;
        cout << "Compression ratio: " << fixed << setprecision(2) 
             << (double)compressedSize / originalSize * 100 << "%" << endl;
        cout << "Space saved: " << originalSize - compressedSize << " bytes" << endl;
        
        cout << "\nHuffman Codes:" << endl;
        for (const auto& pair : huffmanCodes) {
            unsigned char c = pair.first;
            if (c == '\n') cout << "\\n";
            else if (c == '\t') cout << "\\t";
            else if (c == ' ') cout << "' '";
            else cout << "'" << c << "'";
            cout << " -> " << pair.second << endl;
        }
    }
    
    input.close();
    output.close();
}

void compressFile(const string& inputFile, const string& outputFile) {
    cout << "Starting compression..." << endl;
    
    // Read input file
    ifstream input(inputFile, ios::binary);
    if (!input) {
        cout << "Error: Cannot open input file." << endl;
        return;
    }
    
    // Read file in chunks and build frequency table
    cout << "Building frequency table..." << endl;
    unordered_map<unsigned char, int> freqMap;
    vector<unsigned char> fileData;
    const size_t CHUNK_SIZE = 8192;
    char buffer[CHUNK_SIZE];
    size_t totalBytes = 0;
    
    while (input.read(buffer, CHUNK_SIZE)) {
        for (size_t i = 0; i < CHUNK_SIZE; i++) {
            unsigned char c = static_cast<unsigned char>(buffer[i]);
            freqMap[c]++;
            fileData.push_back(c);
        }
        totalBytes += CHUNK_SIZE;
    }
    size_t lastChunkSize = input.gcount();
    for (size_t i = 0; i < lastChunkSize; i++) {
        unsigned char c = static_cast<unsigned char>(buffer[i]);
        freqMap[c]++;
        fileData.push_back(c);
    }
    totalBytes += lastChunkSize;
    
    input.close();
    cout << "Read " << totalBytes << " bytes" << endl;
    cout << "Found " << freqMap.size() << " unique characters" << endl;
    
    cout << "\nFrequency table:" << endl;
    for (const auto& pair : freqMap) {
        if (pair.first == '\n') cout << "\\n";
        else if (pair.first == ' ') cout << "' '";
        else cout << "'" << pair.first << "'";
        cout << " -> " << pair.second << endl;
    }
    
    // Build Huffman tree and codes
    cout << "\nBuilding Huffman tree..." << endl;
    Node* root = buildHuffmanTree(freqMap);
    unordered_map<unsigned char, string> huffmanCodes;
    buildHuffmanCodes(root, "", huffmanCodes);
    
    cout << "\nHuffman tree structure:" << endl;
    printNode(root);
    
    cout << "\nHuffman codes:" << endl;
    for (const auto& pair : huffmanCodes) {
        if (pair.first == '\n') cout << "\\n";
        else if (pair.first == ' ') cout << "' '";
        else cout << "'" << pair.first << "'";
        cout << " -> " << pair.second << endl;
    }
    
    // Open output file and write header
    ofstream output(outputFile, ios::binary);
    if (!output) {
        cout << "Error: Cannot create output file." << endl;
        delete root;
        return;
    }
    
    // Write frequency table size and entries
    uint32_t tableSize = freqMap.size();
    output.write(reinterpret_cast<const char*>(&tableSize), sizeof(tableSize));
    
    for (const auto& pair : freqMap) {
        output.put(static_cast<char>(pair.first));
        uint32_t freq = pair.second;
        output.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
    }
    
    // Write file size
    output.write(reinterpret_cast<const char*>(&totalBytes), sizeof(totalBytes));
    
    // Write compressed data
    cout << "\nWriting compressed data..." << endl;
    BitWriter writer(output);
    size_t processedBytes = 0;
    
    for (unsigned char c : fileData) {
        writer.writeBits(huffmanCodes[c]);
        processedBytes++;
        if (processedBytes % 1000000 == 0) {
            cout << "Processed " << processedBytes << " bytes..." << endl;
        }
    }
    
    writer.flush();
    output.close();
    delete root;
    
    displayCompressionStats(inputFile, outputFile, huffmanCodes);
    cout << "Compression complete!" << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }
    
    string inputFile = argv[1];
    string compressedFile = inputFile + ".compressed";
    
    cout << "Compressing " << inputFile << " to " << compressedFile << endl;
    compressFile(inputFile, compressedFile);
    
    return 0;
}