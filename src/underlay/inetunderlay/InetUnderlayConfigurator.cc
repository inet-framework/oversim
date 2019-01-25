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
 * @file InetUnderlayConfigurator.cc
 * @author Markus Mauch, Stephan Krause, Bernhard Heep
 */

#include "InetUnderlayConfigurator.h"

#include <GlobalNodeList.h>
#include <TransportAddress.h>

#include <StringConvert.h>

#include <AccessNet.h>
#include <IRoutingTable.h>
#include <RoutingTable6.h>
#include <IInterfaceTable.h>
#include <IPAddressResolver.h>
#include <IPv4InterfaceData.h>
#include <IPv6InterfaceData.h>
#include <NotificationBoard.h>


#include <InetInfo.h>

Define_Module(InetUnderlayConfigurator);

void InetUnderlayConfigurator::initializeUnderlay(int stage)
{
    //backbone configuration
    if (stage == MIN_STAGE_UNDERLAY) {
        // Find all router modules.
        cTopology topo("topo");
        topo.extractByProperty("node");

        if (par("useIPv6Addresses").boolValue()) {
            setUpIPv6(topo);
            //opp_error("IPv6 is not supported in this release but is coming soon.");
        } else {
            setUpIPv4(topo);
        }
    }
    //access net configuration
    else if(stage == MAX_STAGE_UNDERLAY) {
        // fetch some parameters
        accessRouterNum = getParentModule()->par("accessRouterNum");
        overlayAccessRouterNum = getParentModule()->par("overlayAccessRouterNum");

        // count the overlay clients
        overlayTerminalCount = 0;

        numCreated = 0;
        numKilled = 0;

        // add access node modules to access node vector
        cModule* node;
        for (int i = 0; i < accessRouterNum; i++) {
            node = getParentModule()->getSubmodule("accessRouter", i);
            accessNode.push_back( node );
        }

        for (int i = 0; i < overlayAccessRouterNum; i++) {
            node = getParentModule()->getSubmodule("overlayAccessRouter", i);
            accessNode.push_back( node );
        }

        // debug stuff
        WATCH_PTRVECTOR(accessNode);
    }
}

TransportAddress* InetUnderlayConfigurator::createNode(NodeType type, bool initialize)
{
    Enter_Method_Silent();
    // derive overlay node from ned
    std::string nameStr = "overlayTerminal";
    if( churnGenerator.size() > 1 ){
        nameStr += "-" + convertToString<uint32_t>(type.typeID);
    }
    cModuleType* moduleType = cModuleType::get(type.terminalType.c_str());
    cModule* node = moduleType->create(nameStr.c_str(), getParentModule(),
                                       numCreated + 1, numCreated);

    if (type.channelTypesTx.size() > 0) {
        throw cRuntimeError("InetUnderlayConfigurator::createNode(): Setting "
                    "channel types via the churn generator is not allowed "
                    "with the InetUnderlay. Use **.accessNet.channelTypes instead!");
    }

    node->setGateSize("pppg", 1);

    std::string displayString;

    if ((type.typeID > 0) && (type.typeID <= NUM_COLORS)) {
        ((displayString += "i=device/wifilaptop_l,")
                        += colorNames[type.typeID - 1])
                        += ",40;i2=block/circle_s";
    } else {
        displayString = "i=device/wifilaptop_l;i2=block/circle_s";
    }

    node->finalizeParameters();
    node->setDisplayString(displayString.c_str());

    node->buildInside();
    node->scheduleStart(simTime());

    // create meta information
    InetInfo* info = new InetInfo(type.typeID, node->getId(), type.context);
    AccessNet* accessNet= check_and_cast<AccessNet*>
        (accessNode[intuniform(0, accessNode.size() - 1)]
                ->getSubmodule("accessNet"));

    info->setAccessNetModule(accessNet);
    info->setNodeID(node->getId());

    // add node to a randomly chosen access net and bootstrap oracle
    globalNodeList->addPeer(accessNet->addOverlayNode(node), info);

    // if the node was not created during startup we have to
    // finish the initialization process manually
    if (!initialize) {
        for (int i = MAX_STAGE_UNDERLAY + 1; i < NUM_STAGES_ALL; i++) {
            node->callInitialize(i);
        }
    }

    overlayTerminalCount++;
    numCreated++;

    churnGenerator[type.typeID]->terminalCount++;

    TransportAddress *address = new TransportAddress(
                                       IPAddressResolver().addressOf(node));

    // update display
    setDisplayString();

    return address;
}

