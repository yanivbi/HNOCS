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

#ifndef __HNOCS_OPCALC_H_
#define __HNOCS_OPCALC_H_

#include <omnetpp.h>
using namespace omnetpp;

#include "NoCs_m.h"
#include "routers/hier/FlitMsgCtrl.h"

//
// The Out Port Calc class implements the local routing decision.
// Given a packet message it decides what router output port the message
// should be forwarded to
//
// Ports:
//   inout calc - through which the packets are received and returned
//
// Events:
//   NoCPacketMsg - the head FLIT to be processed and the lastOutPort to be set
//   then the same FLIT is returned on the clac port
//
// This implementation provides XY - Routing:
// ===========================================
// This calculator is performing row first then column (XY) routing so
// given the destination id and it's current id it first needs to know how to extract
// row and column
//
// NOTE: This module assumes the mesh is built out of routers and cores.
// it also requires that routers share the same id as the core they connect to
// It does not require each router to have a core.
// It can handle disconnected ports like on the edges of the network.
//
class XYOPCalc : public cSimpleModule
{
private:
	// parameters
	int numCols; // the total number of columns in the simulations
	int rx, ry;  // the local router x and y coordinates
	int northPort, westPort, southPort, eastPort; // port indexes on the router to be used
	int corePort; // port index where the core module connects
	const char *portType; // the name of the actual module used for Port_Ifc
	const char *coreType; // the name of the actual module used for Core_Ifc

	// methods:

	// convert core and router id's into row and col (X and Y)
	int rowColByID(int id, int &x, int &y);
	// return true if the module is a "Port"
	bool isPortModule(cModule *mod);
	// Get the pointer to the remote Port module on the given port module
	cModule *getPortRemotePort(cModule *port);
	// return true if the module is a "Core"
	bool isCoreModule(cModule *mod);
	// Get the pointer to the remote Core module on the given port module
	cModule *getPortRemoteCore(cModule *port);
	// obtain the index of the current port out_sw port vector connecting to the port
	int getIdxOfSwPortConnectedToPort(cModule *port);
	// analyze Mesh topology and fill in the port numbers to be used for routing
	int analyzeMeshTopology();
	// handle the message
	void handlePacketMsg(NoCFlitMsg* msg);
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

#endif
