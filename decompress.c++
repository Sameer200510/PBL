#include<iostream>
#include<fstream>
#include<string>
#include<unordered_map>
#include<queue>
#include<bitset>
#include<cstdint>
#include<vector>
#include<iomanip>
#include<algorithm>
using namespace std;

// RLE decompression structure
struct RLESequence {
    unsigned char character;
    uint8_t count;
    
    RLESequence(unsigned char c, uint8_t n) : character(c), count(n) {}
};

// Decode RLE sequences back to original data
vector<unsigned char> runLengthDecode(const vector<RLESequence>& sequences) {
    vector<unsigned char> data;
    for (const auto& seq : sequences) {
        data.insert(data.end(), seq.count, seq.character);
    }
    return data;
}

// BitReader class for handling bit-level input
class BitReader {
private:
    ifstream& in;
    vector<uint8_t> inputBuffer;
    size_t bufferPos;
    uint8_t currentByte;
    int bitsLeft;
    bool endOfData;
    static const size_t BUFFER_SIZE = 8192;
    
public:
    BitReader(ifstream& input) : 
        in(input), bufferPos(0), currentByte(0), bitsLeft(0), endOfData(false) {
        inputBuffer.resize(BUFFER_SIZE);
    }
    
    bool readBit() {
        if (endOfData) return false;
        
        if (bitsLeft == 0) {
            if (bufferPos >= inputBuffer.size() || bufferPos == 0) {
                in.read(reinterpret_cast<char*>(inputBuffer.data()), BUFFER_SIZE);
                size_t bytesRead = in.gcount();
                if (bytesRead == 0) {
                    endOfData = true;
                    return false;
                }
                if (bytesRead < BUFFER_SIZE) {
                    inputBuffer.resize(bytesRead);
                }
                bufferPos = 0;
            }
            currentByte = inputBuffer[bufferPos++];
            bitsLeft = 8;
        }
        
        // Read bits in the same order as they were written (MSB to LSB)
        bool bit = (currentByte & (1 << (bitsLeft - 1))) != 0;
        bitsLeft--;
        return bit;
    }
    
    bool hasMore() const {
        return !endOfData;
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

void printHuffmanCodes(Node* root, string code = "") {
    if (!root) return;
    
    if (!root->left && !root->right) {
        root->code = code;
        if (root->character == '\n') cout << "\\n";
        else if (root->character == ' ') cout << "' '";
        else cout << "'" << root->character << "'";
        cout << " -> " << code << " (freq: " << root->freq << ")" << endl;
    }
    
    printHuffmanCodes(root->left, code + "0");
    printHuffmanCodes(root->right, code + "1");
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
    
    priority_queue<Node*, vector<Node*>, Compare> pq;
    for (Node* node : nodes) {
        pq.push(node);
    }
    
    while (pq.size() > 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();
        Node* parent = new Node('\0', left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        pq.push(parent);
    }
    
    return pq.top();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <compressed_file>" << endl;
        return 1;
    }
    
    string compressedFile = argv[1];
    string outputFile = compressedFile;
    // Remove .compressed extension if present
    size_t pos = outputFile.find(".compressed");
    if (pos != string::npos) {
        outputFile = outputFile.substr(0, pos);
    }
    outputFile += ".decompressed";
    
    cout << "Starting decompression..." << endl;
    cout << "Input file: " << compressedFile << endl;
    cout << "Output file: " << outputFile << endl;
    ifstream input(compressedFile, ios::binary);
    if (!input) {
        cout << "Error: Cannot open compressed file." << endl;
        return 2;
    }
    
    // Read frequency table size
    uint32_t tableSize;
    if (!input.read(reinterpret_cast<char*>(&tableSize), sizeof(tableSize))) {
        cout << "Error: Cannot read frequency table size" << endl;
        input.close();
        return 3;
    }
    cout << "Frequency table size: " << tableSize << endl;
    
    // Read frequency table
    unordered_map<unsigned char, int> freqMap;
    cout << "\nFrequency table:" << endl;
    for (uint32_t i = 0; i < tableSize; i++) {
        unsigned char c;
        uint32_t freq;
        if (!input.get(reinterpret_cast<char&>(c)) || 
            !input.read(reinterpret_cast<char*>(&freq), sizeof(freq))) {
            cout << "Error: Cannot read frequency table entry" << endl;
            input.close();
            return 4;
        }
        freqMap[c] = freq;
        if (c == '\n') cout << "\\n";
        else if (c == ' ') cout << "' '";
        else cout << "'" << c << "'";
        cout << " -> " << freq << endl;
    }
    
    // Read total bytes
    size_t totalBytes;
    if (!input.read(reinterpret_cast<char*>(&totalBytes), sizeof(totalBytes))) {
        cout << "Error: Cannot read total bytes" << endl;
        input.close();
        return 5;
    }
    cout << "\nTotal bytes to decode: " << totalBytes << endl;
    
    // Build Huffman tree
    cout << "\nBuilding Huffman tree..." << endl;
    Node* root = buildHuffmanTree(freqMap);
    if (!root) {
        cout << "Error: Failed to build Huffman tree" << endl;
        input.close();
        return 6;
    }
    
    cout << "\nHuffman codes:" << endl;
    printHuffmanCodes(root);
    
    // Open output file
    ofstream output(outputFile, ios::binary);
    if (!output) {
        cout << "Error: Cannot create output file" << endl;
        delete root;
        input.close();
        return 7;
    }
    
    // Read and decode data
    cout << "\nDecoding data..." << endl;
    BitReader reader(input);
    vector<unsigned char> buffer;
    buffer.reserve(8192);
    size_t bytesDecoded = 0;
    
    while (bytesDecoded < totalBytes && reader.hasMore()) {
        Node* current = root;
        while (current->left && current->right && reader.hasMore()) {
            bool bit = reader.readBit();
            current = bit ? current->right : current->left;
        }
        
        if (!current->left && !current->right) {
            buffer.push_back(current->character);
            bytesDecoded++;
            
            if (buffer.size() >= 8192) {
                output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                buffer.clear();
            }
            
            if (bytesDecoded % 1000000 == 0) {
                cout << "Decoded " << bytesDecoded << " bytes..." << endl;
            }
        }
    }
    
    if (!buffer.empty()) {
        output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    }
    
    input.close();
    output.close();
    delete root;
    
    cout << "Decompression complete!" << endl;
    cout << "Decoded " << bytesDecoded << " bytes" << endl;
    
    return 0;
}