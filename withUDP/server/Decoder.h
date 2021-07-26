#ifndef RECEIVER_H
#define RECEIVER_H

#include <chrono>
#include <unordered_set>
#include <deque>
#include <vector>
#include <unordered_set>
#include "globalParameters.h"
#include "Receiver.h"

class Decoder 
{
    public:
        // constructor
        Decoder(const string sp):savePath(sp), blocks(nullptr) {}
        // copy constrol : rule of three??
        ~Decoder();
        // public member functions
        void start();
    private:
        // member functions
        void decode();
        void decode_init();
        void writeBlocks(const symbol& sym, int neighbour);
        void XOR_twoBlocks(char *d1, char *d2);
        void reduce_neighbours(int);
        void removeSymbolFromBlock(symbol& sym, int symIndex, int blockIndex);
        void writeFile() const;

        // data member
        char **blocks;
        long filesize;
        long blocks_n;
        long receivedNumThreshold;
        long redecodeNumInterval;
        string filename;
        const string savePath;
        std::vector<int> stream; // name stream is for general sample algorithm
        Receiver receiver;

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

Decoder::~Decoder()
{
    if(blocks)
    {
        for(int i = 0; i < blocks_n; ++i)
            delete[] blocks[i];
        delete[] blocks;
    }
}

void Decoder::start()
{
    while(1)
    {
        receiveFirstSymForInit();
        receiveEnoughSyms();
        while(decode() == FAILURE)
        {
            receiveMoreSyms();
        }
        receiver.terminate();
        writeFile();
    }
}

void Decoder::receiveMoreSyms()
{
    decodeSyms = backupSyms;
    for(int i = 0; i < redecodeNumInterval; ++i)
    {
        symbol* sym = receiver.getSymbol();
        *(decodeSyms.begin() + i) = *sym;
        *(backupSyms.begin() + i) = *sym;
    }
}

void Decoder::receiveFirstSymForInit()
{
    symbol* firstSym = receiver.getSymbol();
    decodeSyms.push_back(*firstSym);
    backupSyms.push_back(*firstSym);
    // initialize data members
    filename = firstSym->filename;
    filesize = firstSym->filesize;
    blocks_n = filesize / PACKET_SIZE + 1;
    receivedNumThreshold = (long)(blocks_n * THRESHOLD_COEFFICIENT);
    redecodeNumInterval = (long)((double)receivedNumThreshold * 0.1) + 1;
    // initialize blocks 
    blocks = new char*[blocks_n];
    for(int i = 0; i < blocks_n; ++i)
        blocks[i] = new char[PACKET_SIZE];
    // initialize stream for reservoir_sampling algorithm
    stream.resize(blocks_n);
    std::iota(stream.begin(), stream.end(), 0);
}

inline
void Decoder::receiveEnoughSyms()
{
    decodeSyms.resize(receivedNumThreshold);
    backupSyms.resize(receivedNumThreshold);
    for(int i = 1; i < receivedNumThreshold; ++i)
    {
        symbol* sym = receiver.getSymbol();
        *(decodeSyms.begin() + i) = *sym;
        *(backupSyms.begin() + i) = *sym;
    }
}

void Decoder::decode_init(int receivedSymbolsN)
{
    // initialize/clear containers for decoding
    neighboursOfSymbols = std::vector<std::unordered_set<int>>(receivedSymbolsN);
    linkedSymIndexOfBlocks = std::vector<std::unordered_set<int>>(blocks_n);
    degOneSymIndex = std::deque<int>();

    // scan all decodeSyms to fill all containers
    for(int i = 0; i < receivedSymbolsN; ++i)
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
    std::cout << "decode initialize..." << std::endl;
    decode_init();
    
    /* decoding begin */
    std::cout << "decode begin..." << std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
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
            std::cout << "decode succeed!" << std::endl;
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::cout << "decode time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000 << "[ms]" << std::endl;
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
    string outFileFullName = savePath + filename;
    std::ofstream outFile(outFileFullName.c_str(), std::ofstream::binary);
    if(!outFile)
        throw std::runtime_error("can't open output file");
    for(int i = 0; i < blocks_n - 1; ++i)
        outFile.write(blocks[i], PACKET_SIZE);
    outFile.write(blocks[blocks_n - 1], filesize % PACKET_SIZE);
    outFile.close();
    std::cout << "have writed data to file" << outFileFullName << std::endl;
}

#endif
