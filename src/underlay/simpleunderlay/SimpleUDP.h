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

/**
 * @file SimpleUDP.h
 * @author Jochen Reber
 */

//
// Author: Jochen Reber
// Rewrite: Andras Varga 2004,2005
// Modifications: Stephan Krause, Bernhard Mueller
//

#ifndef __SIMPLEUDP_H__
#define __SIMPLEUDP_H__

#include <map>
#include <list>

#include "common/InitStages.h"
#include "inet/transportlayer/udp/UDP.h"

#include "inet/transportlayer/contract/udp/UDPControlInfo_m.h"

using namespace inet;

class GlobalNodeList;
class SimpleNodeEntry;
class GlobalStatistics;

namespace inet {
class IPv4ControlInfo;
class IPv6ControlInfo;
class ICMP;
class ICMPv6;
class UDPPacket;
}

/**
 * Implements the UDP protocol: encapsulates/decapsulates user data into/from UDP.
 *
 * More info in the NED file.
 */
class SimpleUDP : public UDP
{
public:

    // delay fault type string and corresponding map for switch..case
    static std::string delayFaultTypeString;
    enum delayFaultTypeNum {
        delayFaultUndefined,
        delayFaultLiveAll,
        delayFaultLivePlanetlab,
        delayFaultSimulation
    };
    static std::map<std::string, delayFaultTypeNum> delayFaultTypeMap;

protected:

    // statistics
    int numQueueLost; /**< number of lost packets due to queue full */
    int numPartitionLost; /**< number of lost packets due to network partitions */
    int numDestUnavailableLost; /**< number of lost packets due to unavailable destination */
    simtime_t delay; /**< simulated delay between sending and receiving udp module */

    simtime_t constantDelay; /**< constant delay between two peers */
    bool useCoordinateBasedDelay; /**< delay should be calculated from euklidean distance between two peers */
    double jitter; /**< amount of jitter in % of total delay */
    bool enableAccessRouterTxQueue; /* model the tx queue of the access router of an destination node */
    bool faultyDelay; /** violate the triangle inequality?*/
    GlobalNodeList* globalNodeList; /**< pointer to GlobalNodeList */
    GlobalStatistics* globalStatistics; /**< pointer to GlobalStatistics */
    SimpleNodeEntry* nodeEntry; /**< nodeEntry of the overlay node this module belongs to */

public:
    /**
     * set or change the nodeEntry of this module
     *
     * @param entry the new nodeEntry
     */
    void setNodeEntry(SimpleNodeEntry* entry);

protected:
    /**
     * utility: show current statistics above the icon
     */
    void updateDisplayString();

    /**
     * process packets from application
     *
     * @param appData the data that has to be sent
     */
    virtual void processMsgFromApp(cPacket *appData);

    // process UDP packets coming from IP
    virtual void processUDPPacket(cPacket *udpPacket);

    virtual void processUndeliverablePacket(cPacket *udpPacket, cObject *ctrl);
    virtual void sendUp(cPacket *payload, UDPControlInfo *ctrl, SockDesc *sd);

public:
    SimpleUDP();

    /** destructor */
    virtual ~SimpleUDP();

protected:
    /**
     * initialise the SimpleUDP module
     *
     * @param stage stage of initialisation phase
     */
    virtual void initialize(int stage) override;

    virtual void handleMessage(cMessage *msg) override;

    /**
     * returns the number of init stages
     *
     * @return the number of init stages
     */
    virtual int numInitStages() const override
    {
        return NUM_STAGES_ALL;
    }

    void finish() override;
};

#endif

