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

#ifndef __HNOCS_INF_BW_MULTI_VC_SINK_H_
#define __HNOCS_INF_BW_MULTI_VC_SINK_H_

#include <omnetpp.h>
#include <NoCs_m.h>
//
// The InfiniteBWMultiVCSink is consuming FLITs
//
class InfiniteBWMultiVCSink: public cSimpleModule {
private:
	int numVCs;
	simtime_t statStartTime; // in sec
	int numRecPkt; // number of received packets, assume that onlt single source is transmitting
	// statistics
	cOutVector end2EndLatencyVec;
	cStdDev networkLatency; // network-latency for all flits
	cStdDev end2EndLatency; // source-queuing + network-latency for all flits

	cStdDev SoPEnd2EndLatency; // source queuing + network-latency (for Head flit only)
	cStdDev SoPLatency; // network-latency
	cStdDev SoPQTime; // Queuing-time the packet, collect here and not in the source to make sure that I collect statistics

	cStdDev EoPEnd2EndLatency; // source queuing + network-latency (for Head flit only)
	cStdDev EoPLatency; // network-latency
	cStdDev EoPQTime; // Queuing-time the packet, collect here and not in the source to make sure that I collect statistics

	cStdDev packetLatency; // total packet network latency, SoP (1st transmit) -> EoP (received @ sink)
	cStdDev numReceivedPkt; // number of received packets, assume that onlt single source is transmitting

	cLongHistogram SoPEnd2EndLatencyHist; // source queuing + network-latency (for Head flit only)

	std::vector<int> vcFLITs;
	std::vector<int> vcFlitIdx; // for checking receiving order of flits from each vc
	std::vector<int> curPktId; // Current PktId per vc

	std::vector<simtime_t> SoPFirstNetTime; // save the SoP First Trans time until EoP arrive

	void sendCredit(int vc, int num);
protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void finish();
};

#endif
