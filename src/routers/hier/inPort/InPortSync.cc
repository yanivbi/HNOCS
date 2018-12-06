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

#include "InPortSync.h"

// Behavior:
//
// NOTE: on each VC there is only 1 packet being received at a given time
// Also there is one packet being arbitrated on each out port
//
// On new packet arrival a sequence of sending (and then receiving same msg)
// to calcOp is performed.
//
// On empty Q[inVC] or pop of the EoP from Q[inVC] we need to call calcVC (on the SoP).
// This is done by removing the head of Q[inVC] sending it to calcVC and putting it back in
//
// Whenever a FLIT/PKT is sent on out a credit is sent on the in$o.
//
// There is no delay modeling for the internal crossbar. It is assumed that if
// a grant is provided it happens at least FLIT time after previous one
//
Define_Module(InPortSync);

void InPortSync::initialize() {
	numVCs = par("numVCs");
	flitsPerVC = par("flitsPerVC");
	collectPerHopWait = par("collectPerHopWait");
	int rows = par("rows");
	int columns = par("columns");
	statStartTime = par("statStartTime");

	QByiVC.resize(numVCs);
	curOutPort.resize(numVCs);
	curOutVC.resize(numVCs);
	curPktId.resize(numVCs, 0);

	// send the credits to the other size
	for (int vc = 0; vc < numVCs; vc++)
		sendCredit(vc, flitsPerVC);

	QLenVec.setName("Inport_total_Queue_Length");

	if (collectPerHopWait) {
		qTimeBySrcDst_head_flit.resize(rows * columns);
		qTimeBySrcDst_body_flits.resize(rows * columns);
		for (int src = 0; src < rows * columns; src++) {
			qTimeBySrcDst_head_flit[src].resize(rows * columns);
			qTimeBySrcDst_body_flits[src].resize(rows * columns);
			for (int dst = 0; dst < rows * columns; dst++) {
				char str[64];
				char str1[64];
				sprintf(str, "%d_to_%d VC acquisition time", src, dst);
				sprintf(str1, "%d_to_%d transmission time", src, dst);
				qTimeBySrcDst_head_flit[src][dst].setName(str);
				qTimeBySrcDst_body_flits[src][dst].setName(str1);
			}
		}

	}
}

// obtain the attached info
inPortFlitInfo* InPortSync::getFlitInfo(NoCFlitMsg *msg) {
	cObject *obj = msg->getControlInfo();
	if (obj == NULL) {
		throw cRuntimeError("-E- %s BUG - No Control Info for FLIT: %s",
				getFullPath().c_str(), msg->getFullName());
	}

	inPortFlitInfo *info = dynamic_cast<inPortFlitInfo*> (obj);
	return info;
}

// send back a credit on the in port
void InPortSync::sendCredit(int vc, int numFlits) {
	if (gate("in$o")->getPathEndGate()->getType() != cGate::INPUT) {
		return;
	}
	EV<< "-I- " << getFullPath() << " sending " << numFlits
	<< " credits on VC=" << vc << endl;

	char credName[64];
	sprintf(credName, "cred-%d-%d", vc, numFlits);
	NoCCreditMsg *crd = new NoCCreditMsg(credName);
	crd->setKind(NOC_CREDIT_MSG);
	crd->setVC(vc);
	crd->setFlits(numFlits);
	crd->setSchedulingPriority(0);
	send(crd, "in$o");
}

	// create and send a Req to schedule the given FLIT, assume it is SoP
void InPortSync::sendReq(NoCFlitMsg *msg) {
	inPortFlitInfo *info = getFlitInfo(msg);
	int outPort = info->outPort;
	int inVC = info->inVC;
	int outVC = msg->getVC();

	if (msg->getType() != NOC_START_FLIT) {
		throw cRuntimeError("SendReq for flit which isn`t SoP");
	}

	EV<< "-I- " << getFullPath() << " sending Req through outPort:" << outPort
	<< " on VC: " << outVC << endl;

	char reqName[64];
	sprintf(reqName, "req-s:%d-d:%d-p:%d-f:%d", (msg->getPktId() >> 16), msg->getDstId(),
			(msg->getPktId() % (1<< 16)), msg->getFlitIdx());
	NoCReqMsg *req = new NoCReqMsg(reqName);
	req->setKind(NOC_REQ_MSG);
	req->setOutPortNum(outPort);
	req->setOutVC(outVC);
	req->setInVC(inVC);
	req->setPktId(msg->getPktId());
	req->setNumFlits(msg->getFlits());
	req->setNumGranted(0);
	req->setNumAcked(0);
	req->setSchedulingPriority(0);
	send(req, "ctrl$o", outPort);
}

	// when we get here it is assumed there is NO messages on the out port
