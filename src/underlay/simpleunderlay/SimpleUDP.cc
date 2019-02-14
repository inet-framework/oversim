//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2005 Andras Varga
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

/**
 * @file SimpleUDP.cc
 * @author Jochen Reber
 */

//
// Author: Jochen Reber
// Rewrite: Andras Varga 2004,2005
// Modifications: Stephan Krause
//

#include "inet/common/INETDefs.h"

#include <CommonMessages_m.h>
#include <GlobalNodeListAccess.h>
#include <GlobalStatisticsAccess.h>

#include <SimpleInfo.h>
#include "inet/transportlayer/udp/UDPPacket.h"
#include "SimpleUDP.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"

#include "inet/networklayer/common/L3AddressResolver.h"

#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/ipv6/IPv6Datagram_m.h"

#include "inet/networklayer/ipv4/ICMPMessage_m.h"
#include "inet/networklayer/icmpv6/ICMPv6Message_m.h"


#define EPHEMERAL_PORTRANGE_START 1024
#define EPHEMERAL_PORTRANGE_END   5000

using namespace inet;

Define_Module( SimpleUDP );

std::string SimpleUDP::delayFaultTypeString;
std::map<std::string, SimpleUDP::delayFaultTypeNum> SimpleUDP::delayFaultTypeMap;

namespace inet {

inline std::ostream & operator<<(std::ostream & os,
                                 const inet::UDP::SockDesc& sd)
{
    os << "sockId=" << sd.sockId;
    os << " appGateIndex=" << sd.appGateIndex;
//    os << " userId=" << sd.userId;    //TODO KLUDGE
    os << " localPort=" << sd.localPort;
    if (sd.remotePort!=0)
        os << " remotePort=" << sd.remotePort;
    if (!sd.localAddr.isUnspecified())
        os << " localAddr=" << sd.localAddr;
    if (!sd.remoteAddr.isUnspecified())
        os << " remoteAddr=" << sd.remoteAddr;
//    if (sd.interfaceId!=-1)
//        os << " interfaceId=" << sd.interfaceId;      //TODO KLUDGE

    return os;
}

static std::ostream & operator<<(std::ostream & os, const UDP::SockDescList& list)
{
    for (SimpleUDP::SockDescList::const_iterator i=list.begin();
     i!=list.end(); ++i)
        os << "sockId=" << (*i)->sockId << " ";
    return os;
}

}

//--------

SimpleUDP::SimpleUDP()
{
    globalStatistics = NULL;
}

SimpleUDP::~SimpleUDP()
{

}

void SimpleUDP::initialize(int stage)
{
    if(stage == MIN_STAGE_UNDERLAY) {
        WATCH_PTRMAP(socketsByIdMap);
        WATCH_MAP(socketsByPortMap);

        lastEphemeralPort = EPHEMERAL_PORTRANGE_START;
        icmp = NULL;
        icmpv6 = NULL;

        numSent = 0;
        numPassedUp = 0;
        numDroppedWrongPort = 0;
        numDroppedBadChecksum = 0;
        numQueueLost = 0;
        numPartitionLost = 0;
        numDestUnavailableLost = 0;
        WATCH(numSent);
        WATCH(numPassedUp);
        WATCH(numDroppedWrongPort);
        WATCH(numDroppedBadChecksum);
        WATCH(numQueueLost);
        WATCH(numPartitionLost);
        WATCH(numDestUnavailableLost);

        globalNodeList = GlobalNodeListAccess().get();
        globalStatistics = GlobalStatisticsAccess().get();
        constantDelay = par("constantDelay");
        useCoordinateBasedDelay = par("useCoordinateBasedDelay");

        delayFaultTypeString = par("delayFaultType").stdstringValue();
        delayFaultTypeMap["live_all"] = delayFaultLiveAll;
        delayFaultTypeMap["live_planetlab"] = delayFaultLivePlanetlab;
        delayFaultTypeMap["(*getSimulation())"] = delayFaultSimulation;

        switch (delayFaultTypeMap[delayFaultTypeString]) {
        case SimpleUDP::delayFaultLiveAll:
        case SimpleUDP::delayFaultLivePlanetlab:
        case SimpleUDP::delayFaultSimulation:
            faultyDelay = true;
            break;
        default:
            faultyDelay = false;
            break;
        }

        jitter = par("jitter");
        enableAccessRouterTxQueue = par("enableAccessRouterTxQueue");
        nodeEntry = NULL;
        WATCH_PTR(nodeEntry);
    }
}

