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
enum NOC_MSGS
{
  NOC_FLIT_MSG    = 1; 
  NOC_CREDIT_MSG  = 2; 
  NOC_REQ_MSG     = 3;
  NOC_GNT_MSG     = 4;
  NOC_ACK_MSG     = 5;
  NOC_POP_MSG     = 7;
  NOC_GEN_MSG     = 8;
  NOC_APP_MSG 	  = 9;
  NOC_CLK_MSG 	  = 10;
  
}

///////////////////////////////////////////////////////////////////////////
//
// MESSAGES THAT ARE AVAILABLE ON NOC WIRES (outside of the router)
//
enum NOC_FLIT_TYPES
{
    NOC_START_FLIT = 1;
    NOC_MID_FLIT   = 2;
    NOC_END_FLIT   = 3;
}

packet NoCFlitMsg
{
  int type;    // first middle end of packet
  int VC;      // the VC of the FLIT as on the wire
  int SL;      // the QoS SL for the FLIT
  int pktId;   // packet id globally unique
  int flits;   // number of flits in this message 
  int flitIdx; // index within the packet
  int srcId;
  int dstId;
  bool firstNet; 
  simtime_t InjectTime; // the time the flit is injected to the NoC , i.e: when it leaves the source`s queue. 
  simtime_t FirstNetTime; // the time the flit is transimitted by a sched,  in order to mask source-router latency effects 
   
}

message NoCCreditMsg extends cMessage
{
    int VC;
    int flits;
    int msgs;
}

///////////////////////////////////////////////////////////////////////////
//
// MESSAGES THAT ARE INTERNAL TO ROUTERS
//

// InPort Request arbitratoin from scheduler 
message NoCReqMsg extends cMessage 
{
    int outVC;      // the output VC to send on
    int inVC;       // the VC of the input buffer storing this packet
    int outPortNum; // the output port  
    int pktId;      // packet ID of the sent packet
    int numFlits;   // flits of the packet
    int numGranted; // number of flits granted
    int numAcked;   // number of flits acked 
    
}

// Scheduler Grant Arbitration to InPort
message NoCGntMsg extends cMessage 
{
    int outVC;      // the output VC to send on
    int inVC;       // the input VC to take the flit from
    int outPortNum; // the output port  
    
}

// each grant must be either declined or accepted using this ACK message
message NoCAckMsg extends cMessage 
{
    bool OK;        // grant can be declined but a response is a must ...
    int outVC;      // the output VC to send on
    int inVC;
    int outPortNum;
    
}

// a pop from a particular VC and OP
message NoCPopMsg extends cMessage
{
    int VC;
    int outPortNum;
}

//////////////////////////////////////////////////////////////////////////////////////////////
