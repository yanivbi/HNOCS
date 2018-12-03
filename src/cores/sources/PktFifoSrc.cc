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
// Behavior:
// There are 3 conditions to send data over the wire
//   1. There must be a FLIT in hand (either just generated or on the Q)
//   2. There is no popMsg message pending (that is the wire is not busy)
//   3. There are enough credits to send the data
//
#include "PktFifoSrc.h"

Define_Module(PktFifoSrc)
;

void PktFifoSrc::initialize() {
	credits = 0;
	pktIdx = 0;
	flitIdx = 0;
	flitSize_B = par("flitSize");
	maxQueuedPkts = par("maxQueuedPkts");
	statStartTime = par("statStartTime");
	isSynchronous = par("isSynchronous");

	numQueuedPkts = 0;
	WATCH(numQueuedPkts);
	WATCH(curPktLen);
	srcId = par("srcId");
	curPktLen = 1; // use 1 to avoid zero delay on first packet
	curPktId = srcId << 16;
	popMsg = NULL;
	numSentPackets = 0;
	numSentPkt.setName("number-sent-packets");
	numGenPackets = 0;
	numGenPkt.setName("number-generated-packets");
	totalNumQPackets = 0;
	numQPkt.setName("number-queue-packets");
	lossProb.setName("loss-probability");

	FullQueueIndicator.setName("Full_Queue_Indicator");
	FullQueueIndicator.collect(0); // initial with zero, if there wont be other collects then it should remain zero

	queueSize.setName("source-queue-size-percent");

	// a dstId parameter of -1 turns off the source...
	dstId = par("dstId");
	if (dstId < 0) {
		EV<< "-I- " << getFullPath() << " is turned OFF" << endl;
	} else {
		char genMsgName[32];
		sprintf(genMsgName, "gen-%d", srcId);
		genMsg = new cMessage(genMsgName);

		scheduleAt(simTime(), genMsg);
		dstIdHist.setName("dstId-Hist");
		dstIdHist.setRangeAutoUpper(0);
		dstIdHist.setCellSize(1.0);
		dstIdVec.setName("dstId");

		// obtain the data rate of the outgoing link
		cGate *g = gate("out$o")->getNextGate();
		if (!g->getChannel()) {
			throw cRuntimeError("-E- no out$o 0 gate for module %s ???",
					g->getFullPath().c_str());
		}
		cDatarateChannel *chan = check_and_cast<cDatarateChannel *> (g->getChannel());
		double data_rate = chan->getDatarate();
		tClk_s = (8 * flitSize_B) / data_rate;
		EV << "-I- " << getFullPath() << " Channel rate is:" << data_rate << " Clock is:"
		<< tClk_s << endl;

		char popMsgName[32];
		sprintf(popMsgName, "pop-src-%d", srcId);
		popMsg = new NoCPopMsg(popMsgName);
		popMsg->setKind(NOC_POP_MSG);
		// start in the low phase to avoid race
		scheduleAt(tClk_s*0.5, popMsg);

		// handling messages
		curPktIdx = 0;
		curMsgLen = 0;

		isTrace=par("isTrace");
		if(isTrace) {

			FILE * traceFile;
			traceIndex=1;
			packetArrivalDelayArraySize=0;
			int tmp;

			for(int i=0; i<MAXTRACESIZE; i++) { //clear array
				packetArrivalDelayArray[i] = 0;
			}

			const char* fileName=par("fileName").stringValue();
			traceFile = fopen (fileName,"r");
			if (traceFile==NULL) // test the file open.
			{
				throw cRuntimeError("Error opening output file");
			}

			while((fscanf (traceFile, "%u\n", &tmp)!=EOF) && (packetArrivalDelayArraySize < MAXTRACESIZE))
			{
				packetArrivalDelayArray[packetArrivalDelayArraySize]=1e-9*tmp; // trace file input is in ns
				packetArrivalDelayArraySize++;
			}

		}

	}
}

		// send the FLIT out and schedule the next pop
void PktFifoSrc::sendFlitFromQ() {
	if (Q.empty() || (credits <= 0))
		return;
	if (!isSynchronous && popMsg->isScheduled())
		return;

	NoCFlitMsg* flit = (NoCFlitMsg*) Q.pop();

	// we count number of outstanding packets
	if (flit->getType() == NOC_END_FLIT) {
		numQueuedPkts--;
		numSentPackets++;
	}
	flit->setInjectTime(simTime());
	EV<< "-I- " << getFullPath() << "flit injected at time: " << flit->getInjectTime() << endl;
	send(flit, "out$o");
	credits--;

	if (!isSynchronous) {
		// sched the pop
		simtime_t txFinishTime = gate("out$o")->getTransmissionChannel()->getTransmissionFinishTime();
		// only < , allow "zero" local-port latency (when local capacity is infinite)
		if (txFinishTime < simTime()) {
			throw cRuntimeError("-E- BUG - We just sent - must be busy!");
		}
		scheduleAt(txFinishTime, popMsg);
	}
}

