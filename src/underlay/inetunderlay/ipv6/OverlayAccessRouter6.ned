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

package oversim.underlay.inetunderlay.ipv6;

import inet.linklayer.ethernet.EthernetInterface;
import inet.linklayer.loopback.LoopbackInterface;
import inet.linklayer.ppp.PPPInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv6.IPv6NetworkLayer;
import inet.networklayer.ipv6.IPv6RoutingTable;
import inet.transportlayer.tcp.TCP;
import inet.transportlayer.udp.UDP;
import oversim.common.BootstrapList;
import oversim.common.CryptoModule;
import oversim.common.IOverlay;
import oversim.common.ITier;
import oversim.common.NeighborCache;
import oversim.underlay.inetunderlay.AccessNet;


//
// Access router that participates in the overlay
//
// @author Markus Mauch, Bernhard Heep
//
module OverlayAccessRouter6
{
    parameters:
        @networkNode;
        string overlayType; // overlay protocol compound module to use
        string tier1Type; // tier 1 application to use
        string tier2Type; // tier 2 module to use
        string tier3Type; // tier 3 module to use
        int numTiers; // number of tiers

        @display("bgb=361,464");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(absPath(".routingTable6"));
    gates:
        inout pppg[]; // gates from overlay
        inout ethg[]; // placeholder for zero-size vector
        input overlayNeighborArrowIn[]; // incoming gate for visualizing overlay neighborship with connection arrows
        output overlayNeighborArrowOut[]; // incoming gate for visualizing overlay neighborship with connection arrows

    submodules:
        accessNet: AccessNet {
            parameters:
                useIPv6Addresses = true;
                @display("p=76,402");
        }
        tier3: <tier3Type> like ITier {
            parameters:
                @display("p=56,64;i=block/segm");
        }
        tier2: <tier2Type> like ITier {
            parameters:
                @display("p=139,88;i=block/segm");
        }
        tier1: <tier1Type> like ITier {
            parameters:
                @display("p=218,122;i=block/segm");
        }
        overlay: <overlayType> like IOverlay {
            parameters:
                @display("p=290,184;i=block/network2");
        }
        udp: UDP {
            parameters:
                @display("p=290,262;i=block/transport");
        }
        tcp: TCP {
            parameters:
                @display("p=233,262");
        }
        networkLayer: IPv6NetworkLayer {
            parameters:
                @display("p=290,340;i=block/fork;q=queue");
            gates:
                ifIn[sizeof(pppg)+sizeof(ethg)];
                ifOut[sizeof(pppg)+sizeof(ethg)];
        }
        interfaceTable: InterfaceTable {
            parameters:
                @display("p=76,262;i=block/table");
        }
        routingTable6: IPv6RoutingTable {
            parameters:
                isRouter = true;
                @display("p=76,340;i=block/table");
        }
        lo0: LoopbackInterface {
            @display("p=269,414");
        }
        ppp[sizeof(pppg)]: PPPInterface {
            parameters:
                @display("p=290,414,row,90;q=txQueue;i=block/ifcard");
        }
        eth[sizeof(ethg)]: EthernetInterface {
            parameters:
                @display("p=239,414,row,110;q=queue;i=block/ifcard");
        }
        neighborCache: NeighborCache {
            parameters:
                @display("p=168,262;i=block/table");
        }
        bootstrapList: BootstrapList {
            parameters:
                @display("p=168,340;i=block/table");
        }
        cryptoModule: CryptoModule {
            parameters:
                @display("p=168,402");
        }
    connections allowunconnected:
        tier1.to_lowerTier --> overlay.appIn if numTiers>0;
        tier1.from_lowerTier <-- overlay.appOut if numTiers>0;
        tier1.udpOut --> udp.appIn++ if numTiers>0;
        udp.appOut++ --> tier1.udpIn if numTiers>0;
        tier1.tcpOut --> tcp.appIn++ if numTiers>0;
        tcp.appOut++ --> tier1.tcpIn if numTiers>0;

        tier2.to_lowerTier --> tier1.from_upperTier if numTiers > 1;
        tier2.from_lowerTier <-- tier1.to_upperTier if numTiers > 1;
        tier2.udpOut --> udp.appIn++ if numTiers>1;
        udp.appOut++ --> tier2.udpIn if numTiers>1;
        tier2.tcpOut --> tcp.appIn++ if numTiers>1;
        tcp.appOut++ --> tier2.tcpIn if numTiers>1;

        tier3.to_lowerTier --> tier2.from_upperTier if numTiers > 2;
        tier3.from_lowerTier <-- tier2.to_upperTier if numTiers > 2;
        tier3.udpOut --> udp.appIn++ if numTiers>2;
        udp.appOut++ --> tier3.udpIn if numTiers>2;
        tier3.tcpOut --> tcp.appIn++ if numTiers>2;
        tcp.appOut++ --> tier3.tcpIn if numTiers>2;

        overlay.udpOut --> udp.appIn++;
        overlay.udpIn <-- udp.appOut++;

        overlay.tcpOut --> tcp.appIn++;
        overlay.tcpIn <-- tcp.appOut++;

        bootstrapList.udpOut --> udp.appIn++;
        bootstrapList.udpIn <-- udp.appOut++;

        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;

        tcp.ipOut --> networkLayer.transportIn++;
        tcp.ipIn <-- networkLayer.transportOut++;

        // connections to network outside
        networkLayer.ifOut++ --> lo0.upperLayerIn;
        lo0.upperLayerOut --> networkLayer.ifIn++;

        for i=0..sizeof(pppg)-1 {
            pppg[i] <--> { @display("m=s"); } <--> ppp[i].phys;
            ppp[i].upperLayerOut --> networkLayer.ifIn++;
            ppp[i].upperLayerIn <-- networkLayer.ifOut++;
        }

        for i=0..sizeof(ethg)-1 {
            ethg[i] <--> { @display("m=s"); } <--> eth[i].phys;
            eth[i].upperLayerOut --> networkLayer.ifIn++;
            eth[i].upperLayerIn <-- networkLayer.ifOut++;
        }
}

