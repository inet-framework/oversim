//
// Copyright (C) 2012 Institute of Telematics, Karlsruhe Institute of Technology (KIT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

//
// @author Felix Palmen, Bernhard Heep
//

cplusplus {{
#include <IPvXAddress.h>
#include <NodeHandle.h>
#include <OverlayKey.h>
#include <CommonMessages_m.h>

// constants for message length in bit
static const int PASTRYTYPE_L = 8;
static const int LASTHOPFLAG_L = 8;
static const int TIMESTAMP_L = 32;
static const int ROWNUMBER_L = 16;
static const int PASTRYFINDNODEEXTDATA_L = NODEHANDLE_L + HOPCOUNT_L;

#define PASTRY_L(msg) (BASEOVERLAY_L(msg) + PASTRYTYPE_L)

#define PASTRYSTATE_L(msg) (PASTRY_L(msg) + TRANSPORTADDRESS_L \
    + NODEHANDLE_L * \
    (msg->getRoutingTableArraySize() + msg->getLeafSetArraySize() \
    + msg->getNeighborhoodSetArraySize()) + 3 * ARRAYSIZE_L + HOPCOUNT_L + \
    LASTHOPFLAG_L + TIMESTAMP_L)

#define PASTRYNEWLEAFS_L(msg) (msg->getLeafsArraySize() * NODEHANDLE_L + \
        ARRAYSIZE_L)

#define PASTRYJOINCALL_L(msg) PASTRY_L(msg)
#define PASTRYJOINRESPONSE_L(msg) PASTRY_L(msg) 

#define PASTRYREQUESTSTATECALL_L(msg) PASTRY_L(msg)
#define PASTRYREQUESTSTATERESPONSE_L(msg) PASTRY_L(msg)

#define PASTRYREQUESTREPAIRCALL_L(msg) PASTRY_L(msg)
#define PASTRYREQUESTREPAIRRESPONSE_L(msg) PASTRY_L(msg)

#define PASTRYREQUESTLEAFSETCALL_L(msg) PASTRY_L(msg)
#define PASTRYREQUESTLEAFSETRESPONSE_L(msg) PASTRY_L(msg)

#define PASTRYREQUESTROUTINGROWCALL_L(msg) (PASTRY_L(msg) + ROWNUMBER_L)
#define PASTRYREQUESTROUTINGROWRESPONSE_L(msg) PASTRY_L(msg)
}}


class noncobject IPvXAddress;
class noncobject TransportAddress;
class noncobject NodeHandle;
class noncobject OverlayKey;

class BaseOverlayMessage;
class BaseCallMessage;
class BaseResponseMessage;
class BaseAppDataMessage;


enum PastryStateMsgType
{
    PASTRY_STATE_STD        = 0x01;
    PASTRY_STATE_JOIN       = 0x02;
    PASTRY_STATE_MINJOIN    = 0x04;
    PASTRY_STATE_UPDATE     = 0x08;
    PASTRY_STATE_REPAIR     = 0x10;
    PASTRY_STATE_JOINUPDATE = 0x20;
    PASTRY_STATE_LEAFSET    = 0x40;
    PASTRY_STATE_ROUTINGROW = 0x80;
}


//
// Message used to send a PastryState
//
packet PastryStateMessage extends BaseOverlayMessage
{
    int pastryStateMsgType @enum(PastryStateMsgType) = PASTRY_STATE_STD;    // the type of the PastryStateMessage
    NodeHandle sender = NodeHandle::UNSPECIFIED_NODE;    // NodeHandle of the node sending this message
    NodeHandle routingTable[];    // the routingTable of the sender
    NodeHandle leafSet[];         // the leafSet of the sender
    NodeHandle neighborhoodSet[]; // the neighborhoodSet of the sender
    int row = 0;                  // row or number of hops this message takes
    bool lastHop = false;         // is this node the destination node?
    simtime_t timestamp;          // simTime when sending this message
}


//
// Message used to find a Pastry node
//
packet PastryFindNodeExtData
{
    TransportAddress sendStateTo = TransportAddress::UNSPECIFIED_NODE;    // the sender of this message
    int joinHopCount = 0;    // counts the hops this message takes
}


//
// Message used to inform about new PastryLeafs
//
packet PastryNewLeafsMessage
{
    NodeHandle leafs[];    // the new PastryLeafs
}


message PastrySendState
{
    TransportAddress dest = TransportAddress::UNSPECIFIED_NODE;
}


packet PastryJoinCall extends BaseCallMessage
{
}


packet PastryJoinResponse extends BaseResponseMessage
{
}


packet RequestStateCall extends BaseCallMessage
{
}


packet RequestStateResponse extends BaseResponseMessage
{
}


packet RequestRepairCall extends BaseCallMessage
{
}


packet RequestRepairResponse extends BaseResponseMessage
{
}


packet RequestLeafSetCall extends BaseCallMessage
{
}


packet RequestLeafSetResponse extends BaseResponseMessage
{
}


packet RequestRoutingRowCall extends BaseCallMessage
{
    int row;
}


packet RequestRoutingRowResponse extends BaseResponseMessage
{
}
