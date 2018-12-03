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
// Realistic Router: non-zero interanal delay and finite datarate
// Works with both Synchronous & Asynchronous routers 


package hnocs.routers.hier;
// Data Channel Internal to Hierarchical Router
 channel SwLink extends ned.DatarateChannel
{
    datarate = 16Gbps;
}

// Control Channel Internal to Hierarchical Router
channel SwCtrlLink extends ned.DelayChannel
{
    delay = 2.0ns; // 500Mhz clock
}

// Hierarchical Router Structure : Ports connected by data and control channels
module Router like hnocs.routers.Router_Ifc
{
    parameters:
        string portType;
        int numPorts; // number of ports on this router
        int id; // serve as a global identifier for routing etc
        @display("i=block/broadcast");
    gates:
        inout in[];
        inout out[];
    submodules:
        port[numPorts]: <portType> like hnocs.routers.Port_Ifc {
            parameters:
                numPorts = numPorts;
                @display("p=100,100,ring,100,100");
            gates:
                sw_in[numPorts - 1];
                sw_out[numPorts - 1];
                sw_ctrl_in[numPorts - 1];
                sw_ctrl_out[numPorts - 1];
        }
    connections allowunconnected:
        for p=0..numPorts-1 {
            port[p].in <--> in[p];
            port[p].out <--> out[p];
        }
        for p=0..numPorts - 1, for o=0..numPorts - 1 {
            port[p].sw_out[o-1] <-- SwLink <-- port[o].sw_in[p] if p < o;
            port[p].sw_out[o] <-- SwLink <-- port[o].sw_in[p-1] if p > o;
            port[p].sw_ctrl_out[o-1] <--> SwCtrlLink <--> port[o].sw_ctrl_in[p] if p < o;
            port[p].sw_ctrl_out[o] <--> SwCtrlLink <--> port[o].sw_ctrl_in[p-1] if p > o;
        }
}
