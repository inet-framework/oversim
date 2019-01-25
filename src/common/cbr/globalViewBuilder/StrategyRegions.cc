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
 * @StrategyRegions.cc
 * @author Daniel Lienert
 */

#include <iostream>
#include <fstream>
#include "StrategyRegions.h"
#include <GlobalViewBuilder.h>

StrategyRegions::StrategyRegions()
{
    EV << "|DD|> StrategyRegions::StrategyRegions (Using Send Strategy 'Regions') <||" << endl;
}


StrategyRegions::~StrategyRegions()
{
    // TODO Auto-generated destructor stub
}


GlobalViewBuilderCall* StrategyRegions::getCoordinateMessage()
{
    RegionsStrategyCall* msg = new RegionsStrategyCall("RegionsStrategyCall");
    RegionDataContainer tmpRegionContainer;
    tmpRegionContainer.regionData = getCombinedRegionsMap();
    msg->setRegionData(tmpRegionContainer);
    msg->setBitLength(REGIONSSTRATEGYCALL_L(msg));

    return msg;
}


std::string StrategyRegions::getStrategyCombinedParams()
{
    std::stringstream tempStr;
    tempStr << globalViewBuilder->parProxy("gvbSendStrategy").stdstringValue();
    tempStr << "/";
    tempStr << getMaxSpread();
    tempStr << "/";
    tempStr << getSizePerDim();
    return tempStr.str();
}


const regionCountMap StrategyRegions::getCombinedRegionsMap()
{
    regionCountMap combinedRegionMap;
    regionCountMap branchRegionMap;

    branchRegionDataMap::const_iterator regionDataMapIterator = regionData.begin();

    while(regionDataMapIterator != regionData.end()) {
        branchRegionMap = regionDataMapIterator->second;

        regionCountMap::iterator bRMapIterator = branchRegionMap.begin();

        while(bRMapIterator != branchRegionMap.end()) {
            if(combinedRegionMap.find(bRMapIterator->first) == combinedRegionMap.end()) {
                combinedRegionMap.insert(regionCountPair(bRMapIterator->first, bRMapIterator->second));
            } else {
                combinedRegionMap.find(bRMapIterator->first)->second += bRMapIterator->second;
            }

            bRMapIterator++;
        }
        regionDataMapIterator++;
    }

    return combinedRegionMap;
}


void StrategyRegions::handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall)
{
    RegionsStrategyCall* regionsStrategyCall =
        dynamic_cast<RegionsStrategyCall*>(globalViewBuilderCall);
    setBranchCoordinates(regionsStrategyCall->getSrcNode(),
                         regionsStrategyCall->getRegionData().regionData);
}


std::string StrategyRegions::getStrategyDataStatus()
{
    std::stringstream tempStr;

    regionCountMap tmpRCMap = getCombinedRegionsMap();
    regionCountMap::iterator rcmIterator = tmpRCMap.begin();

    while (rcmIterator != tmpRCMap.end()) {
        tempStr << rcmIterator->first << ':' << rcmIterator->second << ", ";
        rcmIterator++;
    }

    tempStr << " CRM:" << tmpRCMap.size() << " BC:" << regionData.size();

    return tempStr.str();
}


std::vector<std::vector<double> > StrategyRegions::getGlobalViewData()
{
    regionCountMap tmpRCMap = getCombinedRegionsMap();

    // measurement of CCD size
    /*
    std::fstream f;
    std::string name("CCD_");
    name += simTime().str();
    name += ".bin";
    f.open(name.c_str(), std::ios::binary|std::ios::out);
    for (regionCountMap::const_iterator it = tmpRCMap.begin(); it != tmpRCMap.end(); ++it) {
        f.write((char*)&(it->first), sizeof(it->first));
        f.write((char*)&(it->second), sizeof(it->second));
    }
    f.close();
     */

    regionCountMap::iterator rcmIterator = tmpRCMap.begin();

    std::vector<std::vector<double> > globalViewData;

    while(rcmIterator != tmpRCMap.end()) {
        for(int i = 0; i < rcmIterator->second; i++) {
            globalViewData.push_back(convertRegionToCoordVector(rcmIterator->first, 2));
            // TODO get the source dimension (2) dynamic from configuration
        }
        rcmIterator++;
    }
    return globalViewData;
}

void StrategyRegions::setMyCoordinates(const AbstractNcsNodeInfo& ncsInfo)
{
    std::vector<double> tmpDimVector = ncsInfo.getCoords();
    regionCountMap tmpCountMap;
    tmpCountMap.insert(regionCountPair(convertCoordVectorToRegion(tmpDimVector),1));

    setBranchCoordinates(thisNode, tmpCountMap);	// i am my own branch here ...
}


