#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"                              // Reporting convenience macros

using namespace sc_core;
using namespace tlm;

///  filename for reporting
static const char *filename = "DmaChannel.cpp";

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
			REPORT_ERROR(filename, __FUNCTION__, "Transaction finished successfully.");
		} else {
			REPORT_ERROR(filename, __FUNCTION__, "Transaction failed. Restarting loop...");
			
			// since the transaction failed. restarting the function loop
			continue;
		}
		
		// logging the recieved packet descriptor
		REPORT_INFO(filename, __FUNCTION__, "Packet descriptor recieved" << this->m_packet_descriptor );

		//*********************************************************
		// Forward the packet descriptor to an arbitrary port
		//*********************************************************

		// selecting a random port to send packet descriptor
		port_id = rand() % 4;
		
		REPORT_INFO(filename, __FUNCTION__, "Send to output port " << port_id);

		// starting transaction
		startTransaction(TLM_WRITE_COMMAND, port_arr[port_id], (unsigned char *) &this->m_packet_descriptor, sizeof(this->m_packet_descriptor));

		// wait for appropriate response to be recieved
		wait(this->transactionFinished_event); 

		// get response status
		tlm_stat = this->payload.get_response_status();
		
		if (tlm_stat == TLM_OK_RESPONSE) {
			REPORT_INFO(filename, __FUNCTION__, "Transaction finished successfully.");
		} else {
			REPORT_INFO(filename, __FUNCTION__, "Transaction failed.");
			cout << "unsuccessfull" << endl;
		}
		
		REPORT_INFO(filename, __FUNCTION__, "Wrote to output port: " << port_id << ", the value: " << this->m_packet_descriptor);
	}
}


//**********************************************************************
// nb_transport_bw: implementation of the backward path callback
//**********************************************************************
tlm_sync_enum Cpu::nb_transport_bw(tlm_generic_payload& transaction,
		tlm_phase& phase, sc_time& delay_time) {
	
	if (do_logging) {
		if (phase == BEGIN_RESP) {
			cout << endl << sc_time_stamp() 
					<< " CPU::nb_transport_bw : Callback successfull." << endl;
		} else {
			cout << endl << sc_time_stamp()
				<< " CPU::nb_transport_bw : Callback failed." << endl;
		}
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

	if (do_logging) {
		if (tlm_resp != TLM_UPDATED || phase != END_REQ) {
			cout << endl << sc_time_stamp()
				<< " CPU::processor_thread : Transaction could not start." << endl;
		} else {
			cout << endl << sc_time_stamp()
				<< " CPU::processor_thread : Transaction started successfully." << endl;
		}	
	}
}

unsigned int Cpu::instances = 0;
