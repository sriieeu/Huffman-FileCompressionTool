#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <sys/stat.h>

struct HuffmanNode {
    unsigned char symbol;
    unsigned int frequency;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(unsigned char sym, unsigned int freq)
        : symbol(sym), frequency(freq), left(nullptr), right(nullptr) {}

    HuffmanNode(std::shared_ptr<HuffmanNode> l, std::shared_ptr<HuffmanNode> r)
        : symbol(0), frequency(l->frequency + r->frequency), left(l), right(r) {}

    bool isLeaf() const {
        return left == nullptr && right == nullptr;
    }
};

struct CompareNode {
    bool operator()(const std::shared_ptr<HuffmanNode>& a,
                   const std::shared_ptr<HuffmanNode>& b) const {
        return a->frequency > b->frequency;
    }
};

class BitWriter {
private:
    std::ofstream& output;
    unsigned char buffer;
    int bitCount;
    size_t totalBits;

public:
    BitWriter(std::ofstream& out)
        : output(out), buffer(0), bitCount(0), totalBits(0) {}

    void writeBit(bool bit) {
        if (bit) buffer |= (1 << (7 - bitCount));
        bitCount++;
        totalBits++;
        if (bitCount == 8) {
            output.put(buffer);
            buffer = 0;
            bitCount = 0;
        }
    }

    void writeBits(const std::string& bits) {
        for (char bit : bits) writeBit(bit == '1');
    }

    void writeByte(unsigned char byte) {
        for (int i = 7; i >= 0; i--) writeBit((byte >> i) & 1);
    }

    void flush() {
        if (bitCount > 0) output.put(buffer);
    }

    size_t getTotalBits() const { return totalBits; }
};

class BitReader {
private:
    std::ifstream& input;
    unsigned char buffer;
    int bitCount;
    bool endOfFile;

public:
    BitReader(std::ifstream& in)
        : input(in), buffer(0), bitCount(0), endOfFile(false) {}

    bool readBit() {
        if (bitCount == 0) {
            int byte = input.get();
            if (byte == EOF) {
                endOfFile = true;
                return false;
            }
            buffer = static_cast<unsigned char>(byte);
            bitCount = 8;
        }
        bool bit = (buffer >> (bitCount - 1)) & 1;
        bitCount--;
        return bit;
    }

    unsigned char readByte() {
        unsigned char byte = 0;
        for (int i = 0; i < 8; i++) byte = (byte << 1) | (readBit() ? 1 : 0);
        return byte;
    }

    bool isEOF() const { return endOfFile && bitCount == 0; }
};

class HuffmanCodec {
private:
    std::shared_ptr<HuffmanNode> root;
    std::unordered_map<unsigned char, std::string> encodingTable;
    std::unordered_map<unsigned char, unsigned int> frequencyTable;