void SimpleUDP::finish()
{
    globalStatistics->addStdDev("SimpleUDP: Packets sent",
                                numSent);
    globalStatistics->addStdDev("SimpleUDP: Packets dropped with bad checksum",
                                numDroppedBadChecksum);
    globalStatistics->addStdDev("SimpleUDP: Packets dropped due to queue overflows",
                                numQueueLost);
    globalStatistics->addStdDev("SimpleUDP: Packets dropped due to network partitions",
                                numPartitionLost);
    globalStatistics->addStdDev("SimpleUDP: Packets dropped due to unavailable destination",
                                numDestUnavailableLost);
}

void SimpleUDP::updateDisplayString()
{
    char buf[80];
    sprintf(buf, "passed up: %d pks\nsent: %d pks", numPassedUp, numSent);
    if (numDroppedWrongPort>0) {
        sprintf(buf+strlen(buf), "\ndropped (no app): %d pks", numDroppedWrongPort);
        getDisplayString().setTagArg("i",1,"red");
    }
    if (numQueueLost>0) {
        sprintf(buf+strlen(buf), "\nlost (queue overflow): %d pks", numQueueLost);
        getDisplayString().setTagArg("i",1,"red");
    }
    getDisplayString().setTagArg("t",0,buf);
}

void SimpleUDP::sendUp(cPacket *payload, UDPControlInfo *udpCtrl, SockDesc *sd)
{
    // send payload with UDPControlInfo up to the application
    udpCtrl->setSockId(sd->sockId);
//    udpCtrl->setUserId(sd->userId);   //TODO KLUDGE
    payload->setControlInfo(udpCtrl);

    send(payload, "appOut", sd->appGateIndex);
    numPassedUp++;
}

void SimpleUDP::processUndeliverablePacket(cPacket *udpPacket, cObject *ctrl)
{
    numDroppedWrongPort++;
    EV << "[SimpleUDP::processUndeliverablePacket()]\n"
       << "    Dropped packet bound to unreserved port, ignoring ICMP error"
       << endl;

    delete udpPacket;
    delete ctrl;
}

void SimpleUDP::processUDPPacket(cPacket *udpPacket)
{
    int srcPort, destPort;
    L3Address srcAddr, destAddr;

    UDPDataIndication *ctrl = check_and_cast<UDPDataIndication *>(udpPacket->removeControlInfo());

    srcPort = ctrl->getSrcPort();
    destPort = ctrl->getDestPort();

	srcAddr = ctrl->getSrcAddr();
	destAddr = ctrl->getDestAddr();

    // simulate checksum: discard packet if it has bit error
    EV << "Packet " << udpPacket->getName() << " received from network, dest port " << destPort << "\n";
    if (udpPacket->hasBitError())
    {
        EV << "Packet has bit error, discarding\n";
        delete udpPacket;
        numDroppedBadChecksum++;
        return;
    }

    // send back ICMP error if no socket is bound to that port
    SocketsByPortMap::iterator it = socketsByPortMap.find(destPort);
    if (it==socketsByPortMap.end())
    {
        EV << "No socket registered on port " << destPort << "\n";
        processUndeliverablePacket(udpPacket, ctrl);
        return;
    }

    // deliver a copy of the packet to each matching socket

    if (destAddr.getType() == L3Address::AddressType::IPv6)
    {
        // packet size is increased in processMsgFromApp
    	udpPacket->setByteLength(udpPacket->getByteLength() - UDP_HEADER_BYTES - IPv6_HEADER_BYTES);
    }
    else
    {
    	udpPacket->setByteLength(udpPacket->getByteLength() - UDP_HEADER_BYTES - IP_HEADER_BYTES);
    }

    SockDesc *sd = findSocketForUnicastPacket(destAddr, destPort, srcAddr, srcPort);
    if (!sd) {
        EV_WARN << "No socket registered on port " << destPort << "\n";
        processUndeliverablePacket(udpPacket, ctrl);
        return;
    }
    else {
        sendUp(udpPacket, ctrl, sd);
    }
}

