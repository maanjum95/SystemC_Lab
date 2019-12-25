// consum_2.h

#include "consum_2.h"

#include <iostream>
#include <iomanip>

void consum_2::consumer() {
	unsigned int data_len;
	unsigned char consum_data[16];

	bool success;

	while (true) {
		// dynamic sensitivity to fetch_event sc_event
		wait(fetch_event);

		// generate random length b/w 1 to 16 bytes to fetch
		data_len = 1 + rand() % 16;	
		
		// call the transaction
		success = consum2fifo_port->read_fifo(consum_data, data_len);	
		cout << endl << sc_time_stamp() << " " << name() 
				<< " transaction is finished";
		
		if (success)
			cout << " successfully" << endl;
		else
			cout << " not or only in part successfully." << endl;
		
		cout << "     " << name() << " read " << data_len << " bytes."
				<< endl;

		cout << endl << sc_time_stamp() << " " << name() 
			<< " reading " << data_len << " bytes: 0x";

		// switch to hexadecimal mode
		cout << hex; 

		// writing the read data
		for (int i = 0; i < (int)data_len; i++) {
			cout << std::setw(2) << std::setfill('0') << (int)*(consum_data + i) << " ";
		}

		// switching back to decimal
		cout << dec << endl; 
	}
	
}

//*******===============================================================*******
//*******                            fetch_trigger                      *******
//*******               generates a pattern of read transtations
//*******===============================================================*******

void consum_2::fetch_trigger() {

	while(true) {
		wait(1200, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(800, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(1000, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(800, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(600, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(1200, SC_NS);
		fetch_event.notify(0, SC_NS);
	}
}
