#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"                              // Reporting convenience macros

using namespace sc_core;
using namespace tlm;

void Cpu::processor_thread(void) {
	int port_id;
    tlm_response_status tlm_stat;
    bool discard_packet;
    
    soc_address_t port_arr[] = {OUTPUT_0_ADDRESS, OUTPUT_1_ADDRESS, OUTPUT_2_ADDRESS, OUTPUT_3_ADDRESS};

	while (true) {
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
                cout << sc_time_stamp() << " " << name() << ": Packet descriptor recieved: " 
                <<  this->m_packet_descriptor << " with packet size: " << this->m_packet_descriptor.size << endl;

        //*********************************************************
		// Reading the packet header from memory
		//*********************************************************
        while (true) {
            // getting the ipheader from memory
            // The actual packet data as stored in the memory are prepended with a time stamp of the packet
            // arrival time (type sc_time) and the packet size (type uint64_t). Therefore, these values have to
            // be read in addition to the 20 bytes of a minimum IP header. 
            // the order in IpPacket header is uint64_t data_size; sc_time received; unsigned char packet_data[PACKET_MAX_SIZE];
            this->startTransaction(TLM_READ_COMMAND, this->m_packet_descriptor.baseAddress,
                                    (unsigned char *) &this->m_packet_header, 
                                    sizeof(uint64_t) + sizeof(sc_time) + IpPacket::MINIMAL_IP_HEADER_LENGTH);

            // wait for the transaction to be finished
            wait(transactionFinished_event); 

            // get response status
            tlm_stat = this->payload.get_response_status();

            // check status of response
            // restart the loop if error is recieved.
            if (tlm_stat == TLM_OK_RESPONSE) {
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": IpPacket header transaction finished successfully." << endl;
            } else {
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": IpPacket header transaction failed. Restarting processor thread loop..." << endl;
                
                // failed to read packet header from mem
                continue;
            }
            // logging the recieved IpPacket header
            if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": IpPacket header recieved at: " 
                    << this->m_packet_header.received << " with size: " << this->m_packet_header.data_size << endl;
            
            // successfull to read packet header from mem
            break;
        }

        //*********************************************************
		// Processing the packet read from memory
		//*********************************************************

        // 1. Verify the IP header integrity. If it is incorrect, discard the packet by writing the packet
        // descriptor to the discard queue in the I/O module. Then wait for a new interrupt from
        // the processor queue. Otherwise go on with step 2.
        // 2. Identify the next-hop, i.e. the output port for the IP packet based on its destination
        // address. For this purpose, the next-hop lookup table of the NPU has to be searched. It
        // contains a list of IP address prefixes with a port number associated to each. For the
        // destination IP address the entry with the longest prefix that matches the IP destination
        // address has to be found, which in turn determines the output port of the packet.
        // Technische Universität München – LIS Manual SystemC Laboratory
        // 22
        // 3. Decrement the TTL field by 1. If TTL results in 0, then discard the packet. (This
        // mechanism prevents packets from wandering infinitely around in the network. In
        // reality an ICMP error message would have to be created in this case, which is neglected
        // in the lab.)
        // 4. Update the header checksum (as the header was modified).
        // 5. Write back the modified packet header to the memory.
        // 6. Write the packet descriptor into the queue corresponding to the output port.
        // tlm_response_status tlm_stat;
        if (verifyHeaderIntegrity(this->m_packet_header)) {
            port_id = this->makeNHLookup (this->m_packet_header);

            if (do_logging & LOG_CPU)
                cout << sc_time_stamp() << " " << name() << ": Packet needs to be forwarded to port: " << port_id << endl;

            if (decrementTTL(this->m_packet_header) > 0) {
                // updating the checksum after decrementing TTL  
                updateChecksum(this->m_packet_header);

                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Packet successfully processed. Ready to be forwarded." << endl;
                
                discard_packet = false;
            } else {
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Time Excedded. TTL < 1. Packet will be discarded." << endl;
                
                discard_packet = true;
            }
        } else {
            if (do_logging & LOG_CPU)
                cout << sc_time_stamp() << " " << name() << ": Header Integrity failed. Packet will be discarded." << endl;
            
            discard_packet = true;
        }

        // either the packet will be added to discard queue or forward it to output queue
        if (discard_packet) {
            //*********************************************************
            // Writing the packet descriptor to discard queue
            //*********************************************************      
            while (true) {
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Writing packet descriptor " << this->m_packet_descriptor << " to discard queue." << endl;

                // starting transaction
                startTransaction(TLM_WRITE_COMMAND, DISCARD_QUEUE_ADDRESS, (unsigned char *) &this->m_packet_descriptor, sizeof(this->m_packet_descriptor));

                // wait for appropriate response to be recieved
                wait(this->transactionFinished_event); 

                // get response status
                tlm_stat = this->payload.get_response_status();
                
                if (tlm_stat == TLM_OK_RESPONSE) {
                    if (do_logging & LOG_CPU)
                        cout << sc_time_stamp() << " " << name() << ": Discard queue transaction finished successfully." << endl;
                } else {
                    if (do_logging & LOG_CPU)
                        cout << sc_time_stamp() << " " << name() << ": Discard queue transaction failed." << endl;
                    
                    // Failed write to discard queue
                    continue;
                }
                
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Wrote packet descriptor " << this->m_packet_descriptor <<" to discard queue." << endl;
                
                // successfull to write to discard queue
                break;
            }

        } else {
            //*********************************************************
            // Writing the modified packet header to memory
            //*********************************************************
            while (true) {
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Writing processed packet header back to memory." << endl;

                // starting transaction
                this->startTransaction(TLM_WRITE_COMMAND, this->m_packet_descriptor.baseAddress,
                                        (unsigned char *) &this->m_packet_header, 
                                        sizeof(uint64_t) + sizeof(sc_time) + IpPacket::MINIMAL_IP_HEADER_LENGTH);

                // wait for appropriate response to be recieved
                wait(this->transactionFinished_event); 

                // get response status
                tlm_stat = this->payload.get_response_status();
                
                if (tlm_stat == TLM_OK_RESPONSE) {
                    if (do_logging & LOG_CPU)
                        cout << sc_time_stamp() << " " << name() << ": Packet header transaction finished successfully." << endl;
                } else {
                    if (do_logging & LOG_CPU)
                        cout << sc_time_stamp() << " " << name() << ": Packet header transaction failed." << endl;

                    // failed to write packet back to memory
                    continue;
                }
                
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Wrote packet header back to memory." << endl;
                
                // successful to write packet back to memory
                break;
            }
            //*********************************************************
            // Writing the packet descriptor into queue of the output port
            //*********************************************************
            while (true) {
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Writing packet descriptor to output queue: " << port_id << endl;

                // starting transaction
                startTransaction(TLM_WRITE_COMMAND, port_arr[port_id], (unsigned char *) &this->m_packet_descriptor, sizeof(this->m_packet_descriptor));

                // wait for appropriate response to be recieved
                wait(this->transactionFinished_event); 

                // get response status
                tlm_stat = this->payload.get_response_status();
                
                if (tlm_stat == TLM_OK_RESPONSE) {
                    if (do_logging & LOG_CPU)
                        cout << sc_time_stamp() << " " << name() << ": Packet descriptor transaction finished successfully." << endl;
                } else {
                    if (do_logging & LOG_CPU)
                        cout << sc_time_stamp() << " " << name() << ": Packet descriptor transaction failed." << endl;

                    // failed to write the packet descrptor to output queue
                    continue;
                }
                
                if (do_logging & LOG_CPU)
                    cout << sc_time_stamp() << " " << name() << ": Wrote packet descriptor to output queue id: " << port_id << endl;
                
                // successful to write the packet descriptor to output queue
                break;
            }
        }
    }
}


//**********************************************************************
// nb_transport_bw: implementation of the backward path callback
//**********************************************************************
tlm_sync_enum Cpu::nb_transport_bw(tlm_generic_payload& transaction,
		tlm_phase& phase, sc_time& delay_time) {
	
	if (phase == BEGIN_RESP) {
        if (do_logging & LOG_CPU)
			cout << sc_time_stamp() << " " << name() << ": Transport backward callback successfull" << endl;
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
}

unsigned int Cpu::instances = 0;
