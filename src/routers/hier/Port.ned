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

// A Single port of a hierarchical router. The port is built of:
// InPort - the input buffers
// OPCalc - calculates out port for the packet
// VCCalc - assigned out VC
// Sched - out port scheduler
package hnocs.routers.hier;

module Port like hnocs.routers.Port_Ifc
{
    parameters:
        string inPortType;
        string OPCalcType;
        string VCCalcType;  
        string schedType;      
        int numPorts;
        @display("i=block/arrival;bgl=2");
    gates:
        inout in;
        inout out;
        output sw_in[numPorts-1];
        input  sw_out[numPorts-1];
		inout  sw_ctrl_in[numPorts-1];
		inout  sw_ctrl_out[numPorts-1];
    submodules:
        inPort: <inPortType> like hnocs.routers.hier.inPort.InPort_Ifc {
            @display("p=81,50");
            gates:
                out[numPorts-1];
                ctrl[numPorts-1];
        }
        vcCalc: <VCCalcType> like hnocs.routers.hier.vcCalc.VCCalc_Ifc {
            @display("p=51,135");
        }
        opCalc: <OPCalcType> like hnocs.routers.hier.opCalc.OPCalc_Ifc {
            @display("p=109,135");
        }
        sched: <schedType> like hnocs.routers.hier.sched.Sched_Ifc {
            @display("p=179,50");
            gates:
                in[numPorts-1];
                ctrl[numPorts-1];
                out[1];
        }
    connections:
        inPort.calcOp <--> opCalc.calc;
        inPort.calcVc <--> vcCalc.calc;
        inPort.in <--> in;
        sched.out[0] <--> out;
        //        sched.out <--> outPort.in;
        //        outPort.out <--> out;
        for i=0..numPorts-2 {
            inPort.out[i] --> sw_in[i];
            inPort.ctrl[i] <--> sw_ctrl_in[i];
            sched.in[i] <-- sw_out[i];
            sched.ctrl[i] <--> sw_ctrl_out[i];
        }
}
