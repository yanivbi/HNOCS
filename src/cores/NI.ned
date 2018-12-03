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

// A Generic Network Interface

package hnocs.cores;

module NI like NI_Ifc
{
    parameters:
        // The identifier of the core. Should be globally unique.
        // NOTE: routing engines may impose requirements on its coding
        int id; 
        string sourceType;
        string sinkType;
    @display("i=block/square");
    gates:
        inout in;
        inout out;

    submodules:
        source: < sourceType > like hnocs.cores.sources.Source_Ifc {
            parameters:
                srcId = id;
            @display("p=53,46");
        }
        sink: < sinkType > like hnocs.cores.sinks.Sink_Ifc {
            @display("p=53,112");
        }
    connections:
        source.out <--> out;
        sink.in <--> in;
}
