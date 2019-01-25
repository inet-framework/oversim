//
// Copyright (C) 2008 Institut fuer Telematik, Universitaet Karlsruhe (TH)
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
 * @file CoordBasedRouting.cc
 * @author Fabian Hartmann
 * @author Bernhard Heep
 */


#include <fstream>
#include <string>
#include <cassert>
#include <algorithm>

#include <omnetpp.h>

#include <GlobalNodeListAccess.h>
#include <PeerInfo.h>

#include "CoordBasedRouting.h"


Define_Module(CoordBasedRouting);

const std::string CoordBasedRouting::NOPREFIX = "NOPREFIX";

void CoordBasedRouting::initialize()
{
    areaCoordinateSource = par("areaCoordinateSource");
    cbrStartAtDigit = par("CBRstartAtDigit");
    cbrStopAtDigit = par("CBRstopAtDigit");
    cbrChangeIdLater = par("CBRchangeIdLater");
    cbrChangeIdStart = par("CBRchangeIdStart");
    cbrChangeIdStop = par("CBRchangeIdStop");
    globalNodeList = GlobalNodeListAccess().get();

    gap = new AP;

    // XML file found?
    if (std::string(areaCoordinateSource) == "") {
        EV << "[CoordBasedRouting::initialize()]\n    No CBR area file found."
           << " Using dCBR." << endl;
        return;
    }

    std::ifstream check_for_xml_file(areaCoordinateSource);
    if (!check_for_xml_file) {
        check_for_xml_file.close();
        //TODO
        throw cRuntimeError("CBR area file not found!");
        return;
    }
    else {
        EV << "[CoordBasedRouting::initialize()]\n    CBR area file '"
           << areaCoordinateSource << "' loaded." << endl;
        check_for_xml_file.close();
    }

    // XML file found, let's parse it
    parseSource(areaCoordinateSource);
}


void CoordBasedRouting::finish()
{
    gap->clear();
}


void CoordBasedRouting::parseSource(const char* areaCoordinateSource)
{
    cXMLElement* rootElement = ev.getXMLDocument(areaCoordinateSource);

    xmlDimensions = atoi(rootElement->getAttribute("dimensions"));

    for (cXMLElement *area = rootElement->getFirstChildWithTag("area"); area;
         area = area->getNextSiblingWithTag("area") ) {
        CBRArea tmpArea(xmlDimensions);
        for (cXMLElement *areavals = area->getFirstChild(); areavals;
             areavals = areavals->getNextSibling() ) {
            std::string tagname = std::string(areavals->getTagName());
            if (tagname == "min") {
                uint8_t currentdim = atoi(areavals->getAttribute("dimension"));
                double value = atof(areavals->getNodeValue());
                tmpArea.min[currentdim] = value;
            }

            else if (tagname == "max") {
                uint8_t currentdim = atoi(areavals->getAttribute("dimension"));
                double value = atof(areavals->getNodeValue());
                tmpArea.max[currentdim] = value;
            }

            else if (tagname == "prefix") {
                tmpArea.prefix = areavals->getNodeValue();
            }
        }
        gap->push_back(tmpArea);
    }

    EV << "[CoordBasedRouting::parseSource()]" << endl;
    EV << "    " << gap->size() << " prefix areas detected." << endl;
}


void CoordBasedRouting::splitNodes(CD& nodes,
                                   const std::string& prefix,
                                   const Coords& bottoms,
                                   const Coords& tops,
                                   uint8_t depth,
                                   AP* cap)
{
    // check
    if (nodes.size() < 2 || prefix.length() >= maxPrefix) {
        //std::cout << "nodes: " << nodes.size() << ", prefix.length(): " << prefix.length() << std::endl;
        CBRArea temp(bottoms, tops, prefix);
        //std::cout << temp << std::endl;
        cap->push_back(temp);
        return;
    }

    // sort
    uint8_t splitDim = depth % ccdDim;
    sort(nodes.begin(), nodes.end(), leqDim(splitDim));

    // new vectors for two halfs
    uint32_t newSize = nodes.size() / 2;
    CD lnodes(newSize), rnodes(nodes.size() - newSize);

    // split
    CD::iterator halfIt = nodes.begin();
    for (uint32_t i = 0; i < newSize; ++i, ++halfIt);
    assert(halfIt != nodes.end());
    double splitCoord = (nodes[newSize - 1][splitDim] + nodes[newSize][splitDim]) / 2;

    std::copy(nodes.begin(), halfIt, lnodes.begin());
    std::copy(halfIt, nodes.end(), rnodes.begin());

    // set area borders
    Coords newBottoms, newTops;
    newBottoms = bottoms;
    newTops = tops;
    newBottoms[splitDim] = newTops[splitDim] = splitCoord;

    // recursive calls
    splitNodes(lnodes, prefix + '0', bottoms, newTops, ++depth, cap);
    splitNodes(rnodes, prefix + '1', newBottoms, tops, depth, cap);


    /*

         def splitByNodes(returnval)
            #returnval = 0: return lower half node set
            #returnval = 1: return upper half node set
            #returnval = 2: return split coordinate
            num = @nodes.length
            half = (num / 2).floor
            splitcoord = (@nodes[half-1][@thisdim] + @nodes[half][@thisdim]) / 2
            case returnval
                when 0
                    return @nodes[0..half-1]
                when 1
                    return @nodes[half..num-1]
                when 2
                    return splitcoord
            end
        end
        */
}


