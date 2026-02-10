# Huffman File Compression

Clean implementation of Huffman coding in C++.

## Build & Run

```bash
g++ -std=c++11 -O3 -o huffman huffman.cpp

# Compress (output saved in current directory)
./huffman -c /path/to/input.txt

# Decompress
./huffman -d compressed.huff

# Interactive mode
./huffman -i

# Auto-detect mode (compress .txt, decompress .huff)
./huffman /path/to/file.txt
```

## Features

✅ Automatic output naming (file.txt → file.huff)
✅ Output saved in current working directory
✅ File path from any location
✅ Interactive mode for user input
✅ Auto-detection based on extension
✅ Bit-level I/O
✅ Min-heap tree construction
✅ Tree serialization
✅ Compression metrics

## Usage Examples

```bash
# Compress file from any location
./huffman -c ~/Documents/report.txt
# Creates: report.huff (in current directory)

# Decompress
./huffman -d report.huff  
# Creates: report_restored (in current directory)

# Interactive mode
./huffman -i
# Prompts for: compress/decompress and file path

# Auto-detect
./huffman data.txt    # Compresses to data.huff
./huffman data.huff   # Decompresses to data_restored
```

## Algorithm

1. Count byte frequencies
2. Build min-heap priority queue
3. Construct Huffman tree (greedy)
4. Generate prefix-free codes
5. Encode data bit-by-bit

**Time**: O(n + k log k) where n = file size, k = unique bytes
**Space**: O(k) = O(1) since k ≤ 256

## File Format

```
[HUFF][size:8][tree:var][data:var]
```

## Resume Line

Developed file compressor in C++ using Huffman coding with bit-level I/O and compression benchmarking.