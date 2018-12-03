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

// Created: 2010-2-30 for project HNOCS

package hnocs.topologies;

import hnocs.routers.Router_Ifc;
import hnocs.cores.NI_Ifc;

import ned.DelayChannel;

// we use same Link channel for the mesh
channel Link extends ned.DatarateChannel
{
    datarate = 16Gbps; // 32 lines of 2ns clock
    delay = 0us;
}

//
// A generated network with grid topology.
//
network Mesh
{
    parameters:
        string routerType;
        string coreType;
        int columns = default(4);
        int rows = default(4);
    submodules:
        router[columns*rows]: <routerType> like Router_Ifc {
            parameters:
                numPorts = 5;
                id = index;
                @display("p=100,100,matrix,$columns,150,150");
            gates:
                in[5];
                out[5];
        }
        core[columns*rows]: <coreType> like NI_Ifc {
            parameters:
                id = index;
                @display("p=150,150,matrix,$columns,150,150");
        }

    connections allowunconnected:
        for r=0..rows-1, for c=0..columns-1 {
            // ports on routers are 0 = north, 1 = west, 2 = south, 3 = east, 4 = core
            // connect south north (all but last row)
            router[r*columns+c].in[2] <--> Link <--> router[(r+1)*columns+c].out[0] if r!=rows-1;
            router[r*columns+c].out[2] <--> Link <--> router[(r+1)*columns+c].in[0] if r!=rows-1;
            // connect east west (all but on last column)
            router[r*columns+c].in[3] <--> Link <--> router[r*columns+c+1].out[1] if c!=columns-1;
            router[r*columns+c].out[3] <--> Link <--> router[r*columns+c+1].in[1] if c!=columns-1;

            // connect the Cores to port 4
            router[r*columns+c].in[4] <--> Link <--> core[r*columns+c].out;
            router[r*columns+c].out[4] <--> Link <--> core[r*columns+c].in;
        }
}
