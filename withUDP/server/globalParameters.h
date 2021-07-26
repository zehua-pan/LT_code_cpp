#ifndef GLOBALPARAMETERS_H
#define GLOBALPARAMETERS_H

#include <time.h>

const int FAILURE = -1;
const int SUCCESS = 1;
const int PACKET_SIZE = 32768;
const int DISTRIBUTION_RANGE = 25;
const int PORT = 8080;
const std::string terminalMsg("end");
const double THRESHOLD_COEFFICIENT = 1.7;

struct timeval tvY = {0, 100 * 1000};
struct timeval tvN = {0, 0};

struct symbol
{
    // constructor
    symbol() = default;
    symbol(int i, int d, long f): symbolID(i), degree(d), filesize(f){}
    // copy control
    symbol(const symbol& s) : symbolID(s.symbolID), degree(s.degree), filesize(s.filesize){copyNewData(s);}
    symbol& operator=(const symbol& s);
    // member functions
    void copyNewData(const symbol& s);
    // data member
    int symbolID;
    int degree;
    long filesize;
    int neighbours[DISTRIBUTION_RANGE];
    char data[PACKET_SIZE];
};

void symbol::copyNewData(const symbol& s)
{
    for(int i = 0; i < DISTRIBUTION_RANGE; ++i)
        *(neighbours + i) = *(s.neighbours + i);
    char* thisData = data;
    const char* newData = s.data;
    long* longThisData = reinterpret_cast<long*>(thisData);
    const long* longNewData = reinterpret_cast<const long*>(newData);
    for(int i = 0; i < PACKET_SIZE / 8; i += 4)
    {
        *(longThisData + i) = *(longNewData + i);
        *(longThisData + i + 1) = *(longNewData + i + 1);
        *(longThisData + i + 2) = *(longNewData + i + 2);
        *(longThisData + i + 3) = *(longNewData + i + 3);
    }
}

symbol& symbol::operator=(const symbol& s)
{
    symbolID = s.symbolID;
    degree = s.degree;
    filesize = s.filesize;
    copyNewData(s);
    return *this;
}

const size_t SER_BUF_SIZE = sizeof(symbol);
const size_t OFFSET = 128;

#endif
