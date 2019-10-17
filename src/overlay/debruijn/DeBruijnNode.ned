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

/**
 * @file DeBruijnNode.ned
 * @author Jan Peter Drees
 */

package oversim.overlay.debruijn;

import oversim.overlay.simpleOverlay.SimpleOverlayNode;
import oversim.common.IOverlay;

//
// A De Bruijn Node is an expanded variant of the SimpleOverlayNode
//
simple DeBruijnNode extends SimpleOverlayNode
{
    // Connect this model with our c++ class
    parameters:
        
        @class(DeBruijnNode); //< connects our module with our c++ class for the logic

        int nodeNumber; //< Parameter for the total number of nodes
        int dimensions = default(3);
        int visnode = default(-1);
        bool darkcolors = default(false);
        bool showq = default(true);
        bool showlist = default(true);
        bool showdb = default(true);
        bool showgeneraldb = default(true);
}

//
// This module embeds our Overlay node in the Simulation
//
module DeBruijnNodeModule like IOverlay
{
    // gates to the other layers of a node
    gates:
        input udpIn;   // gate from the UDP layer
        output udpOut;    // gate to the UDP layer
        input tcpIn;    // gate from the TCP layer
        output tcpOut;    // gate to the TCP layer
        input appIn;   // gate from the application
        output appOut;    // gate to the application

    // the module that holds the overlay protocol    
    submodules:
        deBruijnNode: DeBruijnNode;
        
    // connects the overlay module with the other layers  
    connections allowunconnected:
        udpIn -->  deBruijnNode.udpIn;
        udpOut <--  deBruijnNode.udpOut;
        appIn -->  deBruijnNode.appIn;
        appOut <--  deBruijnNode.appOut;
}
