#ifndef DECODER_H
#define DECODER_H 

#include <chrono>
#include <unordered_set>
#include <deque>
#include <fstream>
#include <vector>
#include <unordered_set>
#include "globalParameters.h"
#include "./Receiver.h"

class Decoder 
{
    public:
        // constructor
        Decoder(const std::string sp):savePath(sp), blocks(nullptr), receivedSymN(0) {}
        // copy constrol : rule of three??
        ~Decoder() {free();}
        // public member functions
        void start();
    private:
        // member functions
        void receiveFirstSymForInit();
        void receiveSymbols(int start, int num);
        int decode();
        void decode_init();
        void writeBlocks(const symbol& sym, int neighbour);
        void XOR_twoBlocks(char *d1, char *d2);
        void reduce_neighbours(int);
        void removeSymbolFromBlock(symbol& sym, int symIndex, int blockIndex);
        void writeFile() const;
        void free();
        void resetDataMember();
        void resetDecoder();

        /* data members (never change) */
        const std::string savePath;
        Receiver receiver;
        /* data member (do not change during one iteration) */
        long filesize;
        long blocks_n;
        long receivedNumThreshold;
        long redecodeNumInterval;
        std::string filename;
        /* data members (might change if decode fails) */
        long receivedSymN;
        char **blocks;
        std::vector<symbol> decodeSyms;
        std::vector<symbol> backupSyms;
        // maintain a queue of index of symbol with degree 1
        std::deque<int> degOneSymIndex;
        // maitntain a vector of set of all neighbours of these decodeSyms
        std::vector<std::unordered_set<int>> neighboursOfSymbols;
        // maintain a vector of set of all linked decodeSyms of blocks
        std::vector<std::unordered_set<int>> linkedSymIndexOfBlocks;
        // use a set to record the recovered block, if all done, decode succeed
        std::unordered_set<int> recoveredBlockIndex;
        // use a set to record the removed decodeSyms,
        std::unordered_set<int> removedSymIndex;

};