// generate a new packet and Q all its flits
void PktFifoSrc::handleGenMsg(cMessage *msg) {
	// if we already queued too many packets wait for a next gen ...
	numGenPackets++;

	if (numQueuedPkts < maxQueuedPkts) {
		numQueuedPkts++;
		totalNumQPackets++;

		// we change destination and packet length on MESSAGE boundary
		if (curPktIdx == curMsgLen) {
			curMsgLen = par("msgLen");
			if (curMsgLen <= 0) {
				throw cRuntimeError("-E- can not handle <= 0 packets message");
			}
			curPktIdx = 0;
			dstId = par("dstId");
			curPktLen = par("pktLen");
		}
		curPktVC = par("pktVC");
		dstIdHist.collect(dstId);
		dstIdVec.record(dstId);
		pktIdx++;
		curPktId = (srcId << 16) + pktIdx;
		curPktIdx++;

		for (flitIdx = 0; flitIdx < curPktLen; flitIdx++) {
			char flitName[128];
			sprintf(flitName, "flit-s:%d-t:%d-p:%d-f:%d", srcId, dstId, pktIdx,
					flitIdx);
			NoCFlitMsg *flit = new NoCFlitMsg(flitName);
			flit->setKind(NOC_FLIT_MSG);
			flit->setByteLength(flitSize_B);
			flit->setBitLength(8 * flitSize_B);
			flit->setVC(curPktVC);
			flit->setSrcId(srcId);
			flit->setDstId(dstId);
			flit->setPktId(curPktId);
			flit->setFlitIdx(flitIdx);
			flit->setSchedulingPriority(0);
			flit->setFirstNet(true);
			flit->setFlits(curPktLen);

			if (flitIdx == 0) {
				flit->setType(NOC_START_FLIT);
			} else if (flitIdx == curPktLen - 1) {
				flit->setType(NOC_END_FLIT);
			} else {
				flit->setType(NOC_MID_FLIT);
			}
			Q.insert(flit);
		}
		if (!isSynchronous)
			sendFlitFromQ();
	} else { // not a packet overflow
		// EV<< "-I- " << getFullPath() << "Source queue is full" << endl;
		if (simTime() > statStartTime) {
			FullQueueIndicator.collect(1);
		}
	}
	queueSize.collect(1.0*numQueuedPkts / maxQueuedPkts);
	// schedule next gen
	if (isTrace) {
		scheduleAt(simTime() + packetArrivalDelayArray[traceIndex
				% (packetArrivalDelayArraySize - 1)], genMsg);
		traceIndex++;
	} else {
		double flitArrivalDelay = par("flitArrivalDelay");
		scheduleAt(simTime() + curPktLen * flitArrivalDelay, genMsg);
	}
}

void PktFifoSrc::handleCreditMsg(NoCCreditMsg *msg) {
	int vc = msg->getVC();
	int flits = msg->getFlits();
	delete msg;
	if (vc == 0) {
		credits += flits;
	}
	if (!isSynchronous)
		sendFlitFromQ();
}

void PktFifoSrc::handlePopMsg(cMessage *msg) {
	sendFlitFromQ();
	if (isSynchronous)
		scheduleAt(simTime() + tClk_s, msg);
}

void PktFifoSrc::handleMessage(cMessage *msg) {
	int msgType = msg->getKind();
	if (msgType == NOC_POP_MSG) {
		handlePopMsg((NoCPopMsg*) msg);
	} else if (msgType == NOC_CREDIT_MSG) {
		handleCreditMsg((NoCCreditMsg*) msg);
	} else {
		handleGenMsg(msg);
	}
}

void PktFifoSrc::finish() {
	dstIdHist.record();
	FullQueueIndicator.record();
	numSentPkt.collect(numSentPackets);
	numSentPkt.record();
	numGenPkt.collect(numGenPackets);
	numGenPkt.record();
	numQPkt.collect(totalNumQPackets);
	numQPkt.record();
	queueSize.record();

	if (numGenPackets != 0) {
		lossProb.collect(1 - (totalNumQPackets / numGenPackets));
	} else {
		lossProb.collect(-1); // source is turned off ...
	}
	lossProb.record();
}

PktFifoSrc::~PktFifoSrc() {
	int dstId = par("dstId");

	if (popMsg) {
		cancelAndDelete(popMsg);
	}

	if (dstId >= 0 && (genMsg)) {
		cancelAndDelete(genMsg);
	}

	while (!Q.empty()) {
		NoCFlitMsg* flit = (NoCFlitMsg*) Q.pop();
		delete flit;
	}
}
