//
// Copyright (C) 2010-2011 Eitan Zahavi, The Technion EE Department
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

#ifndef __HNOCS_FLU_VCCALC_H_
#define __HNOCS_FLU_VCCALC_H_

#include <omnetpp.h>
using namespace omnetpp;

#include "NoCs_m.h"
#include "routers/hier/HierRouter.h"
#include "routers/hier/FlitMsgCtrl.h"

//
// The VC Calculation Class provides the means to modify the VC of the FLIT
// provided to it.
//
// Ports:
// 	A single inout port named "calc"
//
// Events:
//  NoCPacketMsg - that its VC is to be modified and then sent back over the "calc"
//
// The basic implementation provided here is simply examining the output port
// database of used VCs (that is what VC are used by packets that are in flight)
//
class FLUVCCalc : public cSimpleModule
{
private:
	// params
	const char* schedType;

	// state
	std::vector< const std::vector<int> * > opCredits;
	std::vector< const std::vector<int> * > opVCUsage;
	int lastSrc, lastDst, lastOVC;

	// methods
	class Sched *getSchedOnPort(int op);
	void handlePacketMsg(NoCFlitMsg *msg);

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

#endif
