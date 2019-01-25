//
// Copyright (C) 2006 Institut fuer Telematik, Universitaet Karlsruhe (TH)
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

cplusplus {{
#include<CommonMessages_m.h>

#define PROXFINDNODECALL_L(msg) (FINDNODECALL_L(msg)) 
#define PROXFINDNODERESPONSE_L(msg) (FINDNODERESPONSE_L(msg) + \
                                     (msg->getNextHopNcsInfosArraySize() ? \
                                      msg->getNextHopNcsInfosArraySize() * 32 : \
                                      0))
}}

class FindNodeCall;
class FindNodeResponse;
class ProxMSG;

enum ProximityLookupMetrik
{
    NONE = 0;                       // No metric is choosen
    BY_KEY = 1;                     // Metric compares by key
    BY_KEY_X = 2;                   // Metric compares by key and choose the best proximity out of X nodes
    BY_PROXIMITY = 3;               // Metric compares by proximity
    BY_PROXIMITY_X = 4;             // Metric compares by proximity and choose the best NodeID out of X nodes
    BY_KEY_AND_PROXIMITY = 5;       // Metric compares by key and then by proximity
    BY_KEY_AND_PROXIMITY_WEIGHT = 6;// Metric calculates a compar value from key and proximity
}

//
// A basic prox find node rpc call
//
packet ProxFindNodeCall extends FindNodeCall
{
}

//
// A basic prox find node rpc response
//
packet ProxFindNodeResponse extends FindNodeResponse
{
        double nextHopNcsInfos[];
}