const AP* CoordBasedRouting::calculateCapFromCcd(const CD& ccd, uint8_t bpd)
{
    if (ccd.size() == 0) return NULL;

    AP* cap = new AP();
    maxPrefix = bpd * cbrStopAtDigit; //TODO

    ccdDim = ccd[0].size();

    Coords bottoms(ccdDim, -1500), tops(ccdDim, 1500);
    std::string prefix;

    CD nodes = ccd;
    splitNodes(nodes, prefix, bottoms, tops, 0, cap);

    return cap;


    /*

    def split()
        # split unless no more nodes left or maximum prefix length is reached
        unless @nodes == nil || @depth >= MAXPREFIX
            lnodes = splitByNodes(0)
            unodes = splitByNodes(1)
            splitcoord = splitByNodes(2);

            lnodes = nil    if lnodes.length <= MINNODES
            unodes = nil    if unodes.length <= MINNODES

            newbottoms = []
            @bottoms.each do |bottom|
                newbottoms << bottom
            end
            newbottoms[@thisdim] = splitcoord

            newtops = []
            @tops.each do |top|
                newtops << top
            end
            newtops[@thisdim] = splitcoord

            addChild(lnodes, @prefix+"0", @bottoms, newtops);
            addChild(unodes, @prefix+"1", newbottoms, @tops);

            @children.each do |child|
                child.split
            end
        end
    end
 */
}


OverlayKey CoordBasedRouting::getNodeId(const Coords& coords,
                                        uint8_t bpd, uint8_t length,
                                        const AP* cap) const
{
    std::string prefix = getPrefix(coords, cap);

    // if no prefix is returned, something is seriously wrong with the Area Source XML
    if (prefix == NOPREFIX) {
        std::stringstream ss;
        ss << "[CoordBasedRouting::getNodeId()]: No prefix for given coords (";
        for (uint8_t i = 0; i < coords.size(); ++i) {
            ss << coords[i];
            if (i != (coords.size() - 1)) {
                ss << ", ";
            }
        }
        ss << ") found. Check your area source file!";

        EV << ss.str() << endl;
        //std::cout << ss.str() << std::endl;
        return OverlayKey::random();
    }
    std::string idString;

    // ID string:
    //                          |- endPos
    // 00000000000000011010101010000000000000
    // |_startLength_||_prefix_||_endLength_|
    // |__  .. beforeEnd ..  __|
    // |___        .... length ....      ___|
    //
    // startLength and endLength bits are set to 0 at first, then
    // randomized
    // Prefix will be cut off if stop digit is exceeded

    uint8_t startLength = (bpd * cbrStartAtDigit < length) ?
                          (bpd * cbrStartAtDigit) : length;
    uint8_t beforeEnd = (startLength + prefix.length() < length) ?
                        (startLength + prefix.length()) : length;
    uint8_t endPos = (bpd * cbrStopAtDigit < beforeEnd) ?
                     (bpd * cbrStopAtDigit) : beforeEnd;
    uint8_t endLength = length - endPos;

    // Fill startLength bits with zeros
    for (uint8_t i = 0; i < startLength; i++)
        idString += "0";

    // Now add prefix and cut it off if stop digit and/or key length is exceeded
    idString += prefix;
    if (endPos < idString.length())
        idString.erase(endPos);
    if (length < idString.length())
        idString.erase(length);

    // fill endLength bits with zeros, thus key length is reached
    for (uint8_t i = 0; i < endLength; i++)
        idString += "0";

    OverlayKey nodeId(idString, 2);

    // randomize non-prefix (zero filled) parts
    if (startLength > 0)
        nodeId = nodeId.randomPrefix(length - startLength);
    if (endLength > 0)
        nodeId = nodeId.randomSuffix(endLength);

    EV << "[CoordBasedRouting::getNodeId()]\n"
       <<"    calculated id: " << nodeId << endl;
    return nodeId;
}

