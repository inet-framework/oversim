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

/**
 * @file AccessNet.h
 * @author Markus Mauch
 */

#ifndef __ACCESSNET_H__
#define __ACCESSNET_H__

#include "common/OverSimDefs.h"

#include <common/InitStages.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/ipv4/IPv4Route.h>

using namespace inet;

namespace inet {
class IInterfaceTable;
class InterfaceEntry;
class IRoutingTable;
class IPv6RoutingTable;
class IPv6Route;
}

/**
 * Structure to manipulate IPv6 addresses easily
 */
struct IPv6Words
{
public:
    uint32_t d0, d1, d2, d3;

    IPv6Words(L3Address addr) {
        IPv6Address ip6 = addr.toIPv6();
        const uint32_t* words = ip6.words();
        d0 = words[0];
        d1 = words[1];
        d2 = words[2];
        d3 = words[3];
    }

    IPv6Words() {
            d0 = 0;
            d1 = 0;
            d2 = 0;
            d3 = 0;
        }
};

/**
 * Information about a getNode(usually a router)
 */
class NodeInfo
{
public:
    L3Address ipvxAddress; //!< the IP Address
    cModule* module; //!< pointer to node getModule(not this module)
    IInterfaceTable* interfaceTable; //!< pointer to interface table of this node
    IIPv4RoutingTable* routingTable; //!< pointer to routing table of this node
    IPv6RoutingTable* routingTable6;
    simtime_t createdAt; //!< creation timestamp

    /**
     * Stream out
     *
     * @param os the output stream
     * @param n the node info
     * @return the stream
     */
    friend std::ostream& operator<<(std::ostream& os, NodeInfo& n);
};

/**
 * Information about a terminal
 */
class TerminalInfo : public NodeInfo
{
public:
    cModule* PPPInterface; //!< pointer to PPP module
    cModule* remotePPPInterface; //!< pointer to remote PPP module
    InterfaceEntry* interfaceEntry; //!< pointer to interface entry
    InterfaceEntry* remoteInterfaceEntry; //!< pointer to remote interface entry
    IInterfaceTable* remoteInterfaceTable; //!< pointer to remote interface table
    IPv4Route* remoteRoutingEntry; //!< pointer to remote routing table
    IPv4Route* routingEntry; //!< pointer to routing entry
    IPv6Route* remoteIpv6RoutingEntry; //!< pointer to remote routing table
    IPv6Route* ipv6routingEntry; //!< pointer to routing entry
};

/**
 * Configuration module for access networks
 */
class AccessNet : public cSimpleModule
{
public:

    /**
     * Returns number of nodes at this access router
     *
     * @return number of nodes
     */
    virtual int size()
    {
        return overlayTerminal.size();
    }

    /**
     * Getter for router module
     *
     * @return pointer to router module
     */
    virtual cModule* getAccessNode()
    {
        return router.module;
    }

    /**
     * Gathers some information about the terminal and appends it to
     * the overlay terminal vector
     *
     * Gathers some information about the terminal and appends it to
     * the overlay terminal vector.
     * (called by InetUnderlayConfigurator in stage MAX_STAGE_UNDERLAY)
     */
    virtual L3Address addOverlayNode(cModule* overlayNode, bool migrate = false);

    /**
     * returns a random ID
     */
    int getRandomNodeId();

    /**
     * Removes a node from the access net
     */
    virtual cModule* removeOverlayNode(int ID);

    /**
     * searches overlayTerminal[] for a given node
     *
     * @param ID position of the node in overlayTerminal
     * @return the nodeId if found, -1 else
     */
    virtual cModule* getOverlayNode(int ID);

    /**
     * set access types
     *
     * @param typeRx access receive type
     * @param typeTx access transmit type
     */
    void selectChannel(const std::string& typeRx, const std::string &typeTx)
    {
        channelTypeRxStr = typeRx;
        channelTypeTxStr = typeTx;
    }

protected:

    NodeInfo router; //!< this access router
    std::vector<TerminalInfo> overlayTerminal; //!< the terminals at this access router
    std::vector<L3Address> returnedIPs; //!< list of IP addresses wich are no longer in use

    L3Address getAssignedPrefix(IInterfaceTable* ift);
    /**
     * OMNeT number of init stages
     *
     * @return neede number of init stages
     */
    virtual int numInitStages() const override
    {
        return NUM_STAGES_ALL;
    }

    /**
     * Gather some information about the router node.
     */
    virtual void initialize(int stage) override;

    /**
     * OMNeT handleMessage method
     *
     * @param msg the message to handle
     */
    virtual void handleMessage(cMessage* msg) override;

    /**
     * Displays the current number of terminals connected to this access net
     */
    virtual void updateDisplayString();

    uint32_t lastIP; //!< last assigned IP address
    bool useIPv6; //!< IPv6 address usage

    std::vector<std::string> channelTypesRx; //!< vector of possible access channels (rx)
    std::string channelTypeRxStr; //!< the current active channel type (rx)
    std::vector<std::string> channelTypesTx; //!< vector of possible access channels (tx)
    std::string channelTypeTxStr; //!< the current active channel type (tx)

    // statistics
    cOutVector lifetimeVector; //!< vector of node lifetimes
};

/**
 * Returns a module's fist unconnected gate
 *
 * @param owner gate owner module
 * @param name name of the gate vector
 * @param type gate type (input or output)
 */
cGate* firstUnusedGate(cModule* owner, const char* name, cGate::Type type = cGate::NONE);



#endif
