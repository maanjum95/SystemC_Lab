#include "counter.h"

void counter::count() {
	// fill in here
	// since it is a thread execution will resume
	// after the wait statement and there is a need
	// for infinite loop.

	// initializing the counter
	this->cnt_int = 0;

	while (true) {
		// waiting before counter implementation
		wait();

		// reset is active low
		if (this->res.read()) 
			// counting from 0 to 99 (inclusive)
			this->cnt_int = (this->cnt_int + 1) % 100;	
		else
			this->cnt_int = 0;
		
		// writing to out signal cnt
			cnt.write(this->cnt_int);
	}
}
