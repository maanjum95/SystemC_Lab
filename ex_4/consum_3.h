// consum_3.h
#ifndef CONSUM_3_H
#define CONSUM_3_H

#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

// to make things readable...
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

SC_MODULE(consum_3) {
public:
    // initiator socket to interconnect consumer with FIFO
    simple_initiator_socket<consum_3> consum2fifo_socket;

private:
    // event for communciation between fetch_trigger() and consumer() processes
    sc_event fetch_event;
    // event for informing about finished transaction
    sc_event response_event;

private:
    void consumer();
    void fetch_trigger();

public:
    // return state from FIFO
    tlm_sync_enum nb_transport_bw (tlm_generic_payload &payload, // ref to payload
                                    tlm_phase &phase,           // ref to phase
                                    sc_time &delay_time);       // ref to delay time

    SC_CTOR(consum_3) : consum2fifo_socket("consum2fifo_socket") {
        // register nb_transport_bw function with sockets
        consum2fifo_socket.register_nb_transport_bw(this, &consum_3::nb_transport_bw);

        // registration of processes
        SC_THREAD(consumer);
        SC_THREAD(fetch_trigger);
    }    

};

#endif