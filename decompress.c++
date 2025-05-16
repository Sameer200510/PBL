#include<iostream>
#include<fstream>
#include<string>
using namespace std;

// Function to convert decimal to binary
string decToBin(int decimal) {
    string binary = "";
    for (int i = 7; i >= 0; --i) {
        binary += (decimal & (1 << i)) ? '1' : '0';
    }
    return binary;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "Usage: ./a.out <input file>\n";
        return 1;
    }

    // Open the input file in binary mode
    ifstream inputFile(argv[1], ios::binary);
    if (!inputFile) {
        cout << "Error opening input file\n";
        return 2;
    }

    // Generate output file name by replacing "-compressed" with "-decompressed"
    string inFile = argv[1];
    string outFile = inFile.substr(0, inFile.find("-compressed")) + "-decompressed.txt";

    // Create the output file
    ofstream outputFile(outFile, ios::binary);
    if (!outputFile) {
        cout << "Error creating output file\n";
        return 3;
    }

    // Read the file extension length and the extension
    char buff;
    inputFile.read(&buff, 1);
    int extLength = buff - '0';
    string extension = "";
    for (int i = 0; i < extLength; ++i) {
        inputFile.read(&buff, 1);
        extension += buff;
    }
    outFile += extension;

    // Read the Huffman codes (mapping)
    string huffmanCode;
    char previousChar = '\0';
    unordered_map<string, char> decodeMap;

    while (inputFile.read(&buff, 1)) {
        if (buff != '\0') {
            huffmanCode += buff;
        } else {
            decodeMap[huffmanCode] = previousChar;
            huffmanCode = "";
        }
        previousChar = buff;
    }

    // Read padding info
    inputFile.read(&buff, 1);
    int padding = buff - '0';

    // Decode the actual compressed data
    string binaryStr = "";
    while (inputFile.read(&buff, 1)) {
        binaryStr += decToBin(buff);
    }

    // Remove padding
    binaryStr = binaryStr.substr(padding);

    // Decode binary string into original characters
    string tempCode = "";
    for (char bit : binaryStr) {
        tempCode += bit;
        if (decodeMap.find(tempCode) != decodeMap.end()) {
            outputFile.put(decodeMap[tempCode]);
            tempCode = "";
        }
    }

    cout << "Decompression successful.\n";
    inputFile.close();
    outputFile.close();
    return 0;
}