void InPortSync::sendFlit(NoCFlitMsg *msg) {
	int inVC = getFlitInfo(msg)->inVC;
	int outPort = getFlitInfo(msg)->outPort;

	if (gate("out", outPort)->getTransmissionChannel()->isBusy()) {
		EV << "-E-" << getFullPath() << " out port of InPort is busy! will be available in " << (gate("out", outPort)->getTransmissionChannel()->getTransmissionFinishTime()-simTime()) << endl;
		throw cRuntimeError(
				"-E- Out port of InPort is busy!");
	}

	EV << "-I- " << getFullPath() << " sending Flit from inVC: " << inVC
	   << " through outPort:" << outPort << " on VC: " << msg->getVC() << endl;

	// delete the info object
	inPortFlitInfo *info = (inPortFlitInfo*) msg->removeControlInfo();
	delete info;

	// collect
	if (simTime()> statStartTime) {
		if (collectPerHopWait) {
			if (msg->getType() == NOC_START_FLIT) {
				qTimeBySrcDst_head_flit[msg->getSrcId()][msg->getDstId()].collect(1e9*(simTime().dbl() - msg->getArrivalTime().dbl()));
			} else {
				qTimeBySrcDst_body_flits[msg->getSrcId()][msg->getDstId()].collect(1e9*(simTime().dbl() - msg->getArrivalTime().dbl()));
			}
		}
	}
	// send to Sched
	send(msg, "out", outPort);

	// send the credit back on the inVC of that FLIT
	sendCredit(inVC,1);
}

// Handle the Packet when it is back from the VC calc
// store the outVC in curOutVC[inVC] for next pops and Send the req
void InPortSync::handleCalcVCResp(NoCFlitMsg *msg) {
	// store the calc out VC in the current received packet on the inVC
	inPortFlitInfo *info = getFlitInfo(msg);
	int inVC = info->inVC;
	int outVC = msg->getVC();

	curOutVC[inVC] = outVC;

	// we queue the flits on their inVC
	if (QByiVC[inVC].isEmpty()) {
		QByiVC[inVC].insert(msg);
	} else {
		QByiVC[inVC].insertBefore(QByiVC[inVC].front(), msg);
	}

	// Total queue size
	measureQlength();

	EV << "-I- " << getFullPath() << " Packet:" << (msg->getPktId() >> 16)
	   << "." << (msg->getPktId() % (1<< 16))
	   << " will be sent on VC:" << outVC << endl;

	sendReq(msg);
}

// Handle the packet when it is back from the Out Port calc
// Keep track of current out port per inVC
// if the Q is empty send to calc out VC or else Q it
void InPortSync::handleCalcOPResp(NoCFlitMsg *msg) {
	int inVC = getFlitInfo(msg)->inVC;

	curOutPort[inVC] = getFlitInfo(msg)->outPort;
	EV << "-I- " << getFullPath() << " Packet:" << (msg->getPktId() >> 16)
	   << "." << (msg->getPktId() % (1<< 16))
	   << " will be sent to port:" << curOutPort[inVC] << endl;

	// buffering is by inVC
	if (QByiVC[inVC].getLength() >= flitsPerVC) {
		throw cRuntimeError("-E- VC %d is already full receiving packet:%d",
				inVC, msg->getPktId());
	}

	// send it to get the out VC
	if (QByiVC[inVC].isEmpty()) {
		send(msg,"calcVc$o");
	} else {
		// we queue the flits on their inVC
		QByiVC[inVC].insert(msg);

		// Total queue size
		measureQlength();
	}
}

// handle received FLIT
void InPortSync::handleInFlitMsg(NoCFlitMsg *msg) {
	// allocate control info
	inPortFlitInfo *info = new inPortFlitInfo;
	msg->setControlInfo(info);
	int inVC = msg->getVC();
	info->inVC = inVC;

	// record the first time the flit is transmitted by sched, in order to mask source-router latency effects
	if (msg->getFirstNet()) {
		msg->setFirstNetTime(simTime());
		msg->setFirstNet(false);
	}


	if (msg->getType() == NOC_START_FLIT) {

		// make sure current packet is 0
		if (curPktId[inVC]) {
			throw cRuntimeError("-E- got new packet 0x%x during packet 0x%x",
					curPktId[inVC], msg->getPktId());
		}
		curPktId[inVC] = msg->getPktId();

		// for first flit we need to calc outVC and outPort
		EV << "-I- " << getFullPath() << " Received Packet:"
		   << (msg->getPktId() >> 16) << "." << (msg->getPktId() % (1<< 16))
		   << endl;

		// send it to get the out port calc
		send(msg, "calcOp$o");
	} else {
		// make sure the packet id is correct
		if (msg->getPktId() != curPktId[inVC]) {
			throw cRuntimeError("-E- got FLIT %d with packet 0x%x during packet 0x%x",
					msg->getFlitIdx(), msg->getPktId(), curPktId[inVC]);
		}

		// on last FLIT need to zero out the current packet Id
		if (msg->getType() == NOC_END_FLIT)
			curPktId[inVC] = 0;

		// since we do not allow interleaving of packets on same inVC we can use last head
		// of packet info (stored by inVC)
		int outPort = curOutPort[inVC];
		info->outPort = outPort;

		// queue
		EV << "-I- " << getFullPath() << " FLIT:" << (msg->getPktId() >> 16)
		   << "." << (msg->getPktId() % (1<< 16))
	       << "." << msg->getFlitIdx() << " Queued to be sent on OP:"
		   << outPort << endl;

		// buffering is by inVC
		if (QByiVC[inVC].getLength() >= flitsPerVC) {
			throw cRuntimeError("-E- VC %d is already full receiving packet:%d",
					inVC, msg->getPktId());
		}

		// we always queue continue flits on their out Port and VC -
		// May cause BW issue if the arrival is a little "slower" then Sched GNT
		// since it realy depends on the order of events in same tick!
		QByiVC[inVC].insert(msg);

		// Total queue size
		measureQlength();
	}
}

		// A Gnt starts the sending on a FLIT on an output port
