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

package hnocs.routers.hier.sched.wormhole;

//
// Async Scheduler module
//
simple SchedAsync like hnocs.routers.hier.sched.Sched_Ifc
{
    parameters:
        int numVCs;					// number of VCs
        int flitSize @unit(byte);   // flitSize [bytes]
        int arbitration_type; 		// 0 - winner takes all arbitration ;  1 - round robin arbitration
        double statStartTime @unit(s); // start time for recording statistics [sec]
        @display("i=block/join");
    gates:
        inout ctrl[]; // connected to sw - send gnt, receive req, ack
        input in[];   // connected to sw - receiving flits
        inout out[];  // drives the links between routers
}
// For details about statistics, please refer to SchedAsync.h 