/****************************************************************************
*	 DRAMSim2: A Cycle Accurate DRAM simulator 
*	 
*	 Copyright (C) 2010   	Elliott Cooper-Balis
*									Paul Rosenfeld 
*									Bruce Jacob
*									University of Maryland
*
*	 This program is free software: you can redistribute it and/or modify
*	 it under the terms of the GNU General Public License as published by
*	 the Free Software Foundation, either version 3 of the License, or
*	 (at your option) any later version.
*
*	 This program is distributed in the hope that it will be useful,
*	 but WITHOUT ANY WARRANTY; without even the implied warranty of
*	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	 GNU General Public License for more details.
*
*	 You should have received a copy of the GNU General Public License
*	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/



#include "TraceBasedSim.h"

using namespace HybridSim;
using namespace std;

int complete = 0;

int main()
{
	printf("hybridsim_test main()\n");
	some_object obj;
	obj.add_one_and_run();
}


void some_object::read_complete(uint id, uint64_t address, uint64_t clock_cycle)
{
	printf("[Callback] read complete: %d 0x%lx cycle=%lu\n", id, address, clock_cycle);
	complete++;
}

void some_object::write_complete(uint id, uint64_t address, uint64_t clock_cycle)
{
	printf("[Callback] write complete: %d 0x%lx cycle=%lu\n", id, address, clock_cycle);
	complete++;
}

/* FIXME: this may be broken, currently */
void power_callback(double a, double b, double c, double d)
{
	printf("power callback: %0.3f, %0.3f, %0.3f, %0.3f\n",a,b,c,d);
}

int some_object::add_one_and_run()
{
	/* pick a DRAM part to simulate */
	HybridSystem *mem = new HybridSystem(1);
	//MemorySystem *mem = new MemorySystem(0, "ini/DDR3_micron_32M_8B_x8_sg15.ini", "ini/system.ini", "", "");


	/* create and register our callback functions */
	typedef CallbackBase<void,uint,uint64_t,uint64_t> Callback_t;
	Callback_t *read_cb = new Callback<some_object, void, uint, uint64_t, uint64_t>(this, &some_object::read_complete);
	Callback_t *write_cb = new Callback<some_object, void, uint, uint64_t, uint64_t>(this, &some_object::write_complete);
	mem->RegisterCallbacks(read_cb, write_cb, power_callback);

	srand (time(NULL));

	mem->addTransaction(false, 140708070030063);
	for (uint64_t i=0; i<10000; i++)
	{
		mem->update();
	}
	

	cout << "Preparing transactions to preload cache with data...\n";
	uint64_t num_init = 10000;
	for (uint64_t i=0; i<num_init; i++)
	{
		DRAMSim::Transaction t = DRAMSim::Transaction(DATA_READ, i*PAGE_SIZE, NULL);
		//cout << i << "calling HybridSystem::addTransaction\n";
		mem->addTransaction(t);
		if (i%10000 == 0)
			cout << i << "/" << num_init << endl;
	}
	cout << "Running transactions to preload cache with data...\n";
	int factor = 1000;
	for (uint64_t i=0; i<num_init*factor; i++)
	{
		mem->update();
		if (i%1000000 == 0)
		{
			cout << i << "/" << num_init*factor << endl;
		}
	}

	uint64_t cur_addr = 0;

	const uint64_t NUM_ACCESSES = 100;
	const int MISS_RATE = 10;

	cout << "Starting flash test...\n";
	cout << "triggering writebacks to flash..." << endl;
	for (uint64_t i=0; i<NUM_ACCESSES; i++)
	{
		TransactionType type = DATA_WRITE;

		curr_addr = curr_addr + 4096; //4096 sets so mod 4096 should fill only one set		

		DRAMSim::Transaction t = DRAMSim::Transaction(type, cur_addr, NULL);
		mem->addTransaction(t);

#if DEBUG_CACHE
		cout << "\n\tAdded transaction " << i << " of type=" << type << " addr=" << cur_addr << " set=" << SET_INDEX(cur_addr) 
			<< " tag=" << TAG(cur_addr) << endl;
#endif
		
		// TODO: not sure this update factor is correct for this test
		for (int j=0; j<factor; j++)
		{
			mem->update();
		}
	}

	curr_addr = 0;
	
	cout << "reading from flash... hopefully" << endl;
	for (uint64_t i=0; i<NUM_ACCESSES; i++)
	  {
	    Transaction type = DATA_READ;

	    curr_addr = curr_addr + 4096; //4096 sets so mod 4096 should fill only one set		

		DRAMSim::Transaction t = DRAMSim::Transaction(type, cur_addr, NULL);
		mem->addTransaction(t);

#if DEBUG_CACHE
		cout << "\n\tAdded transaction " << i << " of type=" << type << " addr=" << cur_addr << " set=" << SET_INDEX(cur_addr) 
			<< " tag=" << TAG(cur_addr) << endl;
#endif
		
		// TODO: not sure this update factor is correct for this test
		for (int j=0; j<factor; j++)
		{
			mem->update();
		}	    
	  }


	for (int i=0; i<1000000; i++)
	{
		mem->update();
	}


	cout << "\n\n" << mem->currentClockCycle << ": completed " << complete << "\n\n";
	cout << "dram_pending=" << mem->dram_pending.size() << " flash_pending=" << mem->flash_pending.size() << "\n\n";
	cout << "dram_queue=" << mem->dram_queue.size() << " flash_queue=" << mem->flash_queue.size() << "\n\n";
	cout << "pending_pages=" << mem->pending_pages.size() << "\n\n";

	return 0;
}