//TODO: getRandomNode()
void InetUnderlayConfigurator::preKillNode(NodeType type, TransportAddress* addr)
{
    Enter_Method_Silent();

    AccessNet* accessNetModule = NULL;
    int nodeID;
    InetInfo* info;

    // If no address given, get random node
    if (addr == NULL) {
        addr = globalNodeList->getRandomAliveNode(type.typeID);

        if (addr == NULL) {
            // all nodes are already prekilled
            std::cout << "all nodes are already prekilled" << std::endl;
            return;
        }
    }

    // get node information
    info = dynamic_cast<InetInfo*>(globalNodeList->getPeerInfo(*addr));

    if (info != NULL) {
        accessNetModule = info->getAccessNetModule();
        nodeID = info->getNodeID();
    } else {
        opp_error("IPv4UnderlayConfigurator: Trying to pre kill node "
                  "with nonexistant TransportAddress!");
    }

    uint32_t effectiveType = info->getTypeID();

    // do not kill node that is already scheduled
    if(scheduledID.count(nodeID))
        return;

    cModule* node = accessNetModule->getOverlayNode(nodeID);
    globalNodeList->removePeer(IPAddressResolver().addressOf(node));

    //put node into the kill list and schedule a message for final removal of the node
    killList.push_front(IPAddressResolver().addressOf(node));
    scheduledID.insert(nodeID);

    overlayTerminalCount--;
    numKilled++;

    churnGenerator[effectiveType]->terminalCount--;

    // update display
    setDisplayString();

    // inform the notification board about the removal
    NotificationBoard* nb = check_and_cast<NotificationBoard*>(
            node->getSubmodule("notificationBoard"));
    nb->fireChangeNotification(NF_OVERLAY_NODE_LEAVE);

    double random = uniform(0, 1);

    if (random < gracefulLeaveProbability) {
        nb->fireChangeNotification(NF_OVERLAY_NODE_GRACEFUL_LEAVE);
    }

    cMessage* msg = new cMessage();
    scheduleAt(simTime() + gracefulLeaveDelay, msg);

}

void InetUnderlayConfigurator::migrateNode(NodeType type, TransportAddress* addr)
{
    Enter_Method_Silent();

    AccessNet* accessNetModule = NULL;
    int nodeID = -1;
    InetInfo* info;

    // If no address given, get random node
    if(addr == NULL) {
        info = dynamic_cast<InetInfo*>(globalNodeList->getRandomPeerInfo(type.typeID));
    } else {
        // get node information
        info = dynamic_cast<InetInfo*>(globalNodeList->getPeerInfo(*addr));
    }

    if(info != NULL) {
        accessNetModule = info->getAccessNetModule();
        nodeID = info->getNodeID();
    } else {
        opp_error("IPv4UnderlayConfigurator: Trying to pre kill node with nonexistant TransportAddress!");
    }

    // do not migrate node that is already scheduled
    if(scheduledID.count(nodeID))
        return;

    cModule* node = accessNetModule->removeOverlayNode(nodeID);//intuniform(0, accessNetModule->size() - 1));

    if(node == NULL)
        opp_error("IPv4UnderlayConfigurator: Trying to remove node which is nonexistant in AccessNet!");

    //remove node from bootstrap oracle
    globalNodeList->killPeer(IPAddressResolver().addressOf(node));

    node->bubble("I am migrating!");

    // connect the node to another access net
    AccessNet* newAccessNetModule;

    do {
        newAccessNetModule = check_and_cast<AccessNet*>(accessNode[intuniform(0, accessNode.size() - 1)]->getSubmodule("accessNet"));
    } while((newAccessNetModule == accessNetModule) && (accessNode.size() != 1));

    // create meta information
    InetInfo* newinfo = new InetInfo(type.typeID, node->getId(), type.context);

    newinfo->setAccessNetModule(newAccessNetModule);
    newinfo->setNodeID(node->getId());

    //add node to a randomly chosen access net bootstrap oracle
    globalNodeList->addPeer(newAccessNetModule->addOverlayNode(node, true), newinfo);

    // inform the notification board about the migration
    NotificationBoard* nb = check_and_cast<NotificationBoard*>(node->getSubmodule("notificationBoard"));
    nb->fireChangeNotification(NF_OVERLAY_TRANSPORTADDRESS_CHANGED);
}