void InPortSync::handleGntMsg(NoCGntMsg *msg) {
	int outVC = msg->getOutVC();
	int inVC = msg->getInVC();
	int op = msg->getArrivalGate()->getIndex();

	EV << "-I- " << getFullPath() << " Gnt of inVC: " << inVC << " outVC:" << outVC
	   << " through gate:" << msg->getArrivalGate()->getFullPath() <<" SimTime:" <<simTime()<< endl;

	NoCFlitMsg* foundFlit = NULL;
	if (!QByiVC[inVC].isEmpty()) {
		foundFlit = (NoCFlitMsg*)QByiVC[inVC].pop();
		foundFlit->setVC(curOutVC[inVC]);

		// Total queue size
		measureQlength();

		// If NOC_END_FLIT, then check if there is another packet, if yes send to calcVC
		if (foundFlit->getType() == NOC_END_FLIT && !QByiVC[inVC].isEmpty()) {
			NoCFlitMsg* nextPkt = (NoCFlitMsg*)QByiVC[inVC].pop();
			// need to get oVC and the response will send the req
			send(nextPkt,"calcVc$o");
		}

		sendFlit(foundFlit);

	} else {
		EV << "-I- Could not find any flit with inVC:" << inVC << endl;
		// send an NAK
		char nakName[64];
		sprintf(nakName, "nak-op:%d-ivc:%d-ovc:%d", op, inVC, outVC);
		NoCAckMsg *ack = new NoCAckMsg(nakName);
		ack->setKind(NOC_ACK_MSG);
		ack->setOutPortNum(op);
		ack->setInVC(inVC);
		ack->setOutVC(outVC);
		ack->setOK(false);
		send(ack, "ctrl$o", op);
	}
	delete msg;
}

void InPortSync::handleMessage(cMessage *msg) {
	int msgType = msg->getKind();
	cGate *inGate = msg->getArrivalGate();
	if (msgType == NOC_FLIT_MSG) {
		if (inGate == gate("calcVc$i")) {
			handleCalcVCResp((NoCFlitMsg*) msg);
		} else if (inGate == gate("calcOp$i")) {
			handleCalcOPResp((NoCFlitMsg*) msg);
		} else {
			handleInFlitMsg((NoCFlitMsg*) msg);
		}
	} else if (msgType == NOC_GNT_MSG) {
		handleGntMsg((NoCGntMsg*) msg);
	} else {
		throw cRuntimeError("Does not know how to handle message of type %d",
				msg->getKind());
		delete msg;
	}
}

InPortSync::~InPortSync() {
	// clean up messages at QByiVC
	numVCs = par("numVCs");
	NoCFlitMsg* msg = NULL;
	for (int vc = 0; vc < numVCs; vc++) {
		while (!QByiVC[vc].isEmpty()) {
			msg = (NoCFlitMsg*) QByiVC[vc].pop();
			cancelAndDelete(msg); //cancelAndDelete?!
		}
	}
}

void InPortSync::measureQlength() {
	// measure Total queue length
	if (simTime() > statStartTime) {
		int numVCs = par("numVCs");
		int Qsize = 0;
		for (int vc = 0; vc < numVCs; vc++) {
			Qsize = Qsize + QByiVC[vc].getLength();
		}
		QLenVec.record(Qsize);
	}
}

void InPortSync::finish() {
	if (simTime() > statStartTime) {
		int Dst;
		int Src;
		int rows = par("rows");
		int columns = par("columns");
		if (collectPerHopWait) {
			for (Dst = 0; Dst < (rows * columns); Dst++) {
				for (Src = 0; Src < (rows * columns); Src++) {
					qTimeBySrcDst_head_flit[Src][Dst].record();
					qTimeBySrcDst_body_flits[Src][Dst].record();
				}
			}
		}
	}
}

