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

package hnocs.routers.hier.opCalc.static;

//
// Output Port Calculator - Implementing XY DOR
//
simple XYOPCalc like hnocs.routers.hier.opCalc.OPCalc_Ifc
{
    parameters:
        string portType; // actual Port_Ifc module required to tell a port from a Core
        string coreType; // actual Core_Ifc required to tell a port from a Core
    @display("i=block/fork");
    gates:
        inout calc;
}
