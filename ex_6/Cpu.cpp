#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"                              // Reporting convenience macros

using namespace sc_core;
using namespace tlm;

///  filename for reporting
static const char *filename = "Cpu.cpp";

bool Cpu::read_new_packet_descriptor() 
{
    tlm_response_status tlm_stat;

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
        REPORT_INFO(filename, __FUNCTION__, "Packet descriptor transaction finished successfully.");
    } else {
        REPORT_ERROR(filename, __FUNCTION__, "Packet descriptor transaction failed. Restarting loop...");
        
        // since the transaction failed. restarting the function loop
        return false;
    }
    // logging the recieved packet descriptor
    REPORT_INFO(filename, __FUNCTION__, "Packet descriptor recieved: " << this->m_packet_descriptor << " with size: " << this->m_packet_descriptor.size);
    
    // sucess the new packet descriptor has been read
    return true;
}

bool Cpu::read_packet_header_from_mem() 
{
    tlm_response_status tlm_stat;

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
        REPORT_INFO(filename, __FUNCTION__, "IpPacket header transaction finished successfully.");
    } else {
        REPORT_ERROR(filename, __FUNCTION__, "IpPacket header transaction failed. Restarting loop...");
        
        // since the transaction failed. restarting the function loop
        return false;
    }
    // logging the recieved IpPacket header
    REPORT_INFO(filename, __FUNCTION__, "IpPacket header recieved at: " << this->m_packet_header.received << " with size: " << this->m_packet_header.data_size);

    // success packet header has been read successfully
    return true;
}

bool Cpu::process_packet_header(int &port_id)
{
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

        REPORT_INFO(filename, __FUNCTION__, "Packet needs to be forwarded to port: " << port_id);

        if (decrementTTL(this->m_packet_header) > 0) {
            // updating the checksum after decrementing TTL  
            updateChecksum(this->m_packet_header);

            REPORT_INFO(filename, __FUNCTION__, "Packet successfully processed. Ready to be forwarded.");

            // suceess the packet has been processed and can now be forwarded.
            return true;
        } else {
            REPORT_ERROR(filename, __FUNCTION__, "Time Excedded. TTL < 1. Packet will be discarded.");
            
            this->write_descriptor_to_discard_queue();

            // Restart the processor thread loop
            return false;
        }
    } else {
        REPORT_ERROR(filename, __FUNCTION__, "Header Integrity failed. Packet will be discarded.");
        
        this->write_descriptor_to_discard_queue();

        // Restart the processor thread loop
        return false;
    }
}

bool Cpu::write_descriptor_to_discard_queue() 
{
    tlm_response_status tlm_stat;

    REPORT_INFO(filename, __FUNCTION__, "Writing packet descriptor to discard queue: " << this->m_packet_descriptor);

    // starting transaction
    startTransaction(TLM_WRITE_COMMAND, DISCARD_QUEUE_ADDRESS, (unsigned char *) &this->m_packet_descriptor, sizeof(this->m_packet_descriptor));

    // wait for appropriate response to be recieved
    wait(this->transactionFinished_event); 

    // get response status
    tlm_stat = this->payload.get_response_status();
    
    if (tlm_stat == TLM_OK_RESPONSE) {
        REPORT_INFO(filename, __FUNCTION__, "Transaction finished successfully.");
    } else {
        REPORT_INFO(filename, __FUNCTION__, "Transaction failed.");
        
        // Failed write to discard queue
        return false;
    }
    
    REPORT_INFO(filename, __FUNCTION__, "Wrote to discard queue: " << this->m_packet_descriptor);

    // Success wrote to discard queue
    return true;
}

bool Cpu::write_packet_header_to_mem() 
{
    tlm_response_status tlm_stat;

    REPORT_INFO(filename, __FUNCTION__, "Writing packet header back to memory." );

    // starting transaction
    this->startTransaction(TLM_WRITE_COMMAND, this->m_packet_descriptor.baseAddress,
                            (unsigned char *) &this->m_packet_header, 
                            sizeof(uint64_t) + sizeof(sc_time) + IpPacket::MINIMAL_IP_HEADER_LENGTH);

    // wait for appropriate response to be recieved
    wait(this->transactionFinished_event); 

    // get response status
    tlm_stat = this->payload.get_response_status();
    
    if (tlm_stat == TLM_OK_RESPONSE) {
        REPORT_INFO(filename, __FUNCTION__, "Transaction finished successfully.");
    } else {
        REPORT_INFO(filename, __FUNCTION__, "Transaction failed.");

        // failed to write packet back to memory
        return false;
    }
    
    REPORT_INFO(filename, __FUNCTION__, "Wrote packet header back to memory.");

    // successfully wrote packet back to memory
    return true;
}

bool Cpu::write_descriptor_to_output_queue(int port_id) 
{
    tlm_response_status tlm_stat;
    soc_address_t port_arr[] = {OUTPUT_0_ADDRESS, OUTPUT_1_ADDRESS, OUTPUT_2_ADDRESS, OUTPUT_3_ADDRESS};

    REPORT_INFO(filename, __FUNCTION__, "Writing packet descriptor to output queue id: " << port_id);

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

        // failed to write the packet descrptor
        return false;
    }
    
    REPORT_INFO(filename, __FUNCTION__, "Wrote packet descriptor to output queue id: " << port_id);

    // successfully wrote to the output queue of given port_id
    return true;
}


void Cpu::processor_thread(void) {
	int port_id;

	while(true) {
		//******************************************************
		// read new packet descriptor
		//******************************************************
        if (!this->read_new_packet_descriptor())
            continue;

        //*********************************************************
		// Reading the packet header from memory
		//*********************************************************
        if (!this->read_packet_header_from_mem())
            continue;
        
        //*********************************************************
		// Processing the packet read from memory
		//*********************************************************
        if (!this->process_packet_header(port_id))
            continue;

        //*********************************************************
		// Writing the modified packet header to memory
		//*********************************************************
        if (!this->write_packet_header_to_mem())
            continue;

        //*********************************************************
		// Writing the packet descriptor into queue of the output port
		//*********************************************************
        if (!this->write_descriptor_to_output_queue(port_id))
            continue;
	}
}


//**********************************************************************
// nb_transport_bw: implementation of the backward path callback
//**********************************************************************
tlm_sync_enum Cpu::nb_transport_bw(tlm_generic_payload& transaction,
		tlm_phase& phase, sc_time& delay_time) {
	
	if (phase == BEGIN_RESP) {
		REPORT_INFO(filename, __FUNCTION__, "Callback successfull.");
	} else {
		REPORT_ERROR(filename, __FUNCTION__, "Callback failed.");
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
		REPORT_ERROR(filename, __FUNCTION__, "Transaction failed to start.");
	} else {
		REPORT_INFO(filename, __FUNCTION__, "Transaction started successfully.");
	}
}

unsigned int Cpu::instances = 0;
