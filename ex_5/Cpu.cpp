#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"                              // Reporting convenience macros

using namespace sc_core;
using namespace tlm;

void Cpu::processor_thread(void) {

	tlm_phase phase;
	sc_time delay_time;

	while(true) {

		//******************************************************
		// read new packet descriptor
		//******************************************************

		// fill in

		//*********************************************************
		// Forward the packet descriptor to an arbitrary port
		//*********************************************************

		// fill in
	}
}


//**********************************************************************
// nb_transport_bw: implementation of the backward path callback
//**********************************************************************
tlm_sync_enum Cpu::nb_transport_bw(tlm_generic_payload& transaction,
		tlm_phase& phase, sc_time& delay_time) {
	// fill in
}

unsigned int Cpu::instances = 0;
