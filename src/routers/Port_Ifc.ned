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

// NoC Router Port Interface

package hnocs.routers;

moduleinterface Port_Ifc
{
    parameters:
        int numPorts;
        @display("i=block/arrival;bgl=2");
    gates:
        inout in;
        inout out;
        output sw_in[numPorts-1];
        input  sw_out[numPorts-1];
		inout  sw_ctrl_in[numPorts-1];
		inout  sw_ctrl_out[numPorts-1];
}
