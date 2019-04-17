//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#include "applications/UDPAppBase.h"
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/transportlayer/contract/udp/UDPControlInfo_m.h"

void UDPAppBase::bindToPort(int port)
{
    EV << "Binding to UDP port " << port << endl;

    // TODO UDPAppBase should be ported to use UDPSocket sometime, but for now
    // we just manage the UDP socket by hand...
    cMessage *msg = new cMessage("UDP_C_BIND", UDP_C_BIND);
    UDPBindCommand *ctrl = new UDPBindCommand();
    ctrl->setLocalPort(port);
    ctrl->setSockId(UDPSocket::generateSocketId());
    msg->setControlInfo(ctrl);
    send(msg, "udpOut");
}

void UDPAppBase::sendToUDP(cPacket *msg, int srcPort, const L3Address& destAddr, int destPort)
{
    // send message to UDP, with the appropriate control info attached
    msg->setKind(UDP_C_DATA);

    UDPSendCommand *ctrl = new UDPSendCommand();
//    ctrl->setSrcPort(srcPort);
    ctrl->setDestAddr(destAddr);
    ctrl->setDestPort(destPort);
    msg->setControlInfo(ctrl);

    EV << "Sending packet: ";
    printPacket(msg);

    send(msg, "udpOut");
}

void UDPAppBase::printPacket(cPacket *msg)
{
    UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(msg->getControlInfo());

//    L3Address srcAddr = ctrl->getSrcAddr();
//    L3Address destAddr = ctrl->getDestAddr();
//    int srcPort = ctrl->getSrcPort();
//    int destPort = ctrl->getDestPort();

    EV << msg << "  (" << msg->getByteLength() << " bytes)" << endl;
//    EV  << srcAddr << " :" << srcPort << " --> " << destAddr << ":" << destPort << endl;
}


