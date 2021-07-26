#ifndef SENDER_H
#define SENDER_H

#include <sys/stat.h>
#include <fstream>
#include <unordered_set>
#include <string.h>
#include <random>
#include <vector>
#include "distribution.h"

const int COMMON_PACKET_SIZE = 32768;
const int COMMON_DISTRIBUTION_RANGE = 25;

struct symbol
{
    symbol(int i, int deg, long fz) : symbolID(i), degree(deg), filesize(fz) {}
    int symbolID; // index in generated symbols, used as seed of random number
    int degree; // how many blocks used to generate this symbol
    long filesize; // size of file
    int neighbours[COMMON_DISTRIBUTION_RANGE]; // all neighbours of this symbol
    char data[COMMON_PACKET_SIZE]; // data contents of this symbol
};

class sender 
{
    friend struct test;
    friend struct symbol;
    public:
        // constructor
        sender(const char* f) : _filename(f), blocks(nullptr), filesize(0), blocks_n(0) {}
        // copy control: rule of three
        sender(const sender& s);
        sender& operator=(const sender& s);
        ~sender();
        // member functions
        void read_file();
        std::vector<symbol> encode(double);
        long get_blocks_quantity() const {return blocks_n;}

    private:
        // member functions
        std::vector<int> get_degrees(int symbols_n) const;
        std::vector<int> get_indices(int , int) const;
        std::vector<int> reservoir_sampling(int, int) const;
        void XOR_twoBlocks(char *d1, char *d2) const;
        void get_filesize(const char* _filename);
        // data member 
        static int PACKET_SIZE;
        static int DISTRIBUTION_RANGE;

        const char* _filename;
        char **blocks;
        long filesize;
        long blocks_n;
        std::vector<int> stream; // name stream is for general sample algorithm
};

int sender::PACKET_SIZE = COMMON_PACKET_SIZE;
int sender::DISTRIBUTION_RANGE = 25;


sender::sender(const sender& s) : _filename(s._filename), blocks_n(s.blocks_n), filesize(s.filesize)
{
    char **newblocks = new char*[blocks_n];
    for(int i = 0; i < blocks_n; ++i)
    {
        newblocks[i] = new char[PACKET_SIZE];
        for(int i = 0; i < PACKET_SIZE; ++i)
        {
            newblocks[i][i] = blocks[i][i];
        }
    }
    blocks = newblocks;
}

sender& sender::operator=(const sender& s) 
{
    if(&s != this)
    {
        _filename = s._filename;
        blocks_n = s.blocks_n;
        filesize = s.filesize;
        char **newblocks = new char*[blocks_n];
        for(int i = 0; i < blocks_n; ++i)
        {
            newblocks[i] = new char[PACKET_SIZE];
            for(int i = 0; i < PACKET_SIZE; ++i)
            {
                newblocks[i][i] = blocks[i][i];
            }
        }
        blocks = newblocks;
    }
    return *this;
}

sender::~sender()
{
    if(blocks)
    {
        for(int i = 0; i < blocks_n; ++i)
            delete[] blocks[i];
        delete[] blocks;
    }
}

std::vector<int> sender::get_degrees(int symbols_n) const
{
    std::vector<int> random_degrees(symbols_n, 0);
    // entry of decoding
    random_degrees[0] = 1;

    std::vector<double> probabilities = robust_distribution(DISTRIBUTION_RANGE);
    std::vector<long> discreteD(DISTRIBUTION_RANGE, 0);
    // convert double to long for discrete discrete distribution
    for(int i = 0; i < DISTRIBUTION_RANGE; ++i)
        discreteD[i] = lround(1e6 * probabilities[i]);

    std::discrete_distribution<long> ranD(discreteD.begin(), discreteD.end());
    std::default_random_engine e;

    for(int i = 1; i < symbols_n; ++i)
        random_degrees[i] = ranD(e);

    return random_degrees;
}

void sender::get_filesize(const char* _filename)
{
    struct stat f_stat;
    if(stat(_filename, &f_stat) == -1)
        throw std::runtime_error("can't not access " + std::string(_filename));
    if((f_stat.st_mode & S_IFMT) == S_IFDIR)
        throw std::runtime_error("can't not specify a directory");
    filesize = f_stat.st_size; 
}

void sender::read_file()
{
    std::ifstream infile(_filename, std::ios_base::binary);
    if(!infile)
        throw std::runtime_error("can't not open file " + std::string(_filename));

    // initialize data members
    get_filesize(_filename);
    blocks_n = filesize / PACKET_SIZE + 1;
    stream.resize(blocks_n);
    std::iota(stream.begin(), stream.end(), 0);
    blocks = new char*[blocks_n];

    long readBytes = 0;
    std::cout << "reading " << blocks_n << " blocks into buffer..." << std::endl;
    for(int i = 0; i < blocks_n; ++i)
    {
        blocks[i] = new char[PACKET_SIZE];
        if(i == blocks_n - 1)
        {
            // avoid garbage value in the buffer
            memset(blocks[i], 0, sizeof(blocks[i]));
            infile.read(blocks[i], filesize % PACKET_SIZE);
            readBytes += infile.gcount();
            break;
        }
        infile.read(blocks[i], PACKET_SIZE);
        readBytes += infile.gcount();
    }

    if(readBytes == filesize)
        std::cout << "contents in file are read successfully!" << std::endl;
    else
    {
        std::cerr << "only " << readBytes << " bytes could be read" << std::endl;
        throw std::runtime_error("contents of file missed while being read");
    }

    infile.close();
}

inline
std::vector<int> sender::get_indices(int symbolID, int degree_quantity) const
{
    return reservoir_sampling(symbolID, degree_quantity);
}

std::vector<int> sender::reservoir_sampling(int symbolID, int sample_nums) const
{
    int size = stream.size();
    if(sample_nums >= size)
        return stream;
    std::vector<int> reservoir(stream.begin(), stream.begin() + sample_nums);
    std::default_random_engine e(symbolID);
    for(int i = sample_nums; i < size; ++i)
    {
        int j = e() % (i + 1);
        if(j < sample_nums)
            reservoir[j] = stream[i];
    }
    return reservoir;
}

void sender::XOR_twoBlocks(char *d1, char *d2) const 
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

std::vector<symbol> sender::encode(double multiple = 1.5)
{
    int symbols_n = static_cast<int>(multiple * blocks_n);
    if(symbols_n < blocks_n)
        throw std::logic_error("quantity of symbols should be at least blocks_n");
    std::cout << "a quantity " << symbols_n << " of symbols will be created."<< std::endl;

    std::vector<int> random_degrees = get_degrees(symbols_n);
    std::vector<symbol> symbols;

    std::cout << "encode begins..." << std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    for(int i = 0; i < symbols_n; ++i)
    {
        int degree_quantity = random_degrees[i];
        std::vector<int> neighbours = get_indices(i, degree_quantity);
        struct symbol newsym(i, degree_quantity, filesize);
        memset(newsym.data, 0, sizeof(newsym.data));
        int j = 0;
        for(int blockIndex : neighbours)
        {
            newsym.neighbours[j++] = blockIndex;
            XOR_twoBlocks(newsym.data, blocks[blockIndex]);
        }
        symbols.push_back(newsym);
    }

    std::cout << "encode succeed!" << std::endl; 
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "encode time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000 << "[ms]" << std::endl;
    return symbols;
}

#endif