void InetUnderlayConfigurator::handleTimerEvent(cMessage* msg)
{
    Enter_Method_Silent();

    // get next scheduled node from the kill list
    IPvXAddress addr = killList.back();
    killList.pop_back();

    AccessNet* accessNetModule = NULL;
    int nodeID = -1;

    InetInfo* info = dynamic_cast<InetInfo*>(globalNodeList->getPeerInfo(addr));
    if(info != NULL) {
        accessNetModule = info->getAccessNetModule();
        nodeID = info->getNodeID();
    } else {
        opp_error("IPv4UnderlayConfigurator: Trying to kill node with nonexistant TransportAddress!");
    }

    scheduledID.erase(nodeID);
    globalNodeList->killPeer(addr);

    cModule* node = accessNetModule->removeOverlayNode(nodeID);

    if(node == NULL)
        opp_error("IPv4UnderlayConfigurator: Trying to remove node which is nonexistant in AccessNet!");

    node->callFinish();
    node->deleteModule();

    delete msg;
}

void InetUnderlayConfigurator::setDisplayString()
{
    char buf[80];
    sprintf(buf, "%i overlay terminals\n%i access router\n%i overlay access router",
            overlayTerminalCount, accessRouterNum, overlayAccessRouterNum);
    getDisplayString().setTagArg("t", 0, buf);
}

void InetUnderlayConfigurator::finishUnderlay()
{
    // statistics
    recordScalar("Terminals added", numCreated);
    recordScalar("Terminals removed", numKilled);

    if (!isInInitPhase()) {
        struct timeval now, diff;
        gettimeofday(&now, NULL);
        timersub(&now, &initFinishedTime, &diff);
        printf("Simulation time: %li.%06li\n", diff.tv_sec, diff.tv_usec);
    }
}

