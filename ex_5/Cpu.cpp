#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"                              // Reporting convenience macros

using namespace sc_core;
using namespace tlm;

void Cpu::processor_thread(void) {
	tlm_response_status tlm_stat;

	while(true) {

		//******************************************************
		// read new packet descriptor
		//******************************************************

		// check if packets are available
		if (packetReceived_interrupt != true)
			wait(packetReceived_interrupt.value_changed_event());

		// starting transaction to get packet descriptor
		startTransaction(TLM_READ_COMMAND, PROCESSOR_QUEUE_ADDRESS, (unsigned char *) &m_packet_descriptor, sizeof(m_packet_descriptor));

		// wait for appropriate response to be recieved
		wait(transactionFinished_event); 

		if (do_logging) {
			cout << endl << sc_time_stamp()
				<< "CPU::processor_thread : Transaction is finished";
			
			tlm_stat = payload.get_response_status();

			if (tlm_stat == TLM_OK_RESPONSE)
				cout << " successfully." << endl;
			else
				cout << " unsuccessfully." << endl;
			
			cout << "CPU::processor_thread : Packet descriptor recieved: " << m_packet_descriptor << endl;
		}

		//*********************************************************
		// Forward the packet descriptor to an arbitrary port
		//*********************************************************

		// selecting a random port to send packet descriptor
		soc_address_t port_arr[] = {OUTPUT_0_ADDRESS, OUTPUT_1_ADDRESS, OUTPUT_2_ADDRESS, OUTPUT_3_ADDRESS};
		int port_id = rand() % 4;
		if (do_logging) {
			cout << "CPU::processor_thread : Sending to output port: " << port_id << endl;
		}

		// starting transaction
		startTransaction(TLM_WRITE_COMMAND, port_arr[port_id], (unsigned char *) &m_packet_descriptor, sizeof(m_packet_descriptor));

		// wait for appropriate response to be recieved
		wait(transactionFinished_event); 

		if (do_logging) {
			cout << endl << sc_time_stamp()
				<< "CPU::processor_thread : Transaction is finished";
			
			tlm_stat = payload.get_response_status();
			
			if (tlm_stat == TLM_OK_RESPONSE)
				cout << " successfully." << endl;
			else
				cout << " unsuccessfully." << endl;
			
			cout << endl << "CPU::processor_thread : Wrote to output port:" << port_id << ", the value: " << m_packet_descriptor << endl;
		}	
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
	transactionFinished_event.notify(delay_time);

	// finish the transaction (end of 2nd phase)
	phase = END_RESP;
	return TLM_COMPLETED;
}

/**
* Start a 2-phase transaction with the given arguments.
*
* @param command - TLM_READ_COMMAND or TLM_WRITE_COMMAND
* @param address - the address of the destination/source of the data
* @param data    - pointer to the data that is written or pointer to a
*                  buffer where the data is going to be stored
* @param dataSize - size of the data in bytes
*/
void Cpu::startTransaction(tlm_command command, soc_address_t address,
	unsigned char *data, unsigned int dataSize)
{
	tlm_phase phase;
	sc_time delay_time;
	tlm_sync_enum tlm_resp;

	// setting up the transaction payload
	payload.set_command(command);
	payload.set_data_ptr(data);
	payload.set_address(address);
	payload.set_data_length(dataSize);
	payload.set_response_status(TLM_GENERIC_ERROR_RESPONSE);

	// prepare the transaction
	delay_time = SC_ZERO_TIME;
	phase = BEGIN_REQ;

	// calling the transaction
	tlm_resp = initiator_socket->nb_transport_fw(payload, phase, delay_time);

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
