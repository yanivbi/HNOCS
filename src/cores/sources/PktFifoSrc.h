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

#ifndef __HNOCS_PKT_FIFO_SOURCE_H_
#define __HNOCS_PKT_FIFO_SOURCE_H_

#include <omnetpp.h>
#include <NoCs_m.h>

#define MAXTRACESIZE 500000
//
// A simple source of Packets made out of FLITs on a single VC (0)
//
class PktFifoSrc: public cSimpleModule {
private:
	// parameters:
	int srcId;
	int dstId;
	int flitSize_B;
	simtime_t statStartTime; // in sec
	bool isSynchronous;       // if true will send packets on clock with freq of out link
	bool			isTrace; 					// If true uses a trace file for flitArrivalDelay
	char 			fileName;					// trace filename

	// for reading trace data
	double packetArrivalDelayArray[MAXTRACESIZE];
	int packetArrivalDelayArraySize;
	int traceIndex; // index for trace array

	// state:
	int pktIdx;
	int flitIdx;
	int curPktLen;
	int curPktId;
	int curPktVC;
	double numQueuedPkts;
	int maxQueuedPkts;
	int curMsgDst;			// the destination of the current msg
	int curMsgLen;			// length in packets of current msg
	int curPktIdx;          // the packet index in the msg

	int numSentPackets;// number of sent packets, assume that there is only single destination
	double numGenPackets; // number of generated packets, for loss probability
	double totalNumQPackets; // number of queued packets, for loss probability
	cQueue Q;
	NoCPopMsg *popMsg; // used to pop packets modeling the wire BW
	cMessage  *genMsg; // used to gen next flit
	int credits;       // number of credits on VC=0
	double tClk_s;     // clk extracted from output channel

	// Statistics
	cLongHistogram dstIdHist;
	cOutVector dstIdVec;
	cStdDev FullQueueIndicator; // If >0 then the queue was full during the simulation
	cStdDev queueSize; // queue fill in % tracked every generation event
	cStdDev numSentPkt; // number of sent packets, assume that there is only single destination
	cStdDev numGenPkt; // number of generated packets, for loss probability
	cStdDev numQPkt; // number of queued packets, for loss probability
	cStdDev lossProb; // probability to throw packet i.e. source queue is full and therefore the packet is discarded

	// methods
	void sendFlitFromQ();
	void handleGenMsg(cMessage *msg);
	void handleCreditMsg(NoCCreditMsg *msg);
	void handlePopMsg(cMessage *msg);

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

public:
    virtual ~PktFifoSrc();
};

#endif
