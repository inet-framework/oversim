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

import oversim.common.NeighborCache;
import oversim.common.ITier;
import oversim.common.IOverlay;
import oversim.common.CryptoModule;
import oversim.common.BootstrapList;
import inet.transportlayer.udp.UDP;
import inet.transportlayer.tcp.TCP;
import inet.networklayer.ipv6.IPv6NetworkLayer;
import inet.networklayer.ipv6.IPv6RoutingTable;
import inet.networklayer.common.InterfaceTable;
import inet.linklayer.ppp.PPPInterface;


//
// Router in the overlay network
//
module OverlayRouter6
{
    parameters:
        @networkNode;
        string overlayType;    // overlay protocol compound module to use
        string tier1Type;    // the application on top off the overly used in the simulation
        string tier2Type;    // tier 2 module to use
        string tier3Type;    // tier 3 module to use
        int numTiers;    // number of tiers

        @display("bgb=360,476;i=abstract/router");
    gates:
        inout pppg[];    // gates from overlay
        inout ethg[];    // placeholder for zero-size vector
        input overlayNeighborArrowIn[]; // incoming gate for visualizing overlay neighborship with connection arrows
        output overlayNeighborArrowOut[]; // incoming gate for visualizing overlay neighborship with connection arrows

    submodules:
        tier3: <tier3Type> like ITier {
            parameters:
                @display("p=48,64");
        }
        tier2: <tier2Type> like ITier {
            parameters:
                @display("p=139,96");
        }
        tier1: <tier1Type> like ITier {
            parameters:
                @display("p=218,130");
        }
        overlay: <overlayType> like IOverlay {
            parameters:
                @display("p=282,184");
        }
        udp: UDP {
            parameters:
                @display("p=282,262");
        }
        tcp: TCP {
            parameters:
                @display("p=230,262");
        }
        networkLayer: IPv6NetworkLayer {
            parameters:
                @display("p=282,340;q=queue");
            gates:
                ifIn[sizeof(pppg)+sizeof(ethg)];
                ifOut[sizeof(pppg)+sizeof(ethg)];
        }
        interfaceTable: InterfaceTable {
            parameters:
                @display("p=84,278");
        }
        routingTable6: IPv6RoutingTable {
            parameters:
                isRouter = true;
                @display("p=84,348");
        }
        ppp[sizeof(pppg)]: PPPInterface {
            parameters:
                @display("p=282,422,row,90;q=txQueue");
        }
        neighborCache: NeighborCache {
            parameters:
                @display("p=168,422");
        }
        bootstrapList: BootstrapList {
            parameters:
                @display("p=168,278");

        }
        cryptoModule: CryptoModule {
            parameters:
                @display("p=168,348");
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
        for i=0..sizeof(pppg)-1 {
            pppg[i] <--> ppp[i].phys;
            ppp[i].upperLayerOut --> networkLayer.ifIn[i];
            ppp[i].upperLayerIn <-- networkLayer.ifOut[i];
        }

}

