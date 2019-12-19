// consum_1.cpp

#include "consum_1.h"

void consum_1::consumer() {
	// fill in here
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
