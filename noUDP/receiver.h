#ifndef RECEIVER_H
#define RECEIVER_H

#include <chrono>
#include <unordered_set>
#include <deque>
#include <vector>
#include <unordered_set>

class receiver 
{
    public:
        // constructor
        receiver(std::vector<symbol>&);
        // copy constrol
        ~receiver();
        // public member functions
        void decode();
        void writeFile(const char* outFileName) const;
    private:
        // member functions
        void decode_init();
        void writeBlocks(const symbol& sym, int neighbour);
        void XOR_twoBlocks(char *d1, char *d2);
        void reduce_neighbours(int);
        void removeSymbolFromBlock(symbol& sym, int symIndex, int blockIndex);

        // data member
        static int PACKET_SIZE;
        char **blocks;
        long filesize;
        long blocks_n;
        long receivedSymbolsN;
        std::vector<int> stream; // name stream is for general sample algorithm

        std::vector<symbol>& symbols;
        // maintain a queue of index of symbol with degree 1
        std::deque<int> degOneSymIndex;
        // maitntain a vector of set of all neighbours of these symbols
        std::vector<std::unordered_set<int>> neighboursOfSymbols;
        // maintain a vector of set of all linked symbols of blocks
        std::vector<std::unordered_set<int>> linkedSymIndexOfBlocks;
        // use a set to record the recovered block, if all done, decode succeed
        std::unordered_set<int> recoveredBlockIndex;
        // use a set to record the removed symbols,
        std::unordered_set<int> removedSymIndex;

};

int receiver::PACKET_SIZE = 32768;

receiver::receiver(std::vector<symbol>& sym) : symbols(sym), receivedSymbolsN(sym.size()), blocks(nullptr)
{
    if(receivedSymbolsN < 1) 
        throw std::runtime_error("didn't receive any symbols");
    // initialize parameter received
    filesize = symbols[0].filesize;
    blocks_n = filesize / PACKET_SIZE + 1;
    blocks = new char*[blocks_n];
    // initialize blocks 
    for(int i = 0; i < blocks_n; ++i)
    {
        blocks[i] = new char[PACKET_SIZE];
    }
    // initialize containers for decoding
    neighboursOfSymbols.resize(receivedSymbolsN);
    linkedSymIndexOfBlocks.resize(blocks_n, std::unordered_set<int>());
    // initialize stream for reservoir_sampling algorithm
    stream.resize(blocks_n);
    std::iota(stream.begin(), stream.end(), 0);
}

receiver::~receiver()
{
    if(blocks)
    {
        for(int i = 0; i < blocks_n; ++i)
            delete[] blocks[i];
        delete[] blocks;
    }
}

void receiver::decode_init()
{
    // scan all symbols to fill all containers
    for(int i = 0; i < receivedSymbolsN; ++i)
    {
        symbol& sym = symbols[i];
        if(sym.degree == 1)
            degOneSymIndex.push_back(i);
        int degree = sym.degree;
        for(int j = 0; j < degree; ++j)
        {
            int neighbour = sym.neighbours[j];
            linkedSymIndexOfBlocks[neighbour].insert(i);
            neighboursOfSymbols[i].insert(neighbour);
        }
    }
}

void receiver::decode()
{
    std::cout << "decode initialize..." << std::endl;
    decode_init();
    
    /* decoding begin */
    std::cout << "decode begin..." << std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    while(degOneSymIndex.size() > 0)
    {
        // pop out symbol index(if none, fail) and find this symbol in symbols 
        int symIndex = degOneSymIndex.front();
        degOneSymIndex.pop_front();
        // if found in removed set, skip all process
        if(removedSymIndex.find(symIndex) != removedSymIndex.end())
            continue;
        symbol& sym = symbols[symIndex];
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
            return;
        }
        removeSymbolFromBlock(sym, symIndex, blockIndex);
        // scan the neighbours to find all symbols linked to this block 
        reduce_neighbours(blockIndex);
    }
    throw std::runtime_error("there are no symbols with degree 1, decode fail!");
}

inline
void receiver::removeSymbolFromBlock(symbol& sym, int symIndex, int blockIndex)
{
    --sym.degree;
    removedSymIndex.insert(symIndex);
    linkedSymIndexOfBlocks[blockIndex].erase(symIndex);
}

// XOR their data, reduce their degree, if 1, add to the queue, back to step4
void receiver::reduce_neighbours(int blockIndex)
{
    std::unordered_set<int> tmp = linkedSymIndexOfBlocks[blockIndex];
    // find all symbols linked to this block
    for(int symIndex : tmp)
    {
        // delete this block from their neighbours set
        neighboursOfSymbols[symIndex].erase(blockIndex);

        symbol& sym = symbols[symIndex];
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

void receiver::writeBlocks(const symbol& sym, int blockIndex)
{
    if(blocks[blockIndex] == nullptr)
        throw std::runtime_error("you must initialize blocks for receiver!");
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

void receiver::XOR_twoBlocks(char *d1, char *d2)
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

void receiver::writeFile(const char* outFileName) const
{
    std::ofstream outFile(outFileName, std::ofstream::binary);
    if(!outFile)
        throw std::runtime_error("can't open output file");
    for(int i = 0; i < blocks_n - 1; ++i)
        outFile.write(blocks[i], PACKET_SIZE);
    outFile.write(blocks[blocks_n - 1], filesize % PACKET_SIZE);
    outFile.close();
}

#endif
