//
// Copyright (C) 2010-2011 Eitan Zahavi, The Technion EE Department
// Copyright (C) 2010-2011 Yaniv Ben-Itzhak, The Technion EE Department
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __HNOCS_SCHED_SYNC_H_
#define __HNOCS_SCHED_SYNC_H_

#include <omnetpp.h>
using namespace omnetpp;

#include "NoCs_m.h"
#include "routers/hier/HierRouter.h"

//
// Crossbar Scheduler
//
// Ports:
//  inout in - the input of data and requests from the switch crossbar
//  inout out - the NoC router output
//
// Event:
//   Req - Received on the "in" port carry the request of a InPort to send data on specific
//         VC through that OutPort - one per packet
//   FLIT/Packet - the data being provided from the InPort
//   Credit - received from the other side of the out port
//
// NOTE: for every output port and VC there is a single packet that is granted by the
// scheduler.
//
class SchedSync : public Sched
{
private:
	// parameters
	int numVCs;
    int flitSize_B;           // flitSize
    int arbitration_type;     // 0- winner takes all , 1- round robin ,
    bool freeRunningClk;      // 0 - try shutting down the clk
    bool heterogeneous; 		// arbitrating only when outport isn`t busy, when true use only with idealRouter mesh file and with the maximum frequency (of fastest link)
    simtime_t statStartTime; // in sec

	// Out link info
	cDatarateChannel *chan;
	double data_rate ;
	bool givenTclk; 		// if true uset_clk a parameter from ini file
	// state
	int numInPorts;
	int numReqs;  // total number of requests
	std::vector< std::vector< std::list<NoCReqMsg*> > > ReqsByIPoVC; // active requests by [ip][vc]
	std::vector< int > credits; // credits per VC
	std::vector< int > vcUsage; // count number of pending reqs per VC

	cMessage *popMsg; // this is the clock...
	double tClk_s;    // clock cycle time
	bool isDisconnected; // if true means there is no InPort or Core on the other side
	int numSends; // counts the number of flit sends through the egress link connected to the sched
	// arbitration-type
	int arbiter_start_indx;

	// arbitration state
	int curVC; // last VC sent
	std::vector<int> vcCurInPort;  // last port sending on this VC
	std::vector<NoCReqMsg*> vcCurReq; // the current Req (last one arbitrated on a vc)

	// Statistics
	cStdDev linkUtilization; // the egress link utiliztion connected to the sched

	// methods
	void handleFlitMsg(NoCFlitMsg *msg );
	void handleReqMsg(NoCReqMsg *msg);
	void handleAckMsg(NoCAckMsg *msg);
	void handlePopMsg();
	void handleCreditMsg(NoCCreditMsg *msg);
	void arbitrate();

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
public:
    const std::vector<int> *getCredits() const {return &credits;};
    const std::vector<int> *getVCUsage() const {return &vcUsage;};
    virtual void incrVCUsage(int vc) { vcUsage[vc]++ ; } ;
    virtual ~SchedSync();
};

#endif
