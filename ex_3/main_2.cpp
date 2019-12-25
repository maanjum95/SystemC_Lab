// main_2.cpp

#include "prod_2.h"
#include "fifo_2.h"
#include "consum_2.h"

int sc_main(int argc, char *argv[]) {

    prod_2 prod("producer");
    fifo_2 fifo("fifo");
    consum_2 consum("consumer");

    // interconnecting port and interface
    prod.prod2fifo_port(fifo);
    consum.consum2fifo_port(fifo);

    sc_time sim_dur = sc_time(5000, SC_NS);
	if(argc != 2) {
		cout << "Default simulation time = " << sim_dur << endl;
	}
	else {
		sim_dur = sc_time(atoi(argv[1]), SC_NS);
	}

    // start simulation
	sc_start(sim_dur);

    return 0;
}