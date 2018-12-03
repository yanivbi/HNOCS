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
//
// Behavior:
//
// The Arbiter count the number of outstanding requests on every [inPort][vc]
// It arbitrates on a clock timed by the clk message of type pop
//
// When a grant is sent to the inPort as a result of arbitration selecting this
// port/vc the ReqsByIPoVC Reqs counter is decreased. If a NAK is received the counter is
// increased again.
//
// Arbitration:
// Prefer to keep sending entire packet if possible
// If not cycle first through the VCs
// If no other VC has request it may switch port only if not in the middle of packet
//
// Clk'ed according to the outgoing link rate, gets clk only when it has something to arbitrate ...

#include "SchedSync.h"

Define_Module(SchedSync)
;

void SchedSync::initialize() {
	numInPorts = gateSize("in");
	numVCs = par("numVCs");
	flitSize_B = par("flitSize");
	arbitration_type = par("arbitration_type");
	givenTclk=par("givenTclk");
	heterogeneous=par("heterogeneous");
	credits.resize(numVCs, 0);
	WATCH_VECTOR(credits);
	vcUsage.resize(numVCs, 0);
	WATCH_VECTOR(vcUsage);

	// link utilization statistics
	linkUtilization.setName("link-utilization");
    statStartTime = par("statStartTime");
    numSends = 0;

	// arbitration state
	vcCurInPort.resize(numVCs, 0);
	WATCH_VECTOR(vcCurInPort);

	curVC = numVCs - 1;
	isDisconnected = (gate("out$o", 0)->getPathEndGate()->getType()
			!= cGate::INPUT);

	ReqsByIPoVC.resize(numInPorts);
	for (int i = 0; i < numInPorts; i++)
		ReqsByIPoVC[i].resize(numVCs);
	numReqs = 0;

	vcCurReq.resize(numVCs, NULL);
	WATCH_VECTOR(vcCurReq);

	// start the clock
	if (!isDisconnected) {

		// obtain the data rate of the outgoing link
		cGate *g = gate("out$o", 0)->getNextGate()->getNextGate();
		if (!g->getChannel()) {
			throw cRuntimeError("-E- no out$o 0 gate for module %s ???",
					g->getFullPath().c_str());
		}
		chan = check_and_cast<cDatarateChannel *> (g->getChannel());
		data_rate = chan->getDatarate();

		if (givenTclk){
			double given_tClks=par("tClk");
			int D = floor((data_rate*given_tClks)/(flitSize_B*8));
			tClk_s = given_tClks/D;
			EV<< "-I- " << getFullPath() << " Channel rate is:" << data_rate << " Clock is:" << tClk_s<< " (givenClk=" << given_tClks << "D="<< D << ")"<< endl;
		}else{
			tClk_s = (8 * flitSize_B) / data_rate;
			EV<< "-I- " << getFullPath() << " Channel rate is:" << data_rate << " Clock is:" << tClk_s<< " (freeClk)"<< endl;

		}



		// generate 1st clk
		popMsg = new cMessage("pop");
		popMsg->setKind(NOC_POP_MSG);
		popMsg->setSchedulingPriority(5);
		scheduleAt(simTime()+tClk_s, popMsg);
		freeRunningClk = par("freeRunningClk");

		switch (arbitration_type) {
			case 0: // 0- winner takes all
			arbiter_start_indx=0;
			break;
			case 1:// 1- round robin
			arbiter_start_indx=1;
			break;
			default:
			throw cRuntimeError("-E- arbitration_type %d is unknown ",
					arbitration_type);
		}
	}
}