void StrategyRegions::setBranchCoordinates(const NodeHandle& node, regionCountMap countMap)
{
    if(regionData.find(node) != regionData.end()) {
        regionData.find(node)->second = countMap;
    } else {
        regionData.insert(nodeRegionData(node, countMap));
    }
}


double StrategyRegions::checkMaxSpread(double dimValue)
{
    double maxSpread = static_cast<double>(getMaxSpread());

    if(dimValue < 0.0) {
        if(dimValue > (maxSpread * -1.0)) {
            return dimValue;
        } else {
            return (maxSpread * -1.0);
        }

    } else {
        if(dimValue < maxSpread) {
            return dimValue;
        } else {
            return maxSpread;
        }
    }
}


long StrategyRegions::convertCoordVectorToRegion(std::vector<double> coordVector)
{
    int sizePerDim = getSizePerDim();
    int rangeShift = static_cast<int> (sizePerDim / 2);
    int maxSpread = getMaxSpread();

    long region = 0;
    int currentDimension = 0;

    for(Coords::iterator coordIt = coordVector.begin();
        coordIt < coordVector.end(); coordIt++) {
        // normalize to coordSize
        double coordNormalized = ((checkMaxSpread(*coordIt) /
                                  static_cast<double>(maxSpread)) *
                                  static_cast<double>(rangeShift));

        // round normalized coords
        if (coordNormalized > 0) coordNormalized += .5;
        else coordNormalized -= .5;

        // shift coords
        int coordNormAndShifted = static_cast<int>(coordNormalized) +
                                  rangeShift;

        //std::cout << coordNormalized << " " << coordNormAndShifted << std::endl;

        // TODO special case (wrong rounding?)
        if (coordNormAndShifted == pow(sizePerDim , currentDimension))
            coordNormAndShifted -= 1;

        // calc region number
        region += coordNormAndShifted * pow(sizePerDim , currentDimension);

        currentDimension++;
    }

    //test
    /*
    Coords tmpConvertBack = convertRegionToCoordVector(region, coordVector.size());
    std::cout << coordVector << " " << tmpConvertBack << std::endl;
     */

    return region;
}


std::vector<double> StrategyRegions::convertRegionToCoordVector(int region, int dimCount)
{
    int sizePerDim = getSizePerDim();
    int maxSpread = getMaxSpread();
    int rangeShift = static_cast<int> (sizePerDim / 2);

    std::vector<double> rCoordVector; // reverse Vector
    std::vector<double> CoordVector;

    int normDim = 0;
    int dimShifted = 0;
    int subRegionSize = 0;
    double tmpDimension;

    for (int curDim = dimCount; curDim >= 1; curDim--) {
        subRegionSize = pow(sizePerDim, (curDim - 1));
        dimShifted = static_cast<int>(region / subRegionSize);
        normDim = dimShifted - rangeShift;
        region -= subRegionSize * dimShifted;
        tmpDimension = (static_cast<double>(normDim) /
                        static_cast<double>(rangeShift)) *
                        static_cast<double>(maxSpread);
        rCoordVector.push_back(tmpDimension);
    }

    // reverse vector
    std::vector<double>::iterator vectorIterator;
    for(vectorIterator = rCoordVector.end();
        vectorIterator > rCoordVector.begin(); vectorIterator--) {
        CoordVector.push_back((*(vectorIterator-1)));
    }

    return CoordVector;
}


int StrategyRegions::getSizePerDim()
{
    int sizePerDim =
        globalViewBuilder->parProxy("gvbStrategyRegionsSizePerDimension");
    return sizePerDim;
}


int StrategyRegions::getMaxSpread()
{
    int maxSpread = globalViewBuilder->parProxy("gvbStrategyRegionsMaxSpread");
    return maxSpread;
}


void StrategyRegions::cleanUpCoordData(const treeNodeMap& currentTreeChildNodes)
{
    branchRegionDataMap::const_iterator regionDataMapIterator =
        regionData.begin();

    if (regionDataMapIterator == regionData.end()) return;

    do {
        if (currentTreeChildNodes.find(regionDataMapIterator->first) ==
                currentTreeChildNodes.end() &&
            regionDataMapIterator->first != thisNode) {
            regionDataMapIterator = regionData.erase(regionDataMapIterator);
            //assert(regionDataMapIterator != regionData.end());
        } else ++regionDataMapIterator;
    } while (regionDataMapIterator != regionData.end());
}
