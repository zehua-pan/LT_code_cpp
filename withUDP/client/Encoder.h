#ifndef ENCODER_H
#define ENCODER_H

#include <sys/stat.h>
#include <fstream>
#include <unordered_set>
#include <string.h>
#include <random>
#include <vector>
#include <chrono>
#include "DegreeGenerator.h"
#include "globalParameters.h"
#include "Sender.h"


class Encoder 
{
    friend struct test;
    public:
        // constructor
        Encoder(const string dsp, const string f) :
            dataSrcPath(dsp), filename(f), inFileFullName(dsp + f), blocks(nullptr), sentSymN(0) {}
        // copy control: rule of three??
        ~Encoder();
        // member functions
        void read_file();
        void encode();
        long get_blocks_quantity() const {return blocks_n;}

    private:
        // member functions
        std::vector<int> get_indices(int , int) const;
        std::vector<int> reservoir_sampling(int, int) const;
        void XOR_twoBlocks(char *d1, char *d2) const;
        void set_filesize();

        // data member
        const string dataSrcPath;
        const string filename;
        const string inFileFullName;
        char **blocks;
        long filesize;
        long blocks_n;
        long sentSymN; // statistics as well as symbolID for generating degrees
        std::vector<int> stream; // name stream is for general sample algorithm
};

Encoder::~Encoder()
{
    if(blocks)
    {
        for(int i = 0; i < blocks_n; ++i)
            delete[] blocks[i];
        delete[] blocks;
    }
}

void Encoder::set_filesize()
{
    struct stat f_stat;
    if(stat(inFileFullName.c_str(), &f_stat) == -1)
        throw std::runtime_error("can't not access " + inFileFullName);
    if((f_stat.st_mode & S_IFMT) == S_IFDIR)
        throw std::runtime_error("can't not specify a directory");
    filesize = f_stat.st_size; 
}

void Encoder::read_file()
{
    std::ifstream infile(inFileFullName.c_str(), std::ios_base::binary);
    if(!infile)
        throw std::runtime_error("can't not open file " + inFileFullName);

    // initialize data members
    set_filesize();
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
std::vector<int> Encoder::get_indices(int symbolID, int degree) const
{
    return reservoir_sampling(symbolID, degree);
}

std::vector<int> Encoder::reservoir_sampling(int symbolID, int sample_nums) const
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

void Encoder::XOR_twoBlocks(char *d1, char *d2) const 
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

void setNewSymbol(symbol& newsym)
{
    static DegreeGenerator degreeGenerator(DISTRIBUTION_RANGE);
    std::vector<int> neighbours = get_indices(symbolID, degree);
    degree = degreeGenerator.getDegree();
    newsym.symbolID = sentSymN; 
    newsym.degree = sentSymN % blocks_n == 0 ? 1 : degree; // entry for decoding
    newsym.filesize = filesize; 
    newsym.filename = filename;
    memset(newsym.data, 0, sizeof(newsym.data));
    int j = 0;
    for(int blockIndex : neighbours)
    {
        newsym.neighbours[j++] = blockIndex;
        XOR_twoBlocks(newsym.data, blocks[blockIndex]);
    }
}

void sendSyms(symbol& newsym, int num)
{
    static Sender sender;
    for(int i = 0; i < num; ++i)
    {
        setNewSymbol(newsym);
        sender.send(newsym);
        ++sentSymN;
    }
}

void Encoder::encode()
{
    long sendNumThreshold = static_cast<long>(EXCESS_COEFFIENT * blocks_n);
    long checkInterval = (long)((double)sendNumThreshold * 0.1) + 1;
    struct symbol newsym; // empty symbol for constructing

    std::cout << "encode begins..." << std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    sendSyms(newsym, sendNumThreshold);
    while(sender.checkEndMsg())
    {
        sendSyms(newsym, checkInterval);
    }
    std::cout << "receive end message from server, end this iteration" << std::endl;

    std::cout << "encode succeed!" << std::endl; 
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "encode time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000 << "[ms]" << std::endl;
}

#endif
