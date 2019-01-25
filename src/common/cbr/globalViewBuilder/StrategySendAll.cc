//
// Copyright (C) 2010 Institut fuer Telematik, Universitaet Karlsruhe (TH)
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
 * @StrategySendAll.cc
 * @author Daniel Lienert
 */

#include <TreeManagement.h>

#include "StrategySendAll.h"


StrategySendAll::StrategySendAll()
{
    lastSendCount = 0;
}


StrategySendAll::~StrategySendAll()
{
    // TODO Auto-generated destructor stub
}


void StrategySendAll::setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo)
{
    std::vector<double> tmpDimVector = ncsInfo.getCoords();
    coordinatesVector tmpCoordsVector;
    tmpCoordsVector.push_back(tmpDimVector);

    // i am my own branch here ...
    setBranchCoordinates(thisNode, tmpCoordsVector);
}


void StrategySendAll::setBranchCoordinates(const NodeHandle& node,
                                           coordinatesVector coordsVector)
{
    if(coordData.find(node) != coordData.end()) {
        coordData.find(node)->second = coordsVector;
    } else {
        coordData.insert(nodeCoordData(node, coordsVector));
    }
}


GlobalViewBuilderCall* StrategySendAll::getCoordinateMessage()
{
    SendAllStrategyCall* msg = new SendAllStrategyCall("SendAllStrategyCall");

    CoordDataContainer tmpCoordContainer;
    tmpCoordContainer.coordinatesVector = getCombinedCoordsVector();

    lastSendCount = tmpCoordContainer.coordinatesVector.size();

    msg->setCoordData(tmpCoordContainer);
    msg->setBitLength(SENDALLSTRATEGYCALL_L(msg));

    return msg;
}


void StrategySendAll::handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall)
{
    SendAllStrategyCall* sendAllStrategyCall = dynamic_cast<SendAllStrategyCall*>(globalViewBuilderCall);
    setBranchCoordinates(sendAllStrategyCall->getSrcNode(), sendAllStrategyCall->getCoordData().coordinatesVector);
}


const std::vector<std::vector<double> > StrategySendAll::getCombinedCoordsVector()
{
    coordinatesVector combinedCoordsVector;
    coordinatesVector branchCoordsVector;

    coordDataMap::const_iterator coordDataMapIterator = coordData.begin();

    while(coordDataMapIterator != coordData.end()) {

        branchCoordsVector = coordDataMapIterator->second;

        coordinatesVector::iterator vIterator;
        for(vIterator=branchCoordsVector.begin(); vIterator < branchCoordsVector.end(); vIterator++) {
            combinedCoordsVector.push_back(*vIterator);
        }

        ++coordDataMapIterator;
    }
    return combinedCoordsVector;
}


void StrategySendAll::cleanUpCoordData(const treeNodeMap& currentTreeChildNodes)
{
    coordDataMap::const_iterator coordDataMapIterator = coordData.begin();

    while(coordDataMapIterator != coordData.end()) {
        if(currentTreeChildNodes.find(coordDataMapIterator->first) ==
           currentTreeChildNodes.end() && coordDataMapIterator->first != thisNode) {
            coordDataMapIterator = coordData.erase(coordDataMapIterator);
        } else ++coordDataMapIterator;
    }
}


std::string StrategySendAll::getStrategyDataStatus()
{
    std::stringstream tempStr;
    tempStr << "CoordCount: "  << lastSendCount;
    return tempStr.str();
}


std::string StrategySendAll::getStrategyCombinedParams()
{
    std::stringstream tempStr;
    tempStr << "sendAll";

    return tempStr.str();
}


int StrategySendAll::getSizeOfCoordVector(const coordinatesVector& combinedCoordsVector)
{
    int size = 0;

    coordinatesVector::const_iterator vNodeIterator = combinedCoordsVector.begin();
    std::vector<double>::const_iterator vCoordsIterator;

    while(vNodeIterator != combinedCoordsVector.end()) {

        size += sizeof((*vNodeIterator));

        vCoordsIterator = (*vNodeIterator).begin();
        while(vCoordsIterator != (*vNodeIterator).end()) {
            size += sizeof((*vCoordsIterator));
            vCoordsIterator++;
        }
        vNodeIterator++;
    }

    return size;
}


int StrategySendAll::getSizeOfSingleCoordinate(const coordinatesVector& combinedCoordsVector)
{
    return sizeof((*(*combinedCoordsVector.begin()).begin()));
}

std::vector<std::vector<double> > StrategySendAll::getGlobalViewData()
{
    return getCombinedCoordsVector();
}

