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

#ifndef __HNOCS_HIER_ROUTER_H_
#define __HNOCS_HIER_ROUTER_H_
#include <omnetpp.h>
// we need extra info inside the InPort for tracking FLITs
class Sched : public cSimpleModule {
public:
	// pure virtual...
	virtual const std::vector<int> *getCredits() const = 0;
	virtual const std::vector<int> *getVCUsage() const = 0;
	virtual void  incrVCUsage(int vc) = 0;
};

#endif /* __HNOCS_HIER_ROUTER_H_ */
