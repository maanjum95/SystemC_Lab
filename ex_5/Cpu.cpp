#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"                              // Reporting convenience macros

using namespace sc_core;
using namespace tlm;

void Cpu::processor_thread(void) {
	tlm_response_status tlm_stat;
	soc_address_t port_arr[] = {OUTPUT_0_ADDRESS, OUTPUT_1_ADDRESS, OUTPUT_2_ADDRESS, OUTPUT_3_ADDRESS};
	int port_id;

	while(true) {

		//******************************************************
		// read new packet descriptor
		//******************************************************

		// check if packets are available
		while (this->packetReceived_interrupt != true)
			wait(this->packetReceived_interrupt.value_changed_event());

		// starting transaction to get packet descriptor
		this->startTransaction(TLM_READ_COMMAND, PROCESSOR_QUEUE_ADDRESS, (unsigned char *) &this->m_packet_descriptor, sizeof(this->m_packet_descriptor));

		// wait for the transaction to be finished
		wait(transactionFinished_event); 
		
		// get response status
		tlm_stat = this->payload.get_response_status();

		// check status of response. 
		// restart the loop if error is recieved.
		if (tlm_stat == TLM_OK_RESPONSE) {
			if (do_logging & LOG_CPU)
					cout << sc_time_stamp() << " " << name() << ": Packet descriptor transaction finished successfully." << endl;
		} else {
			if (do_logging & LOG_CPU)
				cout << sc_time_stamp() << " " << name() << ": Packet descriptor transaction failed. Restarting processor thread loop..." << endl;
			
			// since the transaction failed. restarting the function loop
			continue;
		}
		
		// logging the recieved packet descriptor
		if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Packet descriptor recieved: " << this->m_packet_descriptor << endl;

		//*********************************************************
		// Forward the packet descriptor to an arbitrary port
		//*********************************************************

		// selecting a random port to send packet descriptor
		port_id = rand() % 4;
		
		if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Sending packet descriptor to port: " << port_id << endl;

		// starting transaction
		startTransaction(TLM_WRITE_COMMAND, port_arr[port_id], (unsigned char *) &this->m_packet_descriptor, sizeof(this->m_packet_descriptor));

		// wait for appropriate response to be recieved
		wait(this->transactionFinished_event); 

		// get response status
		tlm_stat = this->payload.get_response_status();
		
		if (tlm_stat == TLM_OK_RESPONSE) {
			if (do_logging & LOG_CPU)
				cout << sc_time_stamp() << " " << name() << ": Output packet descriptor transaction finished successfully to port: " << port_id << endl;
		} else {
			if (do_logging & LOG_CPU)
				cout << sc_time_stamp() << " " << name() << ": Output packet descriptor transaction failed to port: " << port_id << endl;
		}
		if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Send packet descriptor " << this->m_packet_descriptor << " to output port: " << port_id << endl;
	}
}


//**********************************************************************
// nb_transport_bw: implementation of the backward path callback
//**********************************************************************
tlm_sync_enum Cpu::nb_transport_bw(tlm_generic_payload& transaction,
		tlm_phase& phase, sc_time& delay_time) {
	
	if (phase == BEGIN_RESP) {
		if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Transport backward callback successfull." << endl;
	} else {
		if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Transport backward callback failed." << endl;
	}
	
	// increment delay time
	delay_time += CLK_CYCLE_BUS;

	// respond to transaction after one CYCLE of the BUS.
	this->transactionFinished_event.notify(delay_time);

	// finish the transaction (end of 2nd phase)
	phase = END_RESP;
	return TLM_COMPLETED;
}

void Cpu::startTransaction(tlm_command command, soc_address_t address,
	unsigned char *data, unsigned int dataSize)
{
	tlm_phase phase;
	sc_time delay_time;
	tlm_sync_enum tlm_resp;

	// setting up the transaction payload
	this->payload.set_command(command);
	this->payload.set_data_ptr(data);
	this->payload.set_address(address);
	this->payload.set_data_length(dataSize);
	this->payload.set_response_status(TLM_GENERIC_ERROR_RESPONSE);

	// prepare the transaction
	delay_time = SC_ZERO_TIME;
	phase = BEGIN_REQ;

	// calling the transaction
	tlm_resp = this->initiator_socket->nb_transport_fw(payload, phase, delay_time);

	if (tlm_resp != TLM_UPDATED || phase != END_REQ) {
		if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Transaction failed to start." << endl;
	} else {
		if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Transaction started successfully." << endl;
	}

	// wait for the updated delay value
	wait(delay_time);
}

unsigned int Cpu::instances = 0;