    std::string getFileName(const std::string& path) {
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            return path.substr(lastSlash + 1);
        }
        return path;
    }

    std::string removeExtension(const std::string& filename) {
        size_t lastDot = filename.find_last_of('.');
        if (lastDot != std::string::npos) {
            return filename.substr(0, lastDot);
        }
        return filename;
    }

    bool fileExists(const std::string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    void buildFrequencyTable(const std::vector<unsigned char>& data) {
        frequencyTable.clear();
        for (unsigned char byte : data) frequencyTable[byte]++;
    }

    void buildHuffmanTree() {
        std::priority_queue<std::shared_ptr<HuffmanNode>,
                          std::vector<std::shared_ptr<HuffmanNode>>,
                          CompareNode> minHeap;

        for (const auto& pair : frequencyTable) {
            minHeap.push(std::make_shared<HuffmanNode>(pair.first, pair.second));
        }

        if (minHeap.size() == 1) {
            auto node = minHeap.top();
            minHeap.pop();
            auto dummy = std::make_shared<HuffmanNode>(0, 0);
            root = std::make_shared<HuffmanNode>(node, dummy);
            return;
        }

        while (minHeap.size() > 1) {
            auto left = minHeap.top(); minHeap.pop();
            auto right = minHeap.top(); minHeap.pop();
            auto parent = std::make_shared<HuffmanNode>(left, right);
            minHeap.push(parent);
        }

        root = minHeap.top();
    }

    void generateEncodingTable(std::shared_ptr<HuffmanNode> node, const std::string& code) {
        if (!node) return;
        if (node->isLeaf()) {
            encodingTable[node->symbol] = code.empty() ? "0" : code;
            return;
        }
        generateEncodingTable(node->left, code + "0");
        generateEncodingTable(node->right, code + "1");
    }

    void serializeTree(std::shared_ptr<HuffmanNode> node, BitWriter& writer) {
        if (!node) return;
        if (node->isLeaf()) {
            writer.writeBit(1);
            writer.writeByte(node->symbol);
        } else {
            writer.writeBit(0);
            serializeTree(node->left, writer);
            serializeTree(node->right, writer);
        }
    }

    std::shared_ptr<HuffmanNode> deserializeTree(BitReader& reader) {
        bool isLeaf = reader.readBit();
        if (isLeaf) {
            unsigned char symbol = reader.readByte();
            return std::make_shared<HuffmanNode>(symbol, 0);
        } else {
            auto left = deserializeTree(reader);
            auto right = deserializeTree(reader);
            return std::make_shared<HuffmanNode>(left, right);
        }
    }

    std::string formatBytes(size_t bytes) const {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024.0 && unitIndex < 3) {
            size /= 1024.0;
            unitIndex++;
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
        return oss.str();
    }

public:
    std::string compress(const std::string& inputFile) {
        std::string fileName = getFileName(inputFile);
        std::string outputFile = removeExtension(fileName) + ".huff";

        if (!fileExists(inputFile)) {
            std::cerr << "Error: File not found: " << inputFile << std::endl;
            return "";
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        std::ifstream input(inputFile, std::ios::binary);
        if (!input) {
            std::cerr << "Error: Cannot open file: " << inputFile << std::endl;
            return "";
        }

        std::vector<unsigned char> data((std::istreambuf_iterator<char>(input)),
                                        std::istreambuf_iterator<char>());
        input.close();

        if (data.empty()) {
            std::cerr << "Error: File is empty" << std::endl;
            return "";
        }

        size_t originalSize = data.size();

        buildFrequencyTable(data);
        buildHuffmanTree();
        encodingTable.clear();
        generateEncodingTable(root, "");

        std::ofstream output(outputFile, std::ios::binary);
        if (!output) {
            std::cerr << "Error: Cannot create: " << outputFile << std::endl;
            return "";
        }

        BitWriter writer(output);

        output.write("HUFF", 4);
        output.write(reinterpret_cast<const char*>(&originalSize), sizeof(size_t));

        serializeTree(root, writer);

        for (unsigned char byte : data) {
            writer.writeBits(encodingTable[byte]);
        }

        writer.flush();
        output.close();

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;

        size_t compressedSize = std::ifstream(outputFile, std::ios::binary | std::ios::ate).tellg();
        double ratio = (1.0 - static_cast<double>(compressedSize) / originalSize) * 100.0;

        std::cout << "\n✓ COMPRESSION COMPLETE\n";
        std::cout << "Input:      " << inputFile << "\n";
        std::cout << "Output:     " << outputFile << " (in current directory)\n";
        std::cout << "Original:   " << formatBytes(originalSize) << "\n";
        std::cout << "Compressed: " << formatBytes(compressedSize) << "\n";
        std::cout << "Saved:      " << std::fixed << std::setprecision(1) << ratio << "%\n";
        std::cout << "Time:       " << std::setprecision(3) << elapsed.count() << "s\n";

        return outputFile;
    }

    std::string decompress(const std::string& inputFile) {
        std::string fileName = getFileName(inputFile);
        std::string outputFile = removeExtension(fileName);

        if (outputFile.length() > 5 && outputFile.substr(outputFile.length() - 5) != ".huff") {
            outputFile += "_restored";
        }

        if (!fileExists(inputFile)) {
            std::cerr << "Error: File not found: " << inputFile << std::endl;
            return "";
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        std::ifstream input(inputFile, std::ios::binary);
        if (!input) {
            std::cerr << "Error: Cannot open file: " << inputFile << std::endl;
            return "";
        }

        char magic[4];
        input.read(magic, 4);
        if (std::strncmp(magic, "HUFF", 4) != 0) {
            std::cerr << "Error: Invalid file format (not a .huff file)" << std::endl;
            return "";
        }

        size_t originalSize;
        input.read(reinterpret_cast<char*>(&originalSize), sizeof(size_t));

        BitReader reader(input);
        root = deserializeTree(reader);

        std::ofstream output(outputFile, std::ios::binary);
        if (!output) {
            std::cerr << "Error: Cannot create: " << outputFile << std::endl;
            return "";
        }

        auto currentNode = root;
        size_t decodedBytes = 0;

        while (decodedBytes < originalSize && !reader.isEOF()) {
            bool bit = reader.readBit();
            currentNode = bit ? currentNode->right : currentNode->left;
            if (currentNode->isLeaf()) {
                output.put(currentNode->symbol);
                decodedBytes++;
                currentNode = root;
            }
        }

        output.close();
        input.close();

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;

        std::cout << "\n✓ DECOMPRESSION COMPLETE\n";
        std::cout << "Input:  " << inputFile << "\n";
        std::cout << "Output: " << outputFile << " (in current directory)\n";
        std::cout << "Size:   " << formatBytes(originalSize) << "\n";
        std::cout << "Time:   " << std::fixed << std::setprecision(3) << elapsed.count() << "s\n";

        return outputFile;
    }
};

void printUsage(const char* progName) {
    std::cout << "Huffman Compression Tool\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " -c <file>     Compress file\n";
    std::cout << "  " << progName << " -d <file>     Decompress file\n";
    std::cout << "  " << progName << " -i            Interactive mode\n";
    std::cout << "  " << progName << " <file>        Auto-detect and process\n\n";
    std::cout << "Output is saved in current directory\n";
}

void interactiveMode() {
    HuffmanCodec codec;
    std::string choice, filePath;

    std::cout << "\n=== Huffman Compression Tool ===\n";
    std::cout << "\n1. Compress file\n";
    std::cout << "2. Decompress file\n";
    std::cout << "Choice (1/2): ";
    std::getline(std::cin, choice);

    std::cout << "Enter file path: ";
    std::getline(std::cin, filePath);

    if (filePath.empty()) {
        std::cerr << "Error: No file path provided\n";
        return;
    }

    if (choice == "1") {
        codec.compress(filePath);
    } else if (choice == "2") {
        codec.decompress(filePath);
    } else {
        std::cerr << "Error: Invalid choice\n";
    }
}

int main(int argc, char* argv[]) {
    HuffmanCodec codec;

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string option = argv[1];

    if (option == "-i" || option == "--interactive") {
        interactiveMode();
        return 0;
    }

    std::string filePath = argv[argc - 1];

    if (option == "-c" || option == "--compress") {
        std::string output = codec.compress(filePath);
        return output.empty() ? 1 : 0;
    }
    else if (option == "-d" || option == "--decompress") {
        std::string output = codec.decompress(filePath);
        return output.empty() ? 1 : 0;
    }
    else {
        std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
        if (ext == "huff") {
            std::string output = codec.decompress(filePath);
            return output.empty() ? 1 : 0;
        } else {
            std::string output = codec.compress(filePath);
            return output.empty() ? 1 : 0;
        }
    }

    return 0;
}