// The actual arbitration function - send the GNT to the selected ip/vc
//
// The arbiter has to avoid mixing two packets on same oVC.
// * Changing inPort on same oVC is not allowed in the middle of a packet.
//   This is implemented by tracking the curReq[vc] which is set to NULL
//   once the EoP flit is passing.
// * If the curReq[vc]is not NULL no port change allowed
// * At the end of packet we can not switch to other inPort or even inVC of same inPort
//   before the flits of the packet are all sent (since they may be actually NaKed).
//   So the Req stay at the head of the ReqsByIPoVC[ip][oVC] until all its flits pass.
//
void SchedSync::arbitrate() {

	// loop to find something to do
	int nextInPort;
	int nextVC;
	bool found = false;

	if (!ev.isDisabled()) {
		EV << "-I- " << getFullPath() << " credits: ";
		for (int vc = 0; vc < numVCs; vc++)
			EV << vc << ":" << credits[vc] << " ";
		EV << endl;
		EV << "-I- " << getFullPath() << " requests: ";
		for (int ip = 0; ip < numInPorts; ip++)
			for (int vc = 0; vc < numVCs; vc++)
				EV << ip << "," << vc << ":" << ReqsByIPoVC[ip][vc].size() << " ";
		EV << endl;
	}

	// start with curVC - winner takes all (0)
	// start with next VC - round robin (1)
	for (int i = arbiter_start_indx; !found && (i <= numVCs); i++) {
		int vc = (curVC + i) % numVCs;

		// are there credits on this VC?
		if (!credits[vc])
			continue;

		// can not change port during a Req (if it is still the head of ReqsByIPoVC and has
		// some pending grants to make)
		int ip = vcCurInPort[vc];
		// if there is current Req and it is the same as the looked at ReqsByIPoVC[ip][vc] and
		// it is not completed - use it
		if (vcCurReq[vc] && ReqsByIPoVC[ip][vc].size()
				&& (ReqsByIPoVC[ip][vc].front() == vcCurReq[vc])
				&& (vcCurReq[vc]->getNumGranted() != vcCurReq[vc]->getNumFlits())) {
			nextVC = vc;
			nextInPort = ip;
			found = true;
		} else {
			// go select the first Req (that is starting with current VC and next InPort
			// to curPort of the vc
			for (int j = 1; !found && (j <= numInPorts); j++) {
				int ip = (vcCurInPort[vc] + j) % numInPorts;
				// is there a pending req?
				if (ReqsByIPoVC[ip][vc].size()) {
					nextVC = vc;
					nextInPort = ip;
					found = true;
				}
			}
		}
	}
	if (!found) {
		EV<< "-I- " << getFullPath() << " nothing to arbitrate" << endl;
		return;
	}

	NoCReqMsg *req = ReqsByIPoVC[nextInPort][nextVC].front();
	NoCReqMsg *prevReq = vcCurReq[nextVC];

	// if there is a non null CurReq for the nextVC we can not allow any other req
	if ( prevReq && (req != prevReq)) {
		EV << "-I- " << getFullPath() << " selected other port:" << nextInPort
		<< " while current Req is not fully completed. Ignore it." << endl;
		return;
	}

	// UPDATE ARBITER STATE
	curVC = nextVC;
	vcCurInPort[curVC] = nextInPort;

	// It is possible the Req was fully granted and if so we have nothing to do
	// unless there is a next Req on that ReqsByIPoVC and it shares the same inVC
	if ( req->getNumGranted() == req->getNumFlits()) {
		EV << "-I- " << getFullPath() << " Req waiting for last Flits on port:" << nextInPort
		<< " VC:" << curVC << endl;
		return;
	}

	// update the current req pointer (we may had a more complex condition but this is OK)
	vcCurReq[curVC] = req;

	// if we have got here we can generate a Gnt on the head req.
	int prevGnted = req->getNumGranted();
	req->setNumGranted(prevGnted+1);

	const char *fType = "";
	if (prevGnted == 0) {
		fType = "SoP ";
	} else if (req->getNumGranted() == req->getNumFlits()) {
		fType = "EoP ";
	}
	EV << "-I- " << getFullPath() << " arbitrating " << fType << "VC:" << curVC
	   << " InPort:" << vcCurInPort[curVC] << " Req:" << req->getFullName() << endl;

	// credit updates must happen here. Another option to put them on the flit receiver
	// would cause excessive grants that do not see the real state of the
	credits[curVC]--;

	// send the Gnt
	char gntName[128];
	int inVC = req->getInVC();
	sprintf(gntName, "gnt-ivc:%d-ocv:%d-ip:%d", inVC, curVC, vcCurInPort[curVC]);
	NoCGntMsg *gnt = new NoCGntMsg(gntName);
	gnt->setKind(NOC_GNT_MSG);
	gnt->setOutVC(curVC);
	gnt->setInVC(inVC);
	gnt->setSchedulingPriority(0);

	send(gnt, "ctrl$o", vcCurInPort[curVC]);

	// after completing a Req start scanning from next VC
	// for winner takes all arbitration
	if (arbitration_type==0) {
		if (req->getNumGranted() == req->getNumFlits())
		curVC = (curVC + 1) % numVCs;
	}
}

// a flit is received on the input so send it to the output
// also update Req waiting for last flit
void SchedSync::handleFlitMsg(NoCFlitMsg *msg) {
	int vc = msg->getVC();
	int ip = msg->getArrivalGate()->getIndex();

	// the head of the ReqsByIPoVC MUST match
	NoCReqMsg *req = ReqsByIPoVC[ip][vc].front();

	// this info is only available on debug...
	if (req->getPktId() != msg->getPktId()) {
		throw cRuntimeError(
				"-E- Received PktId 0x%x that does not match the head Req PktId: 0x%x",
				msg->getPktId(), req->getPktId());
	}

	// check if last FLIT of message
	if (msg->getType() == NOC_END_FLIT) {
		// the Req must have now zero pending grants and zero pending acks
		if (req->getNumGranted() != req->getNumFlits()) {
			throw cRuntimeError(
					"-E- Received EoP PktId 0x%x but granted:%d != flits:%d. \n flit Info: VC:%d , Index:%d , Src:%d , Dst:%d",
					msg->getPktId(), req->getNumGranted(), req->getNumFlits(),
					msg->getVC(), msg->getFlitIdx(), msg->getSrcId(),
					msg->getDstId());
		}
		if (req->getNumAcked() + 1 != req->getNumFlits()) {
			throw cRuntimeError(
					"-E- Received EoP PktId 0x%x but acked:%d + 1 != flits:%d",
					msg->getPktId(), req->getNumAcked(), req->getNumFlits());
		}

		vcUsage[vc]--;
		ReqsByIPoVC[ip][vc].pop_front();
		if (vcCurReq[vc] == req)
			vcCurReq[vc] = NULL;
		delete req;
		numReqs--;
	} else {
		// increase the number acked
		req->setNumAcked(req->getNumAcked() + 1);
	}

	if (credits[vc] < 0) {
		throw cRuntimeError("-E- %s Sending on VC %d has no credits packet:%d",
				getFullPath().c_str(), vc, msg->getPktId());
	}

	send(msg, "out$o", 0);
	if (simTime()> statStartTime) {
	    numSends++;
	}

}