void Decoder::start()
{
    while(1)
    {

        std::cout << "waiting for data from client..." << std::endl;
        receiveFirstSymForInit();
        std::cout << "receiving symbols..." << std::endl;
        receiveSymbols(1, receivedNumThreshold);

        std::cout << "have received " << receivedSymN << " symbols" 
            << ", start decoding" << std::endl;
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        while(decode() == FAILURE)
        {
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::cout << "decode fail, decode time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000 << "[ms]" << std::endl;
            std::cout << "try to receive more symbols" << std::endl;

            resetDataMember();
            receiveSymbols(receivedSymN, receivedSymN + redecodeNumInterval);

            std::cout << "currently received " << receivedSymN << " symbols, try decode again.." << std::endl;
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "decode succeed, decode time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000 << "[ms]" << std::endl;

        receiver.endIteration();
        writeFile();
        resetDecoder();
    }
}

inline
void Decoder::resetDataMember()
{
    decodeSyms = backupSyms; // recover symbols that have been modified
    // reset all blocks data
    for(int i = 0; i < blocks_n; ++i)
        memset(blocks[i], 0, sizeof(blocks[i]));
}

void Decoder::receiveFirstSymForInit()
{
    struct symbol* firstSym = receiver.getSymbol();
    // initialize data members (won't change in one iteration)
    filename = std::string(firstSym->filenameArray);
    filesize = firstSym->filesize;
    blocks_n = filesize / PACKET_SIZE + 1;
    receivedNumThreshold = (long)(blocks_n * THRESHOLD_COEFFICIENT);
    redecodeNumInterval = (long)((double)receivedNumThreshold * 0.1) + 1;
    // initialize blocks 
    blocks = new char*[blocks_n];
    for(int i = 0; i < blocks_n; ++i)
        blocks[i] = new char[PACKET_SIZE];
    // update some data members
    decodeSyms.push_back(*firstSym);
    backupSyms.push_back(*firstSym);
    receivedSymN += 1;
}

inline 
void Decoder::receiveSymbols(int start, int sum)
{
    decodeSyms.resize(sum);
    backupSyms.resize(sum);
    for(int i = start; i < sum; ++i)
    {
        symbol* sym = receiver.getSymbol();
        *(decodeSyms.begin() + i) = *sym;
        *(backupSyms.begin() + i) = *sym;
    }
    receivedSymN += sum - start;
}

void Decoder::decode_init()
{
    std::cout << "decode initialize..." << std::endl;
    // initialize containers for decoding
    neighboursOfSymbols = std::vector<std::unordered_set<int>>(receivedSymN);
    linkedSymIndexOfBlocks = std::vector<std::unordered_set<int>>(blocks_n);
    degOneSymIndex = std::deque<int>();
    recoveredBlockIndex = std::unordered_set<int>();
    removedSymIndex = std::unordered_set<int>();
    // scan all symbols to fill all containers
    for(int i = 0; i < receivedSymN; ++i)
    {
        symbol& sym = decodeSyms[i];
        if(sym.degree == 1)
            degOneSymIndex.push_back(i);
        int degree = sym.degree;
        for(int j = 0; j < degree && j < DISTRIBUTION_RANGE; ++j)
        {
            int neighbour = sym.neighbours[j];
            linkedSymIndexOfBlocks[neighbour].insert(i);
            neighboursOfSymbols[i].insert(neighbour);
        }
    }
}

int Decoder::decode()
{
    decode_init();
    /* decoding begin */
    while(degOneSymIndex.size() > 0)
    {
        // pop out symbol index(if none, fail) and find this symbol in decodeSyms 
        int symIndex = degOneSymIndex.front();
        degOneSymIndex.pop_front();
        // if found in removed set, skip all process
        if(removedSymIndex.find(symIndex) != removedSymIndex.end())
            continue;
        symbol& sym = decodeSyms[symIndex];
        // and then recover the block by this symbol
        int blockIndex = *neighboursOfSymbols[symIndex].begin();
        // found, this block has been recovered, skip all process
        if(recoveredBlockIndex.find(blockIndex) != recoveredBlockIndex.end())
            continue;
        writeBlocks(sym, blockIndex);
        recoveredBlockIndex.insert(blockIndex);
        if(recoveredBlockIndex.size() == blocks_n)
        {
            return SUCCESS;
        }
        removeSymbolFromBlock(sym, symIndex, blockIndex);
        // scan the neighbours to find all decodeSyms linked to this block 
        reduce_neighbours(blockIndex);
    }
    return FAILURE;
}

inline
void Decoder::removeSymbolFromBlock(symbol& sym, int symIndex, int blockIndex)
{
    --sym.degree;
    removedSymIndex.insert(symIndex);
    linkedSymIndexOfBlocks[blockIndex].erase(symIndex);
}

// XOR their data, reduce their degree, if 1, add to the queue, back to step4
void Decoder::reduce_neighbours(int blockIndex)
{
    std::unordered_set<int> tmp = linkedSymIndexOfBlocks[blockIndex];
    // find all decodeSyms linked to this block
    for(int symIndex : tmp)
    {
        // delete this block from their neighbours set
        neighboursOfSymbols[symIndex].erase(blockIndex);

        symbol& sym = decodeSyms[symIndex];
        if(sym.degree < 1)
            throw std::runtime_error("symbol with degree less than 1 shouldn't be linked to any blocks!!");
        else if(sym.degree == 1)
            removeSymbolFromBlock(sym, symIndex, blockIndex);
        else
        {
            XOR_twoBlocks(sym.data, blocks[blockIndex]);
            // degree--, if 1, add to the queue
            if(--sym.degree == 1)
                degOneSymIndex.push_back(symIndex);
        }
    }
}

void Decoder::writeBlocks(const symbol& sym, int blockIndex)
{
    if(blocks[blockIndex] == nullptr)
        throw std::runtime_error("you must initialize blocks for Decoder!");
    const char * symData = sym.data;
    char * blockData = blocks[blockIndex];
    const long * longSymData = reinterpret_cast<const long*>(symData);
    long * longBlockData = reinterpret_cast<long*>(blockData);
    for(int i = 0; i < PACKET_SIZE / 8; i += 4)
    {
        longBlockData[i] = longSymData[i];
        longBlockData[i + 1] = longSymData[i + 1];
        longBlockData[i + 2] = longSymData[i + 2];
        longBlockData[i + 3] = longSymData[i + 3];
    }
}

void Decoder::XOR_twoBlocks(char *d1, char *d2)
{
    // modern 64-bit cpu can do 64-bits XOR operation in one single operation
    long* newd1 = reinterpret_cast<long*>(d1);
    long* newd2 = reinterpret_cast<long*>(d2);
    for(int i = 0; i < PACKET_SIZE / 8; i += 4)
    {
        newd1[i] = newd1[i] ^ newd2[i];
        newd1[i + 1] = newd1[i + 1] ^ newd2[i + 1];
        newd1[i + 2] = newd1[i + 2] ^ newd2[i + 2];
        newd1[i + 3] = newd1[i + 3] ^ newd2[i + 3];
    }
}

void Decoder::writeFile() const
{
    std::string outFileFullName = savePath + filename;
    std::ofstream outFile(outFileFullName.c_str(), std::ofstream::binary);
    if(!outFile)
        throw std::runtime_error("can't open output file");
    for(int i = 0; i < blocks_n - 1; ++i)
        outFile.write(blocks[i], PACKET_SIZE);
    outFile.write(blocks[blocks_n - 1], filesize % PACKET_SIZE);
    outFile.close();
    std::cout << "have writed all data to file: " << outFileFullName << std::endl;
}

void Decoder::free()
{
    if(blocks)
    {
        for(int i = 0; i < blocks_n; ++i)
            delete[] blocks[i];
        delete[] blocks;
    }
}

void Decoder::resetDecoder()
{
    std::cout << "reset all data members for next iteration" << std::endl;
    std::cout << std::endl;
    free();
    blocks = nullptr;
    blocks_n = receivedNumThreshold = redecodeNumInterval = receivedSymN = 0;
    filename = std::string();
    decodeSyms = std::vector<symbol>();
    backupSyms = std::vector<symbol>();
    degOneSymIndex = std::deque<int>();
    neighboursOfSymbols = std::vector<std::unordered_set<int>>();
    linkedSymIndexOfBlocks = std::vector<std::unordered_set<int>>();
    recoveredBlockIndex = std::unordered_set<int>();
    removedSymIndex = std::unordered_set<int>();
}

#endif