std::string CoordBasedRouting::getPrefix(const Coords& coords,
                                         const AP* cap) const
{
    //for (uint8_t i = 0; i < coords.size(); ++i) {
    //    std::cout << coords[i] << ", ";
    //}
    //std::cout << std::endl;

    bool areaFound = false;
    uint32_t iter = 0;

    // TODO dimension in dCBR
    /*
    // Return no prefix if coords dimensions don't match area file dimensions
    if (!checkDimensions(coords.size())) {
        //std::cout << "dim" << std::endl;
        return NOPREFIX;
    }
    */

    const AP* ap = ((cap != NULL) ? cap : gap);
    //assert(ap && (ap->size() > 0));

    /*
    for (uint i = 0; i < ap->size(); ++i) {
        for (uint j = 0; i < ap->at(i).min.size(); ++j) {
            std::cout << ap->at(i).min[j] << " - " << ap->at(i).max[j] << std::endl;
        }
    }
    */

    while (!areaFound && iter < ap->size()) {
        //std::cout << (*ap)[iter].min.size() << std::endl;
        CBRArea thisArea = (*ap)[iter];

        // assume we're in the correct area unless any dimension tells us otherwise
        areaFound = true;
        for (uint8_t thisdim = 0; thisdim < coords.size(); thisdim++) {
            if (coords[thisdim] < thisArea.min[thisdim] ||
                coords[thisdim] > thisArea.max[thisdim]) {
                areaFound = false;
                break;
            }
        }

        // no borders are broken in any dimension -> we're in the correct area,
        // return corresponding prefix
        if (areaFound) {
            EV << "[CoordBasedRouting::getPrefix()]\n"
               <<"    calculated prefix: " << thisArea.prefix << endl;
            //std::cout << "[CoordBasedRouting::getPrefix()]\n"
            //           <<"    calculated prefix: " << thisArea.prefix << std::endl;
            return thisArea.prefix;
        }
        iter++;
    }

    // no corresponding prefix found, XML file broken?
    EV << "[CoordBasedRouting::getPrefix()]\n"
       << "    No corresponding prefix found, check your area source file!"
       << endl;

    return NOPREFIX;
}

double CoordBasedRouting::getEuclidianDistanceByKeyAndCoords(const OverlayKey& destKey,
                                                             const Coords& coords,
                                                             uint8_t bpd,
                                                             const AP* cap) const
{
    assert(!destKey.isUnspecified());
    uint32_t iter = 0;
    const AP* ap = ((cap != NULL) ? cap : gap);
    assert(ap);

    while (iter < ap->size()) {
        CBRArea thisArea = (*ap)[iter];

        // Take CBR Start/Stop Digit into account
        uint8_t startbit = bpd * cbrStartAtDigit;
        uint8_t length = (bpd * cbrStopAtDigit - bpd * cbrStartAtDigit <
                (uint8_t)thisArea.prefix.length() - bpd * cbrStartAtDigit)
                ? (bpd * cbrStopAtDigit - bpd * cbrStartAtDigit)
                : (thisArea.prefix.length() - bpd * cbrStartAtDigit);
        if (destKey.toString(2).substr(startbit, length) ==
            thisArea.prefix.substr(startbit, length)) {
            // Get euclidian distance of area center to given coords
            Coords areaCenterCoords;
            areaCenterCoords.resize(getXmlDimensions());
            double sumofsquares = 0;
            for (uint8_t dim = 0; dim < getXmlDimensions(); dim++) {
                areaCenterCoords[dim] =
                    (thisArea.min[dim] + thisArea.max[dim]) / 2;
                sumofsquares += pow((coords[dim] - areaCenterCoords[dim]), 2);
            }
            return sqrt(sumofsquares);
        }
        iter++;
    }
    // while loop finished -> no area with needed prefix found
    // (this shouldn't happen!)
    throw cRuntimeError("[CoordBasedRouting::"
                        "getEuclidianDistanceByKeyAndCoords]: "
                        "No prefix for search key found!");
    return -1;
}


bool CoordBasedRouting::checkDimensions(uint8_t dims) const
{
    if (dims == xmlDimensions) {
        return true;
    } else {
        EV << "[CoordBasedRouting::checkDimensions()]" << endl;
        EV << "    ERROR: Given coordinate dimensions do not match dimensions "
              "in the used area source file. Mapping results will be wrong."
           << endl;
        return false;
    }
}


/**
 * CBRArea Constructor, reserves space for min & max vectors
 */
CBRArea::CBRArea(uint8_t dim)
: min(dim, 0.0), max(dim, 0.0)
{
    //min.reserve(dim);
    //max.reserve(dim);
}

std::ostream& operator<<(std::ostream& os, const CBRArea& area)
{
    for (uint i = 0; i < area.min.size(); ++i) {
        os << "|" << area.min[i] << " - " << area.max[i] << "|";
    }
    os << " -> \"" << area.prefix << "\"";

    return os;
}