void InetUnderlayConfigurator::setUpIPv4(cTopology &topo)
{
    // Assign IP addresses to all router modules.
    std::vector<uint32> nodeAddresses;
    nodeAddresses.resize(topo.getNumNodes());

    // IP addresses for backbone
    // Take start IP from config file
    // FIXME: Make Netmask for Routers configurable!
    uint32 lowIPBoundary = IPAddress(par("startIPv4").stringValue()).getInt();

    // uint32 lowIPBoundary = uint32((1 << 24) + 1);
    int numIPNodes = 0;

    for (int i = 0; i < topo.getNumNodes(); i++) {
        ++numIPNodes;

        uint32 addr = lowIPBoundary + uint32(numIPNodes << 16);
        nodeAddresses[i] = addr;

        // update ip display string
        if (ev.isGUI()) {
            topo.getNode(i)->getModule()->getDisplayString().insertTag("t", 0);
            topo.getNode(i)->getModule()->getDisplayString().setTagArg("t", 0,
                                    const_cast<char*>(IPAddress(addr).str().c_str()));
            topo.getNode(i)->getModule()->getDisplayString().setTagArg("t", 1, "l");
            topo.getNode(i)->getModule()->getDisplayString().setTagArg("t", 2, "red");
        }

        // find interface table and assign address to all (non-loopback) interfaces
        IInterfaceTable* ift = IPAddressResolver().interfaceTableOf(topo.getNode(i)->getModule());

        for ( int k = 0; k < ift->getNumInterfaces(); k++ ) {
            InterfaceEntry* ie = ift->getInterface(k);
            if (!ie->isLoopback()) {
                ie->ipv4Data()->setIPAddress(IPAddress(addr));
                // full address must match for local delivery
                ie->ipv4Data()->setNetmask(IPAddress::ALLONES_ADDRESS);
            }
        }
    }

    // Fill in routing tables.
    for (int i = 0; i < topo.getNumNodes(); i++) {
        cTopology::Node* destNode = topo.getNode(i);
        uint32 destAddr = nodeAddresses[i];

        // calculate shortest paths from everywhere towards destNode
        topo.calculateUnweightedSingleShortestPathsTo(destNode);

        // add overlayAccessRouters and overlayBackboneRouters
        // to the GlobalNodeList
        if ((strcmp(destNode->getModule()->getName(), "overlayBackboneRouter") == 0) ||
                (strcmp(destNode->getModule()->getName(), "overlayAccessRouter") == 0)) {
            //add node to bootstrap oracle
            PeerInfo* info = new PeerInfo(0, destNode->getModule()->getId(), NULL);
            globalNodeList->addPeer(IPvXAddress(nodeAddresses[i]), info);
        }


        // If destNode is the outRouter, add a default route
        // to outside network via the TunOutDevice and a route to the
        // Gateway
        if ( strcmp(destNode->getModule()->getName(), "outRouter" ) == 0 ) {
            IPRoute* defRoute = new IPRoute();
            defRoute->setHost(IPAddress::UNSPECIFIED_ADDRESS);
            defRoute->setNetmask(IPAddress::UNSPECIFIED_ADDRESS);
            defRoute->setGateway(IPAddress(par("gatewayIP").stringValue()));
            defRoute->setInterface(IPAddressResolver().interfaceTableOf(destNode->getModule())->getInterfaceByName("tunDev"));
            defRoute->setType(IPRoute::REMOTE);
            defRoute->setSource(IPRoute::MANUAL);
            IPAddressResolver().routingTableOf(destNode->getModule())->addRoute(defRoute);

            IPRoute* gwRoute = new IPRoute();
            gwRoute->setHost(IPAddress(par("gatewayIP").stringValue()));
            gwRoute->setNetmask(IPAddress(255, 255, 255, 255));
            gwRoute->setInterface(IPAddressResolver().interfaceTableOf(destNode->getModule())->getInterfaceByName("tunDev"));
            gwRoute->setType(IPRoute::DIRECT);
            gwRoute->setSource(IPRoute::MANUAL);
            IPAddressResolver().routingTableOf(destNode->getModule())->addRoute(gwRoute);
        }

        // add route (with host=destNode) to every routing table in the network
        for (int j = 0; j < topo.getNumNodes(); j++) {
            // continue if same node
            if (i == j)
                continue;

            // cancel simulation if node is not connected with destination
            cTopology::Node* atNode = topo.getNode(j);

            if (atNode->getNumPaths() == 0) {
                error((std::string(atNode->getModule()->getName()) + ": Network is not entirely connected."
                        "Please increase your value for the "
                        "connectivity parameter").c_str());
            }

            //
            // Add routes at the atNode.
            //

            // find atNode's interface and routing table
            IInterfaceTable* ift = IPAddressResolver().interfaceTableOf(atNode->getModule());
            IRoutingTable* rt = IPAddressResolver().routingTableOf(atNode->getModule());

            // find atNode's interface entry for the next hop node
            int outputGateId = atNode->getPath(0)->getLocalGate()->getId();
            InterfaceEntry *ie = ift->getInterfaceByNodeOutputGateId(outputGateId);

            // find the next hop node on the path towards destNode
            cModule* next_hop = atNode->getPath(0)->getRemoteNode()->getModule();
            IPAddress next_hop_ip = IPAddressResolver().addressOf(next_hop).get4();


            // Requirement 1: Each router has exactly one routing entry
            // (netmask 255.255.0.0) to each other router
            IPRoute* re = new IPRoute();

            re->setHost(IPAddress(destAddr));
            re->setInterface(ie);
            re->setSource(IPRoute::MANUAL);
            re->setNetmask(IPAddress(255, 255, 0, 0));
            re->setGateway(IPAddress(next_hop_ip));
            re->setType(IPRoute::REMOTE);

            rt->addRoute(re);

            // Requirement 2: Each router has a point-to-point routing
            // entry (netmask 255.255.255.255) for each immediate neighbour
            if (atNode->getDistanceToTarget() == 1) {
                IPRoute* re2 = new IPRoute();

                re2->setHost(IPAddress(destAddr));
                re2->setInterface(ie);
                re2->setSource(IPRoute::MANUAL);
                re2->setNetmask(IPAddress(255, 255, 255, 255));
                re2->setType(IPRoute::DIRECT);

                rt->addRoute(re2);
            }

            // If destNode is the outRouter, add a default route
            // to the next hop in the direction of the outRouter
            if (strcmp(destNode->getModule()->getName(), "outRouter" ) == 0) {
                IPRoute* defRoute = new IPRoute();
                defRoute->setHost(IPAddress::UNSPECIFIED_ADDRESS);
                defRoute->setNetmask(IPAddress::UNSPECIFIED_ADDRESS);
                defRoute->setGateway(IPAddress(next_hop_ip));
                defRoute->setInterface(ie);
                defRoute->setType(IPRoute::REMOTE);
                defRoute->setSource(IPRoute::MANUAL);

                rt->addRoute(defRoute);
            }
        }
    }
}