void SimpleUDP::processMsgFromApp(cPacket *appData)
{
    cModule *node = getParentModule();

    L3Address srcAddr, destAddr;

    UDPSendCommand *udpCtrl = check_and_cast<UDPSendCommand *>(appData->removeControlInfo());

    int sockId = udpCtrl->getSockId();
    auto it = socketsByIdMap.find(sockId);
    if (it == socketsByIdMap.end())
            throw cRuntimeError("socket not found (sockId=%d)", sockId);
    SockDesc *sd = it->second;
    if (!sd->isBound)
        throw cRuntimeError("socket not bound (sockId=%d)", sockId);

    srcAddr = udpCtrl->getSrcAddr();
    if (srcAddr.isUnspecified())
        srcAddr = sd->localAddr;
    int srcPort = sd->localPort;
    destAddr = udpCtrl->getDestAddr();
    if (destAddr.isUnspecified())
        destAddr = sd->remoteAddr;
    int destPort = udpCtrl->getDestPort();
    if (destPort == -1)
        destPort = sd->remotePort;
    delete udpCtrl;

    // add header byte length for the skipped IP and UDP headers (decreased in processUDPPacket)
    if (destAddr.getType() == L3Address::AddressType::IPv6) {
        appData->setByteLength(appData->getByteLength() + UDP_HEADER_BYTES + IPv6_HEADER_BYTES);
    } else {
        appData->setByteLength(appData->getByteLength() + UDP_HEADER_BYTES + IP_HEADER_BYTES);
    }

    /* main modifications for SimpleUDP start here */

    SimpleInfo* info = dynamic_cast<SimpleInfo*>(globalNodeList->getPeerInfo(destAddr));
    numSent++;

    if (info == NULL) {
        EV << "[SimpleUDP::processMsgFromApp() @ " << L3AddressResolver().addressOf(node) << "]\n"
           << "    No route to host " << destAddr
           << endl;

        delete appData;
        numDestUnavailableLost++;
        return;
    }

    SimpleNodeEntry* destEntry = info->getEntry();

    // calculate delay
    simtime_t totalDelay = 0;
    if (srcAddr != destAddr) {
        SimpleNodeEntry::SimpleDelay temp;
        if (faultyDelay) {
            SimpleInfo* thisInfo = static_cast<SimpleInfo*>(globalNodeList->getPeerInfo(srcAddr));
            temp = nodeEntry->calcDelay(appData, *destEntry,
                                        !(thisInfo->getNpsLayer() == 0 ||
                                          info->getNpsLayer() == 0)); //TODO
        } else {
            temp = nodeEntry->calcDelay(appData, *destEntry);
        }
        if (useCoordinateBasedDelay == false) {
            totalDelay = constantDelay;
        } else if (temp.second == false) {
            EV << "[SimpleUDP::processMsgFromApp() @ " << L3AddressResolver().addressOf(node) << "]\n"
               << "    Send queue full: packet " << appData << " dropped"
               << endl;

            delete appData;
            numQueueLost++;
            return;
        } else {
            totalDelay = temp.first;
        }
    }

    SimpleInfo* thisInfo = dynamic_cast<SimpleInfo*>(globalNodeList->getPeerInfo(srcAddr));

    if (!globalNodeList->areNodeTypesConnected(thisInfo->getTypeID(), info->getTypeID())) {
        EV << "[SimpleUDP::processMsgFromApp() @ " << L3AddressResolver().addressOf(node) << "]\n"
                   << "    Partition " << thisInfo->getTypeID() << "->" << info->getTypeID()
                   << " is not connected"
                   << endl;

        delete appData;
        numPartitionLost++;
        return;
    }

    if (jitter) {
        // jitter
        //totalDelay += truncnormal(0, SIMTIME_DBL(totalDelay) * jitter);

        //workaround (bug in truncnormal(): sometimes returns inf)
        double temp = truncnormal(0, SIMTIME_DBL(totalDelay) * jitter);
        while (temp == INFINITY || temp != temp) { // reroll if temp is INF or NaN
            std::cerr << "\n******* SimpleUDP: truncnormal() -> inf !!\n"
                      << std::endl;
            temp = truncnormal(0, SIMTIME_DBL(totalDelay) * jitter);
        }

        totalDelay += temp;
    }

    BaseOverlayMessage* temp = NULL;

    if (hasGUI() && appData) {
        if ((temp = dynamic_cast<BaseOverlayMessage*>(appData))) {
            switch (temp->getStatType()) {
            case APP_DATA_STAT:
                appData->setKind(1);
                break;
            case APP_LOOKUP_STAT:
                appData->setKind(2);
                break;
            case MAINTENANCE_STAT:
            default:
                appData->setKind(3);
                break;
            }
        } else {
            appData->setKind(1);
        }
    }

    EV << "[SimpleUDP::processMsgFromApp() @ " << L3AddressResolver().addressOf(node) << "]\n"
       << "    Packet " << appData << " sent with delay = " << SIMTIME_DBL(totalDelay)
       << endl;

    //RECORD_STATS(globalStatistics->addStdDev("SimpleUDP: delay", totalDelay));

    /* main modifications for SimpleUDP end here */

    UDPDataIndication *udpDataIndication = new UDPDataIndication();
    udpDataIndication->setDestAddr(destAddr);
    udpDataIndication->setDestPort(destPort);
    udpDataIndication->setSrcAddr(srcAddr);
    udpDataIndication->setSrcPort(srcPort);
    appData->setKind(UDP_I_DATA);
    appData->setControlInfo(udpDataIndication);
    // send directly to IPv4/IPv6 gate of the destination node
    sendDirect(appData, totalDelay, 0, destEntry->getUdpIpGate());
}


