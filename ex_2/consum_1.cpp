// consum_1.cpp

#include "consum_1.h"

void consum_1::consumer() {
	while (true) {
		wait();

		if (fetch.read() && !f_empty.read()) {
			get.write(true);
		} else {
			get.write(false);
		}

		// data_valid is get one cycle delayed
		// and with f_empty 
		data_valid.write(get.read() && !f_empty.read());

		if (data_valid.read()) {
			consumed_data = dat2.read();
			cout << sc_time_stamp() << ": consumer " << name() << " : "
					<< consumed_data << " input" << endl;
		} else {
			consumed_data = 0;
		}

	}
}

// generate a pattern of read actions
void consum_1::fetch_trigger() {

	fetch.write(false);
	wait(50, SC_NS);
	while(true) {
		wait(400, SC_NS);
		fetch.write(true);
		wait(100, SC_NS);
		fetch.write(false);
		wait(500, SC_NS);
		fetch.write(true);
		wait(100, SC_NS);
		fetch.write(false);
		wait(400, SC_NS);
		fetch.write(true);
		wait(100, SC_NS);
		fetch.write(false);
		wait(300, SC_NS);
		fetch.write(true);
		wait(1200, SC_NS);
		fetch.write(false);
		wait(200, SC_NS);
		fetch.write(true);
		wait(100, SC_NS);
		fetch.write(false);
	}
}
