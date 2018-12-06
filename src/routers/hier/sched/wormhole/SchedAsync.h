//
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


#ifndef __HNOCS_ASYNC_SCHED_H_
#define __HNOCS_ASYNC_SCHED_H_

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
class SchedAsync: public Sched {
private:
	// parameters
	int numVCs;
	int flitSize_B; // flitSize
	int arbitration_type; // 0- winner takes all , 1- round robin ,
    simtime_t statStartTime; // in sec

	// Out link info
	cDatarateChannel *chan;
	double data_rate;

	// state
	int numInPorts;
	int numReqs; // total number of requests
	cQueue Reqs; // active requests
	std::vector<int> credits; // credits per VC
	cMessage *popMsg; // this is the clock...
	cGate *g; // outgoing link
	bool isDisconnected; // if true means there is no InPort or Core on the other side
	bool isBusy;

	// arbitration-type
	int arbiter_start_indx;

	// arbitration state
	int curVC; // last VC sent
	std::vector<int> vcCurInPort; // last port sending on this VC
	std::vector<int> vcCurInVC; // last inVC  sending on this VC
	std::vector<NoCReqMsg*> vcCurReq; // the current Req (last one arbitrated on a vc)
	std::vector<bool> vcCurNack; // indicate whether current req for outVC is Nack

	// Statistics
	cStdDev numUsedVCs; // total number of used VCs, use max to obtain the total required VC.
	std::vector< int > vcUsage; // count number of pending reqs per VC
	cStdDev linkUtilization; // the egress link utiliztion connected to the sched
	double busyTime;


	// methods
	void measureNumUsedVC();
	void handleFlitMsg(NoCFlitMsg *msg);
	void handleReqMsg(NoCReqMsg *msg);
	void handleAckMsg(NoCAckMsg *msg);
	void handlePopMsg(cMessage *msg);
	void handleCreditMsg(NoCCreditMsg *msg);
	void arbitrate();

protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void finish();

public:
    const std::vector<int> *getVCUsage() const {return &vcUsage;};
    virtual void incrVCUsage(int vc) { vcUsage[vc]++ ; } ;
	const std::vector<int> *getCredits() const {
		return &credits;
	}
	;
	virtual ~SchedAsync();
};

#endif