void SimpleUDP::handleMessage(cMessage *msg)
{
    // received from IP layer
    if (msg->arrivedOn("ipIn") || msg->arrivedOn("ipv6In"))
    {
        SimpleNodeEntry::SimpleDelay temp = std::make_pair(0, true);

        if (enableAccessRouterTxQueue) {
            nodeEntry->calcAccessRouterDelay(PK(msg));
        }

        if (temp.second == false) {
            delete msg;
            return;
        }

        if (temp.first > 0) {
            scheduleAt(simTime() + temp.first, msg);
        } else if (dynamic_cast<ICMPMessage *>(msg) || dynamic_cast<ICMPv6Message *>(msg))
            processICMPError(PK(msg));
        else
            processUDPPacket(PK(msg));
    }
    else if (msg->isSelfMessage())
    {
        if (dynamic_cast<ICMPMessage *>(msg) || dynamic_cast<ICMPv6Message *>(msg))
            processICMPError(PK(msg));
        else
            processUDPPacket(PK(msg));

    }
    else // received from application layer
    {
        if (msg->getKind()==UDP_C_DATA)
            processMsgFromApp(PK(msg));
        else
            processCommandFromApp(msg);
    }

    if (hasGUI())
        updateDisplayString();
}


void SimpleUDP::setNodeEntry(SimpleNodeEntry* entry)
{
    nodeEntry = entry;
}
