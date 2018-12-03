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
// The Arbiter count the number of outstanding requests
// It arbitrates on a clock timed by the clk message of type pop
//
// When a grant is sent to the inPort as a result of arbitration selecting this
// port/vc the ReqsoVC Reqs counter is decreased. If a NAK is received the counter is
// increased again.
//
// Arbitration:
// Configured by arbitration_type parameter:
// 		0 - winner takes all
//      1 - round robin
//
// Single Req queue for all request, get service by FIFO

// Asynchronous Router:
// Arbitrates when:
//   1) Gets FlitMsg.
//   2) Gets ReqMsg.
//   3) Gets AckMsg.
//   4) Gets CreditMsg.
//   5) Finished to transmit.

#include "SchedAsync.h"

Define_Module(SchedAsync)
;

void SchedAsync::initialize() {
	numInPorts = gateSize("in");
	numVCs = par("numVCs");
	flitSize_B = par("flitSize");
	arbitration_type = par("arbitration_type");

	credits.resize(numVCs, 0);
	WATCH_VECTOR(credits);

	// link utilization statistics
	linkUtilization.setName("link-utilization");
	statStartTime = par("statStartTime");
	busyTime = 0;

	// arbitration state
	WATCH_VECTOR(vcCurInPort);

	curVC = numVCs - 1;
	isDisconnected = (gate("out$o", 0)->getPathEndGate()->getType()
			!= cGate::INPUT);

	numReqs = 0;

	// Used VCs
	numUsedVCs.setName("number-used-VCs"); // total number of used VCs

	vcCurInPort.resize(numVCs, -1);
	vcCurInVC.resize(numVCs, -1);
	vcCurReq.resize(numVCs, NULL);
	vcCurNack.resize(numVCs, false);
	isBusy = false;

	WATCH_VECTOR(vcCurReq);

	// start the clock
	if (!isDisconnected) {

		// obtain the data rate of the outgoing link
		g = gate("out$o", 0)->getNextGate()->getNextGate();
		if (!g->getChannel()) {
			throw cRuntimeError("-E- no out$o 0 gate for module %s ???",
					g->getFullPath().c_str());
		}
		chan = check_and_cast<cDatarateChannel *> (g->getChannel());
		data_rate = chan->getDatarate();

		// generate 1st clk
		popMsg = new cMessage("pop");
		popMsg->setKind(NOC_POP_MSG);
		popMsg->setSchedulingPriority(5);

		switch (arbitration_type) {
			case 0: // 0- winner takes all
			arbiter_start_indx=0;
			break;
			case 1:// 1- round robin
			arbiter_start_indx=1;
			break;
			default:
			throw cRuntimeError("-E- arbitration_type is unknown for module %s ",
					getFullPath().c_str());
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
//   So the Req stay at the head of the Reqs[ip][oVC] until all its flits pass.
//
// The above seems to imply that no grants are allowed at end of packets.
// But if the next Req on the same Reqs[ip][oVC] has the same inVC it can be granted.
// So if there is no other port that has data to send we look inside the next req
// otherwise - stop for 2 cycles. (this is done if speculativeGntOnCompltedReq)



void SchedAsync::arbitrate() {

	// loop to find something to do
	int nextInPort;
	int nextVC;
	int nextInVC;
	bool found = false;
	bool found_newreq = false;

	// start with curVC - winner takes all (0)
	// start with next VC - round robin (1)
	for (int i = arbiter_start_indx; !found && (i <= numVCs); i++) {
		int vc = (curVC + i) % numVCs;

		// if no credits or got Nack for this VC don`t arbitrate this VC ...
		if (!credits[vc] || vcCurNack[vc]) {
			//			EV<< "-I- " << getFullPath() << " arbitrate: Continue, no credits for outVC: " << vc << endl;
			continue;
		}

		// can not change port during a Req (if it is still the head of Reqs and has
		// some pending grants to make)
		int ip = vcCurInPort[vc];
		int InVC = vcCurInVC[vc];
		// if there is current Req
		if (vcCurReq[vc]) {
			if (vcCurReq[vc]->getInVC() == vcCurInVC[vc]
					&& vcCurReq[vc]->getArrivalGate()->getIndex() == ip) {
				nextVC = vc;
				nextInPort = ip;
				nextInVC = InVC;
				found = true;
				found_newreq = false;
				EV<< "-I- " << getFullPath() << " Continue previous Req from ip: " << ip <<" Invc: " << InVC << " to outVC: " << vc << endl;
			} else {
				throw cRuntimeError(
						"-E- Mismatch in req arbitration: try arbitrate Inport: %d InVC: %d , BUT should arbitrate: Inport: %d InVC: %d",
						vcCurReq[vc]->getArrivalGate()->getIndex(),
						vcCurReq[vc]->getInVC(), ip, vcCurInVC[vc]);
			}

		} else {
			// go select the first Req
			// is there a pending req?
			if (!Reqs.empty()) {
				NoCReqMsg *req_new = (NoCReqMsg*) Reqs.front();
				nextVC = vc;
				nextInPort = req_new->getArrivalGate()->getIndex();
				nextInVC = req_new->getInVC();
				found = true;
				found_newreq = true;
				EV<< "-I-" <<getFullPath() << " Found new request from Inport:" << nextInPort <<" InVC:" << nextInVC << " To outVC:" << vc << endl;
			}
		}
	} // end of for

	if (!found) {
		EV<< "-I- " << getFullPath() << " nothing to arbitrate" << endl;
		return;
	}

	NoCReqMsg *req = NULL;

	if (found_newreq) {
		if (!Reqs.empty()) {
			req = (NoCReqMsg*) Reqs.pop();
			if (vcCurReq[nextVC] != NULL || vcCurInVC[nextVC] != -1 ||vcCurInPort[nextVC] != -1) {
				throw cRuntimeError("-E- selected other req (Inport: %d, InVC: %d) while current Req (Inport: %d, InVC: %d) is not fully completed.",nextInPort,nextInVC,vcCurInPort[nextVC],vcCurInVC[nextVC]);
			}
		} else {
			throw cRuntimeError("-E- Try to pop from empty Reqs queue");
		}
	} else {
		req = vcCurReq[nextVC];
	}

	if (req==NULL) {
		throw cRuntimeError("-E- Try to update NULL Req !");
	}

	// UPDATE ARBITER STATE
	curVC = nextVC;
	// update the current req pointer (we may had a more complex condition but this is OK)
	vcCurReq[curVC] = req;
	vcCurInVC[curVC]=nextInVC;
	vcCurInPort[curVC] = nextInPort;

	EV << "-I- " << getFullPath() << " Update the current req pointer: curVC:" << curVC << " CurInport:" << vcCurInPort[curVC] << " curInVC:" << vcCurInVC[curVC] << endl;

	// It is possible the Req was fully granted and if so we have nothing to do
	// unless there is a next Req on that Reqs and it shares the same inVC
	if ( req->getNumGranted() == req->getNumFlits()) {
		EV << "-I- " << getFullPath() << " Req waiting for last Flits on port:" << nextInPort
		<< " VC:" << curVC << endl;
		return;
	}

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

	isBusy = true; // Don`t arbitrate until pop or Nack

	// after completing a Req start scanning from next VC
	// for winner takes all arbitration
	if (arbitration_type==0) {
		if (req->getNumGranted() == req->getNumFlits())
		curVC = (curVC + 1) % numVCs;
	}

	measureNumUsedVC();

}

// a flit is received on the input so send it to the output
// also update Req waiting for last flit
void SchedAsync::handleFlitMsg(NoCFlitMsg *msg) {
	int invc = msg->getVC(); // InVC
	int ip = msg->getArrivalGate()->getIndex();
	bool found = false;
	NoCReqMsg *req = NULL;
	int outvc = -1;

	EV<< "-I- " << getFullPath() << " Received PktId"<< msg->getPktId() << "from Inport:" << ip << " InVC:" << invc << endl;

	// find request, i.e.:find the proper outVC
	for (int i = 0; i <= (numVCs-1); i++) {

		if (vcCurInVC[i] == invc && vcCurInPort[i] == ip) {
			outvc = i;
			req = vcCurReq[outvc];
			if (req == NULL) {
				throw cRuntimeError(
						"-E- Mismatch on current Req Info.Received Pkt from Inport: %d inVC: %d to outVC: %d",
						ip, invc, outvc);
			}
			found = true;
			continue;
		}
	}

	if (found == false) {
		throw cRuntimeError(
				"-E- Can find Cur req for received Pkt from Inport: %d inVC: %d to outVC %d",
				ip, invc, outvc);
	}

	// this info is only available on debug...
	if (req->getPktId() != msg->getPktId()) {
		throw cRuntimeError(
				"-E- Received PktId 0x%x that does not match the head Req PktId: 0x%x",
				msg->getPktId(), req->getPktId());
	}

	if (vcCurInPort[outvc] != ip) {
		throw cRuntimeError(
				"-E- Received Pkt from Inport %d but expecting Pkt from Inport %d",
				ip, vcCurInPort[outvc]);
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

		//Reqs[ip].pop_front();
		if (vcCurReq[outvc] == req) {

			EV << "-I- " << getFullPath() << "Get EoP Delete req: Inport:" << vcCurInPort[outvc] << "InVC:" << vcCurInVC[outvc] << endl;
			vcCurReq[outvc] = NULL;
			vcCurInPort[outvc] = -1;
			vcCurInVC[outvc] = -1;

			if(vcCurNack[outvc])
			throw cRuntimeError("-E- Mismatch! on outVC: %d, Inport: %d, InVC: %d ; Get EoP but marked as Nack!",outvc,vcCurInPort[outvc],vcCurInVC[outvc]);

		} else {
			throw cRuntimeError("-E- try to delete a NULL entry in vcCurReq!");
		}
		delete req;
		numReqs--;
	} else {
		// increase the number acked
		req->setNumAcked(req->getNumAcked() + 1);
	}

	if (credits[outvc] < 0) {
		throw cRuntimeError("-E- %s Sending on VC %d has no credits packet:%d",
				getFullPath().c_str(), outvc, msg->getPktId());
	}

	msg->setVC(outvc);

	send(msg, "out$o", 0);

	simtime_t txFinishTime = g->getTransmissionChannel()->getTransmissionFinishTime();
	if (txFinishTime<simTime()) {
		throw cRuntimeError("-E- txFinishTime<simTIme() !! ");
	}

	if (simTime()> statStartTime) {
		busyTime+=txFinishTime.dbl()-simTime().dbl();
	}

	if (!popMsg->isScheduled()) {
		scheduleAt(txFinishTime, popMsg);
		EV<< "-I- " << getFullPath() << " Send flit to outVC:" << outvc << " Next arbitration at: " << txFinishTime << endl;
	}
}

// Place the Req on the Reqs
void SchedAsync::handleReqMsg(NoCReqMsg *msg) {
	if (isDisconnected) {
		throw cRuntimeError("-E- %s REQ on non Disconnected Port! Routing BUG",
				getFullPath().c_str());
	}

	int ip = msg->getArrivalGate()->getIndex();
	EV<< "-I- " << getFullPath() << " Req on InPort:" << ip << endl;

	numReqs++;
	Reqs.insert(msg);

	if (!popMsg->isScheduled() && !isBusy ) {
		arbitrate();
	}

}

	// ACK/NAK handling. Only NAK cause change in outstanding Reqs
void SchedAsync::handleAckMsg(NoCAckMsg *msg) {
	int vc = msg->getOutVC();
	int ip = msg->getArrivalGate()->getIndex();
	if (msg->getOK()) { // ack
		EV<< "-I- " << getFullPath() << " Ack on" << " InPort:" << ip << " outVC: "<< vc << endl;
		vcCurNack[vc] = false;

	} else { // nack
		EV<< "-I- " << getFullPath() << " NAK on" << " InPort:" << ip << " outVC: "<< vc << endl;
		// need to require one extra grant
		NoCReqMsg *req = vcCurReq[vc];
		if (!req) {
			throw cRuntimeError("-E- No Req on InPort:%d VC %d during flit:%s",
					ip, vc, msg->getFullName());
		}
		req->setNumGranted(req->getNumGranted()-1);

		// since we have taken early credits need to recover
		credits[vc]++;
		vcCurNack[vc]=true;
		isBusy = false; // won`t sent after all ...
	}

	delete msg;
	// check if another arbitration is needed
	if (!popMsg->isScheduled() && !isBusy) {
		arbitrate();
	}
}

void SchedAsync::handleCreditMsg(NoCCreditMsg *msg) {
	int vc = msg->getVC();
	int num = msg->getFlits();
	credits[vc] += num;
	delete msg;
	EV<< "-I- " << getFullPath() << " Received new CreditMsg arrived " << endl;

	if (!popMsg->isScheduled() && !isBusy) {
		arbitrate();
		EV<< "-I- " << getFullPath() << " new CreditMsg arrived arbitrate" << endl;
	} else {
		EV<< "-I- " << getFullPath() << " new CreditMsg arrived no arbitrate" << endl;
	}

}

void SchedAsync::handlePopMsg(cMessage *msg) {

	if (g->getTransmissionChannel()->isBusy()) {
		throw cRuntimeError("-E- Try to arbitrate when the link is Busy!");
	}
	EV<< "-I- " << getFullPath() << " PopMsg arrived arbitrate" << endl;
	isBusy = false; // sent has finished
	arbitrate();

}

void SchedAsync::handleMessage(cMessage *msg) {
	int msgType = msg->getKind();
	if (msgType == NOC_FLIT_MSG) {
		handleFlitMsg((NoCFlitMsg*) msg);
	} else if (msgType == NOC_REQ_MSG) {
		handleReqMsg((NoCReqMsg*) msg);
	} else if (msgType == NOC_ACK_MSG) {
		handleAckMsg((NoCAckMsg*) msg);
	} else if (msgType == NOC_POP_MSG) {
		handlePopMsg(msg);
	} else if (msgType == NOC_CREDIT_MSG) {
		handleCreditMsg((NoCCreditMsg *) msg);
	} else {
		throw cRuntimeError("Does not know how to handle message of type %d",
				msg->getKind());
		delete msg;
	}

}

void SchedAsync::measureNumUsedVC() {

	int usedVCs = 0;

	for (int i = 0; i <= (numVCs - 1); i++) {
		if (vcCurReq[i] != NULL) {
			usedVCs++;
		}
	}
	numUsedVCs.collect(usedVCs);
}

SchedAsync::~SchedAsync() {
	// cleanup owned Req
	if ((!isDisconnected) && (popMsg)) {
		cancelAndDelete(popMsg);
	}

	while (!Reqs.empty()) {
		NoCReqMsg *req = (NoCReqMsg *) Reqs.pop();
		cancelAndDelete(req);
	}

	for (int i = 0; i <= (numVCs - 1); i++) {
		if (vcCurReq[i] != NULL) {
			delete vcCurReq[i];
		}
	}

}

void SchedAsync::finish() {

	numUsedVCs.record();

	if (!isDisconnected && (simTime() > statStartTime)) {
		double totalTime=(simTime().dbl()-statStartTime.dbl());
		linkUtilization.collect(100*busyTime/totalTime);
		linkUtilization.record();
	}else{
		linkUtilization.collect(-1); // invalid statistics
		linkUtilization.record();
	}

}