void InetUnderlayConfigurator::setUpIPv6(cTopology &topo)
{
    // Assign IP addresses to all router modules.
    std::vector<IPv6Words> nodeAddresses;
    nodeAddresses.resize(topo.getNumNodes());

    // IP addresses for backbone
    // Take start IP from config file
    // FIXME: Make Netmask for Routers configurable!
    IPv6Words lowIPBoundary(IPv6Address(par("startIPv6").stringValue()));

    // uint32 lowIPBoundary = uint32((1 << 24) + 1);
    int numIPNodes = 0;

    for (int i = 0; i < topo.getNumNodes(); i++) {
        ++numIPNodes;

        IPv6Words addr = lowIPBoundary;
        addr.d0 += numIPNodes;
        nodeAddresses[i] = addr;

        // update ip display string
        if (ev.isGUI()) {
            topo.getNode(i)->getModule()->getDisplayString().insertTag("t", 0);
            topo.getNode(i)->getModule()->getDisplayString().setTagArg("t", 0,
                                    const_cast<char*>(IPv6Address(addr.d0, addr.d1, addr.d2, addr.d3).str().c_str()));
            topo.getNode(i)->getModule()->getDisplayString().setTagArg("t", 1, "l");
            topo.getNode(i)->getModule()->getDisplayString().setTagArg("t", 2, "red");
        }

        // find interface table and assign address to all (non-loopback) interfaces
        IInterfaceTable* ift = IPAddressResolver().interfaceTableOf(topo.getNode(i)->getModule());

        for ( int k = 0; k < ift->getNumInterfaces(); k++ ) {
            InterfaceEntry* ie = ift->getInterface(k);
            if (!ie->isLoopback() && ie->ipv6Data()) {
                //ie->ipv6Data()->assignAddress(IPv6Address(addr.d0, addr.d1, addr.d2, addr.d3), false, 0, 0);
                // full address must match for local delivery
                IPv6Address prefix(addr.d0, addr.d1, addr.d2, addr.d3);
                IPv6InterfaceData::AdvPrefix p;
                p.prefix = prefix;
                p.prefixLength = 32;
                p.advAutonomousFlag = true;
                p.advPreferredLifetime = 0;
                p.advValidLifetime = 0;
                p.advOnLinkFlag = true;
                ie->ipv6Data()->addAdvPrefix(p);
                ie->setMACAddress(MACAddress::generateAutoAddress());

                ie->ipv6Data()->assignAddress(prefix,false, 0, 0);

                if (ie->ipv6Data()->getLinkLocalAddress().isUnspecified()) {
                    ie->ipv6Data()->assignAddress(IPv6Address::formLinkLocalAddress(ie->getInterfaceToken()),false, 0, 0);
                }
            }
        }
    }

    // Fill in routing tables.
    for (int i = 0; i < topo.getNumNodes(); i++) {
        cTopology::Node* destNode = topo.getNode(i);

        // calculate shortest paths from everywhere towards destNode
        topo.calculateUnweightedSingleShortestPathsTo(destNode);

        // add overlayAccessRouters and overlayBackboneRouters
        // to the GlobalNodeList
        if ((strcmp(destNode->getModule()->getName(), "overlayBackboneRouter") == 0) ||
                (strcmp(destNode->getModule()->getName(), "overlayAccessRouter") == 0)) {
            //add node to bootstrap oracle
            PeerInfo* info = new PeerInfo(0, destNode->getModule()->getId(), NULL);
            globalNodeList->addPeer(IPvXAddress(IPv6Address(nodeAddresses[i].d0, nodeAddresses[i].d1, nodeAddresses[i].d2, nodeAddresses[i].d3)), info);
        }

        // add route (with host=destNode) to every routing table in the network
        for (int j = 0; j < topo.getNumNodes(); j++) {
            // continue if same node
            if (i == j)
                continue;

            // cancel simulation if node is not connected with destination
            cTopology::Node* atNode = topo.getNode(j);

            if (atNode->getNumPaths() == 0) {
                error((std::string(atNode->getModule()->getName()) + ": Network is not entirely connected."
                        "Please increase your value for the "
                        "connectivity parameter").c_str());
            }

            //
            // Add routes at the atNode.
            //

            // find atNode's interface and routing table
            IInterfaceTable* ift = IPAddressResolver().interfaceTableOf(atNode->getModule());
            RoutingTable6* rt = IPAddressResolver().routingTable6Of(atNode->getModule());

            // find atNode's interface entry for the next hop node
            int outputGateId = atNode->getPath(0)->getLocalGate()->getId();
            InterfaceEntry *ie = ift->getInterfaceByNodeOutputGateId(outputGateId);

            // find the next hop node on the path towards destNode
            cModule* next_hop = atNode->getPath(0)->getRemoteNode()->getModule();
            int destGateId = destNode->getLinkIn(0)->getLocalGateId();
            IInterfaceTable* destIft = IPAddressResolver().interfaceTableOf(destNode->getModule());
            int remoteGateId = atNode->getPath(0)->getRemoteGateId();
            IInterfaceTable* remoteIft = IPAddressResolver().interfaceTableOf(next_hop);
            IPv6Address next_hop_ip = remoteIft->getInterfaceByNodeInputGateId(remoteGateId)->ipv6Data()->getLinkLocalAddress();
            IPv6InterfaceData::AdvPrefix destPrefix = destIft->getInterfaceByNodeInputGateId(destGateId)->ipv6Data()->getAdvPrefix(0);

            // create routing entry for next hop
            rt->addStaticRoute(destPrefix.prefix, destPrefix.prefixLength, ie->getInterfaceId(), next_hop_ip);

        }
    }
}


/**
 * Extended uniform() function
 *
 * @param start start value
 * @param end end value
 * @param index position of the new value in the static vector
 * @param new_calc '1' if a new random number should be generated
 * @return the random number at position index in the double vector
 */
double uniform2(double start, double end, double index, double new_calc)
{
    static std::vector<double> value;
    if ( (unsigned int)index >= value.size() )
        value.resize((int)index + 1);
    if ( new_calc == 1 )
        value[(int)index] = uniform(start, end);
    return value[(int)index];
};

/**
 * Extended intuniform() function
 *
 * @param start start value
 * @param end end value
 * @param index position of the new value in the static vector
 * @param new_calc '1' if a new random number should be generated
 * @return the random number at position index in the double vector
 */
double intuniform2(double start, double end, double index, double new_calc)
{
    static std::vector<double> value;
    if ( (unsigned int)index >= value.size() )
        value.resize((int)index + 1);
    if ( new_calc == 1 )
        value[(int)index] = (double)intuniform((int)start, (int)end);
    return value[(int)index];
};

Define_NED_Math_Function(uniform2, 4);
Define_NED_Math_Function(intuniform2, 4);
