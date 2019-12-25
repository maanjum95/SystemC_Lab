// consum_3.cpp

#include "consum_3.h"

#include "systemc.h"
#include "tlm.h"

#include <iostream>
#include <iomanip>

// to make things easier readable ...
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

//*******===============================================================*******
//*******                             consumer                          *******
//*******===============================================================*******

void consum_3::consumer() {
	// setup generic payload with an appropriate buffer for the read data to be readd
	tlm_generic_payload trans;
	unsigned char data[16];
	trans.set_data_ptr(data);

	tlm_sync_enum tlm_resp;
	tlm_response_status tlm_stat;
	sc_time delay;
	tlm_phase phase;

	int data_len;	// number of bytes that should be read [1 - 16]

	// wait for pending transaction to complete
	bool pending_transaction = false;

	while (true) {
		if (pending_transaction) {
			cout << sc_time_stamp()
					<< " consumer waiting until transaction is finished"
					<< endl;
			wait (response_event);

			// now response via transport_bw has arrived
			// evaluate return status of transaction
			tlm_stat = trans.get_response_status();
			cout << endl << sc_time_stamp()
					<< " consumer transaction is finished" << endl;
			
			if (tlm_stat == TLM_OK_RESPONSE)
				cout << "successfully.";
			else
				cout << " not or only in part successfully" << endl;

			// output message
			cout << endl << sc_time_stamp() << " consumer reading "
						<< data_len << "bytes";
			cout << hex;
			for (int i = 0; i < data_len; i++) {
				cout << std::setw(2) << std::setfill('0') << (int)*(data + i)
						<< " ";
			}
			cout << dec << endl;

			pending_transaction = false;
		}

		// wait untill a new read request is issued
		wait(fetch_event);

		// length of data to be read, random between 1 to 16
		data_len = 1 + rand() % 16;

		trans.set_command(TLM_READ_COMMAND);
		trans.set_data_length(data_len);
		trans.set_response_status(TLM_GENERIC_ERROR_RESPONSE);

		// prepare the transaction
		delay = SC_ZERO_TIME;
		phase = BEGIN_REQ;

		// output message 
		cout << endl << sc_time_stamp() << " consumer wants to read "
				<< data_len << endl;

		// call the transaction
		tlm_resp = consum2fifo_socket->nb_transport_fw(trans, phase, delay);

		pending_transaction = true;

		if (tlm_resp != TLM_UPDATED || phase != END_REQ) {
			cout << endl << sc_time_stamp()
				<< " consumer: read request not appropriately completed" << endl;
		}
		
	}

}

// *******===============================================================******* //
// *******               implementation of nb_transport_bw               ******* //
// *******===============================================================******* //
//*******================================================*******
//  nb_transport_bw implementation of bw calls from targets
//*******================================================*******

tlm_sync_enum consum_3::nb_transport_bw(tlm_generic_payload &payload,
										tlm_phase &phase,
										sc_time &delay_time) {
	if (phase == BEGIN_RESP) {
		cout << endl << sc_time_stamp() 
					<< " consumer: read confirmation coming" << endl;
	} else {
		cout << endl << sc_time_stamp()
					<< " consumer:: read not correctly confirmed" << endl;
	}

	// increase delay time by a value corresponding to transfer time of
	// confirmation assumed to 1 clock cycle with 50ns cycle time
	delay_time += sc_time(50, SC_NS);

	// consumer process shoudl go on after time needed for the confirmation
	response_event.notify(delay_time);

	// finish the transaction (end of 2nd phase)
	phase = END_RESP;
	return TLM_COMPLETED;
}

//*******===============================================================*******
//*******                            fetch_trigger                      *******
//*******               generates a pattern of read transtations
//*******===============================================================*******

void consum_3::fetch_trigger() {

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
