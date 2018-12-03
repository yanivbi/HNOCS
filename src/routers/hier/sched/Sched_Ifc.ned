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

package hnocs.routers.hier.sched;

//
// Scheduler Module Interface
//
moduleinterface Sched_Ifc
{
    parameters:
        int numVCs;
    @display("i=block/join");
    gates:
        inout ctrl[]; // connected to sw - send gnt, receive req, ack
        input in[];   // connected to sw - receiving flits
        inout out[];  // drives the links between routers
}
