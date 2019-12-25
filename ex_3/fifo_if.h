// fifo_if.h
#include "systemc.h"

#ifndef FIFO_IF_H
#define FIFO_IF_H

class fifo_if : public sc_interface
{
    public:
    // virutal interface functions
    // implemented in fifo_2.cpp
    virtual bool read_fifo(unsigned char *data, unsigned int &count) = 0;
    virtual bool write_fifo(unsigned char *data, unsigned int &count) = 0;
};

#endif