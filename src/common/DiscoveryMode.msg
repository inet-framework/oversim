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


cplusplus {{
    #include <CommonMessages_m.h>
}}

class BaseCallMessage;
class BaseResponseMessage;

class noncobject TransportAddress;


enum DiscoveryNodesType
{
   CLOSE_NODES = 1;
   SPREADED_NODES = 2;
};

//
// TODO generated message class
//
packet DiscoveryCall extends BaseCallMessage {
    unsigned int nodesType enum(DiscoveryNodesType) = CLOSE_NODES;
    int numberOfNodes = -1;
    int index = -1;
}

//
// TODO generated message class
//
packet DiscoveryResponse extends BaseResponseMessage {
    TransportAddress nodes[];
    int maxIndex = -1;
}
