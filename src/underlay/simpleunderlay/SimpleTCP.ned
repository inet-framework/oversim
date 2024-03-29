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



package oversim.underlay.simpleunderlay;

import inet.transportlayer.tcp.TCP;

//
// UDP protocol implementation, for IPv4 and IPv6.
//
// The \UDP protocol header is represented by the class UDPPacket.
//
// <b>Communication with clients (applications)</b>
//
// The module can (should) be connected to several applications.
// For sending an \UDP packet, the application should attach an UDPControlInfo
// object to the payload, and send it to UDP.
// UDP will also attach an UDPControlInfo object to any payload
// message in sends up to the application.
//
// For receiving \UDP packets, the connected applications should first
// "bind" to the given \UDP port. This can be done by sending an
// arbitrary message with message kind UDP_C_BIND and an UDPControlInfo
// attached with srcPort filled in.
//
// If there is only one app which doesn't bind to any port, it will
// receive all packets.
//
// <b>Communication with other nodes</b>
//
// The SimpleUDP model sends packets directly to the destination
//
// @see SimpleUDPPacket, UDPControlInfo, GlobalRoutingHashMap
//
simple SimpleTCP extends TCP
{
    parameters:
        @class(SimpleTCP);
        double constantDelay @unit(s);  // constant delay between two peers
        bool useCoordinateBasedDelay;   // delay should be calculated from 
                                        // euklidean distance between two peers
        string delayFaultType;          // augment coordinate based delays with a realistic error,
                                        // according to "Network Coordinates in the Wild", Figure 7
                                        // possible values: empty, "live_all", "live_planetlab", "simulation"
        double jitter;                  // average amount of jitter in %
        tcpAlgorithmClass = default("inet::tcp::TCPReno"); // TCPReno/TCPTahoe/TCPNewReno/TCPNoCongestionControl/DumbTCP
        @display("i=block/wheelbarrow");
    gates:
}


