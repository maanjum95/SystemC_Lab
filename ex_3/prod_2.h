// prod_2.h
#ifndef PROD_2_H
#define PROD_2_H

#include "systemc.h"
#include "fifo_if.h"

SC_MODULE(prod_2) {
public:
    // fifo interface
    sc_port<fifo_if> prod2fifo_port;

private:
    // event to notify producer to send 
    sc_event send_event;

    void producer();
    void send_trigger();
    
public:
    SC_CTOR(prod_2) {
        // register processes
        SC_THREAD(send_trigger);
        SC_THREAD(producer);
    }
};
#endif