// Place the Req on the ReqsByIPoVC
void SchedSync::handleReqMsg(NoCReqMsg *msg) {
	if (isDisconnected) {
		throw cRuntimeError("-E- %s REQ on non Disconnected Port! Routing BUG",
				getFullPath().c_str());
	}
	int vc = msg->getOutVC();
	int ip = msg->getArrivalGate()->getIndex();
	EV << "-I- " << getFullPath() << " Req on outVC:" << vc << " InPort:" << ip << endl;

	numReqs++;
	ReqsByIPoVC[ip][vc].push_back(msg);

	// Done: in the VC ALLOC
	// vcUsage[vc]++;
}

// ACK/NAK handling. Only NAK cause change in outstanding Reqs
void SchedSync::handleAckMsg(NoCAckMsg *msg) {
	int vc = msg->getOutVC();
	int ip = msg->getArrivalGate()->getIndex();
	if (msg->getOK()) {
		throw cRuntimeError("-E- No ACK possible on BLRouter");
	} else {
		EV<< "-I- " << getFullPath() << " NAK on VC:" << vc	<< " InPort:" << ip << endl;
		// need to require one extra grant
		NoCReqMsg *req = ReqsByIPoVC[ip][vc].front();
		if (!req) {
			throw cRuntimeError("-E- No Req on InPort:%d VC %d during flit:%s",
					ip, vc, msg->getFullName());
		}
		req->setNumGranted(req->getNumGranted()-1);

		// since we have taken early credits need to recover
		credits[vc]++;
	}
	delete msg;
}

void SchedSync::handleCreditMsg(NoCCreditMsg *msg) {
	int vc = msg->getVC();
	int num = msg->getFlits();
	credits[vc] += num;
	delete msg;

}

void SchedSync::handlePopMsg() {

	if (freeRunningClk || numReqs) {
		if (!popMsg->isScheduled()) {
			scheduleAt(simTime() + tClk_s, popMsg);
			 EV<< "-I" << getFullPath() << "popMsg is scheduled to:" <<simTime() + tClk_s << endl;
		}
		bool busy = (gate("out$o", 0)->getTransmissionChannel()->isBusy());
		if (heterogeneous){
			if (~busy)
				arbitrate();
		}else{
			arbitrate();
		}
	}
}

void SchedSync::handleMessage(cMessage *msg) {
	int msgType = msg->getKind();
	if (msgType == NOC_FLIT_MSG) {
		handleFlitMsg((NoCFlitMsg*) msg);
	} else if (msgType == NOC_REQ_MSG) {
		handleReqMsg((NoCReqMsg*) msg);
	} else if (msgType == NOC_ACK_MSG) {
		handleAckMsg((NoCAckMsg*) msg);
	} else if (msgType == NOC_POP_MSG) {
		handlePopMsg();
	} else if (msgType == NOC_CREDIT_MSG) {
		handleCreditMsg((NoCCreditMsg *) msg);
	} else {
		throw cRuntimeError("Does not know how to handle message of type %d",
				msg->getKind());
		delete msg;
	}

	// on any incoming message restart the clock...
	if (!freeRunningClk && !popMsg->isScheduled() && (numReqs > 0)) {
		double j = floor((simTime().dbl() - 1e-18) / tClk_s);
		double nextClk = (j + 1) * tClk_s;
		while (nextClk <= simTime().dbl()+1e-18) {
			nextClk += tClk_s;
		}
		EV<< "-I" << getFullPath() << " restart popMsg is scheduled to:" << nextClk << endl;
		scheduleAt(nextClk, popMsg);
	}
}

void SchedSync::finish() {
    if (!isDisconnected && (simTime() > statStartTime)) {
        int numClks=(int) round((simTime().dbl()-statStartTime.dbl())/tClk_s);
        linkUtilization.collect(100*(double) numSends/numClks);
        linkUtilization.record();
    }else{
        linkUtilization.collect(-1); // invalid statistics
        linkUtilization.record();
    }

}


SchedSync::~SchedSync() {
	// cleanup owned Req
	if ((!isDisconnected) && (popMsg)) {
		cancelAndDelete(popMsg);
	}
	for (int ip = 0; ip < numInPorts; ip++) {
		for (int vc = 0; vc < numVCs; vc++) {
			while (ReqsByIPoVC[ip][vc].size()) {
				NoCReqMsg *req = ReqsByIPoVC[ip][vc].front();
				ReqsByIPoVC[ip][vc].pop_front();
				cancelAndDelete(req);
			}
		}
	}
}
