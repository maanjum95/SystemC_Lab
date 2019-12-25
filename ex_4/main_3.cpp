// main_3.cpp
#include "prod_3.h"
#include "fifo_3.h"
#include "consum_3.h"

int sc_main(int argc, char *argv[]) {
    
    prod_3 prod("producer");
    fifo_3 fifo("fifo");
    consum_3 consum("consumer");

    prod.prod2fifo_socket.bind(fifo.fifo2prod_socket);
    consum.consum2fifo_socket.bind(fifo.fifo2consum_socket);

    sc_time sim_dur = sc_time(3500, SC_NS);
    if (argc != 2) {
        cout << "Default simulation time = " << sim_dur << endl;
    } else {
        sim_dur = sc_time(atoi(argv[1]), SC_NS);
    }

    // start the simulation
    sc_start(sim_dur);

    return 0;
}