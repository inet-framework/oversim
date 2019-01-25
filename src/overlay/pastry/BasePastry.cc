//
// Copyright (C) 2012 Institute of Telematics, Karlsruhe Institute of Technology (KIT)
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
 * @file BasePastry.cc
 * @author Felix Palmen, Gerhard Petruschat, Bernhard Heep
 */

#include <sstream>
#include <stdint.h>
#include <assert.h>

#include <IPAddressResolver.h>
#include <IPvXAddress.h>
#include <IInterfaceTable.h>
#include <IPv4InterfaceData.h>
#include <RpcMacros.h>
#include <InitStages.h>
#include <NeighborCache.h>
#include <GlobalStatistics.h>
#include <BootstrapList.h>

#include "BasePastry.h"


void BasePastry::purgeVectors(void)
{
    // purge Queue for processing multiple STATE messages:
    while (! stateCacheQueue.empty()) {
        delete stateCacheQueue.front().msg;
        delete stateCacheQueue.front().prox;
        stateCacheQueue.pop();
    }

    // delete cached state message:
    delete stateCache.msg;
    stateCache.msg = NULL;
    delete stateCache.prox;
    stateCache.prox = NULL;
}

void BasePastry::baseInit()
{
    bitsPerDigit = par("bitsPerDigit");
    numberOfLeaves = par("numberOfLeaves");
    numberOfNeighbors = par("numberOfNeighbors");
    joinTimeoutAmount = par("joinTimeout");
    repairTimeout = par("repairTimeout");
    enableNewLeafs = par("enableNewLeafs");
    optimizeLookup = par("optimizeLookup");
    useRegularNextHop = par("useRegularNextHop");
    alwaysSendUpdate = par("alwaysSendUpdate");
    proximityNeighborSelection = par("proximityNeighborSelection");

    if (!neighborCache->isEnabled()) {
        throw cRuntimeError("NeighborCache is disabled, which is mandatory "
                                "for Pastry/Bamboo. Activate it by setting "
                                "\"**.neighborCache.enableNeighborCache "
                                "= true\" in your omnetpp.ini!");
    }

    if (numberOfLeaves % 2) {
        EV << "[BasePastry::baseInit() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Warning: numberOfLeaves must be even - adding 1."
           << endl;
        numberOfLeaves++;
    }

    routingTable = check_and_cast<PastryRoutingTable*>
        (getParentModule()->getSubmodule("pastryRoutingTable"));
    leafSet = check_and_cast<PastryLeafSet*>
        (getParentModule()->getSubmodule("pastryLeafSet"));
    neighborhoodSet = check_and_cast<PastryNeighborhoodSet*>
        (getParentModule()->getSubmodule("pastryNeighborhoodSet"));

    stateCache.msg = NULL;
    stateCache.prox = NULL;

    // initialize statistics
    joins = 0;
    joinTries = 0;
    joinPartial = 0;
    joinSeen = 0;
    joinReceived = 0;
    joinSent = 0;
    stateSent = 0;
    stateReceived = 0;
    repairReqSent = 0;
    repairReqReceived = 0;
    stateReqSent = 0;
    stateReqReceived = 0;

    joinBytesSeen = 0;
    joinBytesReceived = 0;
    joinBytesSent = 0;
    stateBytesSent = 0;
    stateBytesReceived = 0;
    repairReqBytesSent = 0;
    repairReqBytesReceived = 0;
    stateReqBytesSent = 0;
    stateReqBytesReceived = 0;

    totalLookups = 0;
    responsibleLookups = 0;
    routingTableLookups = 0;
    closerNodeLookups = 0;
    closerNodeLookupsFromNeighborhood = 0;

    leafsetReqSent = 0;
    leafsetReqBytesSent = 0;
    leafsetReqReceived = 0;
    leafsetReqBytesReceived = 0;
    leafsetSent = 0;
    leafsetBytesSent = 0;
    leafsetReceived = 0;
    leafsetBytesReceived = 0;

    routingTableRowReqSent = 0;
    routingTableRowReqBytesSent = 0;
    routingTableRowReqReceived = 0;
    routingTableRowReqBytesReceived = 0;
    routingTableRowSent = 0;
    routingTableRowBytesSent = 0;
    routingTableRowReceived = 0;
    routingTableRowBytesReceived = 0;

    WATCH(joins);
    WATCH(joinTries);
    WATCH(joinSeen);
    WATCH(joinBytesSeen);
    WATCH(joinReceived);
    WATCH(joinBytesReceived);
    WATCH(joinSent);
    WATCH(joinBytesSent);
    WATCH(stateSent);
    WATCH(stateBytesSent);
    WATCH(stateReceived);
    WATCH(stateBytesReceived);
    WATCH(repairReqSent);
    WATCH(repairReqBytesSent);
    WATCH(repairReqReceived);
    WATCH(repairReqBytesReceived);
    WATCH(stateReqSent);
    WATCH(stateReqBytesSent);
    WATCH(stateReqReceived);
    WATCH(stateReqBytesReceived);
    WATCH(lastStateChange);

    WATCH(leafsetReqSent);
    WATCH(leafsetReqBytesSent);
    WATCH(leafsetReqReceived);
    WATCH(leafsetReqBytesReceived);
    WATCH(leafsetSent);
    WATCH(leafsetBytesSent);
    WATCH(leafsetReceived);
    WATCH(leafsetBytesReceived);

    WATCH(routingTableRowReqSent);
    WATCH(routingTableRowReqBytesSent);
    WATCH(routingTableRowReqReceived);
    WATCH(routingTableRowReqBytesReceived);
    WATCH(routingTableRowSent);
    WATCH(routingTableRowBytesSent);
    WATCH(routingTableRowReceived);
    WATCH(routingTableRowBytesReceived);

    WATCH_PTR(stateCache.msg);
}


void BasePastry::baseChangeState(int toState)
{
    switch (toState) {
    case INIT:
        state = INIT;

        if (!thisNode.getKey().isUnspecified())
            bootstrapList->removeBootstrapNode(thisNode);

        cancelAllRpcs();
        purgeVectors();

        bootstrapNode = bootstrapList->getBootstrapNode();

        routingTable->initializeTable(bitsPerDigit, repairTimeout, thisNode);
        leafSet->initializeSet(numberOfLeaves, bitsPerDigit,
                               repairTimeout, thisNode, this);
        neighborhoodSet->initializeSet(numberOfNeighbors, bitsPerDigit,
                                       thisNode);

        updateTooltip();
        lastStateChange = simTime();

        getParentModule()->getParentModule()->bubble("entering INIT state");

        break;

    case JOIN:
        state = JOIN;

        // bootstrapNode must be obtained before calling this method,
        // for example by calling changeState(INIT)

        if (bootstrapNode.isUnspecified()) {
            // no existing pastry network -> first node of a new one
            changeState(READY);
            return;
        }

        updateTooltip();
        getParentModule()->getParentModule()->bubble("entering JOIN state");

        RECORD_STATS(joinTries++);

        break;

    case READY:
        assert(state != READY);
        state = READY;

        // if we are the first node in the network,
        // there's nothing else to do
        if (bootstrapNode.isUnspecified()) {
            RECORD_STATS(joinTries++; joins++);
            setOverlayReady(true);
            return;
        }

        getParentModule()->getParentModule()->bubble("entering READY state");
        updateTooltip();
        RECORD_STATS(joins++);

        break;

    default: // discovery
        break;
    }
    setOverlayReady(state == READY);
}


void BasePastry::newLeafs(void)
{
    if (! enableNewLeafs) return;

    PastryNewLeafsMessage* msg = leafSet->getNewLeafsMessage();
    if (msg) {
        send(msg, "appOut");
        EV << "[BasePastry::newLeafs() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    newLeafs() called."
           << endl;
    }
}


void BasePastry::proxCallback(const TransportAddress& node, int rpcId,
                              cPolymorphic *contextPointer, Prox prox)
{
    Enter_Method("proxCallback()");

    EV << "[BasePastry::proxCallback() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Pong (or timeout) received (from "
           << node.getIp() << ")"
           << endl;

    double rtt = ((prox == Prox::PROX_TIMEOUT) ? PASTRY_PROX_INFINITE
                                               : prox.proximity);

    // merge single pinged nodes (bamboo global tuning)
    if (rpcId == PING_SINGLE_NODE) {
        routingTable->mergeNode((const NodeHandle&)node,
                                proximityNeighborSelection ?
                                rtt : SimTime::getMaxTime());
        delete contextPointer;
        return;
    }

    if (contextPointer != NULL && stateCache.msg && stateCache.prox) {
        PingContext* pingContext = check_and_cast<PingContext*>(contextPointer);

        if (pingContext->nonce != stateCache.nonce) {
            delete contextPointer;
            return;
        }
        // handle failed node
        if (rtt == PASTRY_PROX_INFINITE && state == READY) {
            handleFailedNode(node); // TODO
            updateTooltip();

            // this could initiate a re-join, exit the handler in that
            // case because all local data was erased:
            if (state != READY) {
                delete contextPointer;
                return;
            }
        }
        switch (pingContext->stateObject) {
        case ROUTINGTABLE:
            assert(stateCache.prox->pr_rt.size() > pingContext->index);
            *(stateCache.prox->pr_rt.begin() + pingContext->index) = rtt;
            break;

        case LEAFSET:
            assert(stateCache.prox->pr_ls.size() > pingContext->index);
            *(stateCache.prox->pr_ls.begin() + pingContext->index) = rtt;
            break;

        case NEIGHBORHOODSET:
            assert(stateCache.prox->pr_ns.size() > pingContext->index);
            *(stateCache.prox->pr_ns.begin() + pingContext->index) = rtt;
            break;

        default:
            throw cRuntimeError("wrong state object type!");
        }
        checkProxCache();
    }

    assert(stateCacheQueue.size() < 50);
    delete contextPointer;
}


void BasePastry::prePing(const PastryStateMessage* stateMsg)
{
    uint32_t rt_size = stateMsg->getRoutingTableArraySize();
    uint32_t ls_size = stateMsg->getLeafSetArraySize();
    uint32_t ns_size = stateMsg->getNeighborhoodSetArraySize();

    for (uint32_t i = 0; i < rt_size + ls_size + ns_size; i++) {
        const NodeHandle* node;
        if (i < rt_size) {
            node = &(stateMsg->getRoutingTable(i));
        }
        else if (i < (rt_size + ls_size) ) {
            node = &(stateMsg->getLeafSet(i - rt_size));
        }
        else {
            node = &(stateMsg->getNeighborhoodSet(i - rt_size - ls_size));
        }
        if ((node->isUnspecified()) || (*node == thisNode)) {
            continue;
        }

        neighborCache->getProx(*node, NEIGHBORCACHE_DEFAULT,
                               PING_RECEIVED_STATE, this, NULL);
    }
}


void BasePastry::pingNodes(void)
{
    EV << "[BasePastry::pingNodes() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]" << endl;

    if (stateCache.msg == NULL) throw cRuntimeError("no state msg");

    assert(stateCache.prox == NULL);
    stateCache.prox = new PastryStateMsgProximity();

    uint32_t rt_size = stateCache.msg->getRoutingTableArraySize();
    stateCache.prox->pr_rt.resize(rt_size, PASTRY_PROX_UNDEF);

    uint32_t ls_size = stateCache.msg->getLeafSetArraySize();
    stateCache.prox->pr_ls.resize(ls_size, PASTRY_PROX_UNDEF);

    uint32_t ns_size = stateCache.msg->getNeighborhoodSetArraySize();
    stateCache.prox->pr_ns.resize(ns_size, PASTRY_PROX_UNDEF);

    // set prox state
    for (uint32_t i = 0; i < rt_size + ls_size + ns_size; i++) {
        const NodeHandle* node;
        std::vector<simtime_t>::iterator proxPos;
        PingContext* pingContext = NULL;
        StateObject stateObject;
        uint32_t index;
        if (stateCache.msg == NULL) break;
        if (i < rt_size) {
            node = &(stateCache.msg->getRoutingTable(i));
            proxPos = stateCache.prox->pr_rt.begin() + i;
            stateObject = ROUTINGTABLE;
            index = i;
        } else if ( i < (rt_size + ls_size) ) {
            node = &(stateCache.msg->getLeafSet(i - rt_size));
            proxPos = stateCache.prox->pr_ls.begin() + (i - rt_size);
            stateObject = LEAFSET;
            index = i - rt_size;
        } else {
            node = &(stateCache.msg->getNeighborhoodSet(i - rt_size - ls_size));
            proxPos = stateCache.prox->pr_ns.begin() + (i - rt_size - ls_size);
            stateObject = NEIGHBORHOODSET;
            index = i - rt_size - ls_size;
        }
        // proximity is undefined for unspecified nodes:
        if (!node->isUnspecified()) {
            pingContext = new PingContext(stateObject, index,
                                          stateCache.nonce);

            Prox prox = neighborCache->getProx(*node, NEIGHBORCACHE_DEFAULT,
                                               PING_RECEIVED_STATE,
                                               this, pingContext);
            if (prox == Prox::PROX_SELF) {
                *proxPos = 0;
            } else if ((prox == Prox::PROX_TIMEOUT) ||
                       (prox == Prox::PROX_UNKNOWN)) {
                *proxPos = PASTRY_PROX_INFINITE;
            } else if (prox == Prox::PROX_WAITING) {
                *proxPos = PASTRY_PROX_PENDING;
            } else {
                *proxPos = prox.proximity;
            }
        } else {
            throw cRuntimeError("Undefined node in STATE message!");
        }
    }
    checkProxCache();
}


void BasePastry::determineAliveTable(const PastryStateMessage* stateMsg)
{
    uint32_t rt_size = stateMsg->getRoutingTableArraySize();
    aliveTable.pr_rt.clear();
    aliveTable.pr_rt.resize(rt_size, 1);

    uint32_t ls_size = stateMsg->getLeafSetArraySize();
    aliveTable.pr_ls.clear();
    aliveTable.pr_ls.resize(ls_size, 1);

    uint32_t ns_size = stateMsg->getNeighborhoodSetArraySize();
    aliveTable.pr_ns.clear();
    aliveTable.pr_ns.resize(ns_size, 1);

    for (uint32_t i = 0; i < rt_size + ls_size + ns_size; i++) {
        const TransportAddress* node;
        std::vector<simtime_t>::iterator tblPos;
        if (i < rt_size) {
            node = &(stateMsg->getRoutingTable(i));
            tblPos = aliveTable.pr_rt.begin() + i;
        } else if ( i < (rt_size + ls_size) ) {
            node = &(stateMsg->getLeafSet(i - rt_size));
            tblPos = aliveTable.pr_ls.begin() + (i - rt_size);
        } else {
            node = &(stateMsg->getNeighborhoodSet(i - rt_size - ls_size));
            tblPos = aliveTable.pr_ns.begin() + (i - rt_size - ls_size);
        }
        if (node->isUnspecified()) {
            *tblPos = PASTRY_PROX_UNDEF;
        } else if (neighborCache->getProx(*node,
                                          NEIGHBORCACHE_DEFAULT_IMMEDIATELY) ==
                   Prox::PROX_TIMEOUT) {
            *tblPos = PASTRY_PROX_INFINITE;
        }
    }
}


bool BasePastry::handleRpcCall(BaseCallMessage* msg)
{
    if (state != READY) {
        EV << "[BasePastry::handleRpcCall() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received RPC call and state != READY"
           << endl;
        return false;
    }

    // delegate messages
    RPC_SWITCH_START( msg )
    // RPC_DELEGATE( <messageName>[Call|Response], <methodToCall> )
    RPC_DELEGATE( RequestLeafSet, handleRequestLeafSetCall );
    RPC_DELEGATE( RequestRoutingRow, handleRequestRoutingRowCall );
    RPC_SWITCH_END( )

    return RPC_HANDLED;
}


void BasePastry::handleRpcResponse(BaseResponseMessage* msg,
                                   cPolymorphic* context, int rpcId,
                                   simtime_t rtt)
{
    RPC_SWITCH_START(msg)
    RPC_ON_RESPONSE( RequestLeafSet ) {
        EV << "[BasePastry::handleRpcResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received a RequestLeafSet RPC Response: id=" << rpcId << "\n"
           << "    msg=" << *_RequestLeafSetResponse << " rtt=" << SIMTIME_DBL(rtt)
           << endl;
        BasePastry::handleRequestLeafSetResponse(_RequestLeafSetResponse);
        break;
    }
    RPC_ON_RESPONSE( RequestRoutingRow ) {
        EV << "[BasePastry::handleRpcResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received a RequestRoutingRow RPC Response: id=" << rpcId << "\n"
           << "    msg=" << *_RequestRoutingRowResponse << " rtt=" << SIMTIME_DBL(rtt)
           << endl;
        BasePastry::handleRequestRoutingRowResponse(_RequestRoutingRowResponse);
        break;
    }
    RPC_SWITCH_END( )
}


void BasePastry::handleRpcTimeout(BaseCallMessage* call,
                                  const TransportAddress& dest,
                                  cPolymorphic* context, int rpcId,
                                  const OverlayKey& key)
{
    EV << "[BasePastry::handleRpcTimeout() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]\n"
       << "    Timeout of RPC Call: id=" << rpcId << "\n"
       << "    msg=" << *call << " key=" << key
       << endl;

    if (state == JOIN) {
        join();
    } else if (state == READY && !dest.isUnspecified() && key.isUnspecified()) {
        handleFailedNode(dest);
    }
}


void BasePastry::handleRequestLeafSetCall(RequestLeafSetCall* call)
{
    EV << "[BasePastry::handleRequestLeafSetCall() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(leafsetReqReceived++;
                 leafsetReqBytesReceived += call->getByteLength());

    if (state != READY) {
        EV << "    local node is NOT in READY state (call deleted)!" << endl;
        delete call;
        return;
    }

    RequestLeafSetResponse* response =
        new RequestLeafSetResponse("REQUEST LEAFSET Response");
    response->setTimestamp(simTime());
    response->setStatType(MAINTENANCE_STAT);

    response->setBitLength(PASTRYREQUESTLEAFSETRESPONSE_L(response));
    response->encapsulate(createStateMessage(PASTRY_STATE_LEAFSET));

    //merge leafset and sender's NodeHandle from call (if exists: PULL)
    if (call->getEncapsulatedPacket()) {
        EV << "    ... it's a leafSet PULL message!" << endl;

        PastryStateMessage* stateMsg =
            check_and_cast<PastryStateMessage*>(call->decapsulate());

        stateMsg->setLeafSetArraySize(stateMsg->getLeafSetArraySize() + 1);
        stateMsg->setLeafSet(stateMsg->getLeafSetArraySize() - 1,
                             stateMsg->getSender());

        handleStateMessage(stateMsg);

        //set sender's NodeHandle in response if it was set in call (PULL/PUSH)
        //response->setSender(thisNode);
        response->setName("LeafSet PUSH");
    }

    RECORD_STATS(leafsetSent++;
                 leafsetBytesSent += response->getByteLength());

    sendRpcResponse(call, response);
}


void BasePastry::handleRequestRoutingRowCall(RequestRoutingRowCall* call)
{
    EV << "[BasePastry::handleRequestRoutingRowCall() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(routingTableRowReqReceived++;
                 routingTableRowReqBytesReceived += call->getByteLength());

    if (state == READY) {
        // fill response with row entries
        if (call->getRow() > routingTable->getLastRow()) {
            EV << "    received request for nonexistent routing"
               << "table row, dropping message!"
               << endl;
            delete call;
        } else {
            RequestRoutingRowResponse* response =
                new RequestRoutingRowResponse("REQUEST ROUTINGROW Response");
            response->setStatType(MAINTENANCE_STAT);

            assert(call->getRow() >= -1 &&
                   call->getRow() <= routingTable->getLastRow());

            response->setBitLength(PASTRYREQUESTROUTINGROWRESPONSE_L(response));
            response->encapsulate(createStateMessage(PASTRY_STATE_ROUTINGROW,
                                                     -1,
                                                     (call->getRow() == -1) ?
                                                      routingTable->getLastRow() :
                                                      call->getRow()));

            RECORD_STATS(routingTableRowSent++;
                         routingTableRowBytesSent += call->getByteLength());
            sendRpcResponse(call, response);
        }
    } else {
        EV << "    received routing table request before reaching "
           << "READY state, dropping message!" << endl;
        delete call;
    }
}


void BasePastry::handleRequestLeafSetResponse(RequestLeafSetResponse* response)
{
    EV << "[BasePastry::handleRequestLeafSetResponse() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(leafsetReceived++; leafsetBytesReceived +=
                 response->getByteLength());

    if (state == READY) {
        handleStateMessage(check_and_cast<PastryStateMessage*>(response->decapsulate()));
    }
}


void BasePastry::handleRequestRoutingRowResponse(RequestRoutingRowResponse* response)
{
    EV << "[BasePastry::handleRequestRoutingRowResponse() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(routingTableRowReceived++;
                 routingTableRowBytesReceived += response->getByteLength());

    if (state == READY) {
        handleStateMessage(check_and_cast<PastryStateMessage*>(response->decapsulate()));
    }
}


PastryStateMessage* BasePastry::createStateMessage(enum PastryStateMsgType type,
                                                   simtime_t timestamp,
                                                   int16_t row,
                                                   bool lastHop)
{
    //checks
    if ((type & (PASTRY_STATE_JOIN | PASTRY_STATE_MINJOIN)) &&
        ((row == -1) || (timestamp != -1))) {
        throw cRuntimeError("Creating JOIN State without hopCount"
                            " or with timestamp!");
    } else if ((type & (PASTRY_STATE_ROUTINGROW)) && row == -1) {
        throw cRuntimeError("Creating ROUTINGROW State without row!");
    } else if ((type & (PASTRY_STATE_UPDATE)) &&
                timestamp == -1) {
        throw cRuntimeError("Creating UPDATE/REPAIR State without timestamp!");
    } else if ((type & (PASTRY_STATE_LEAFSET | PASTRY_STATE_STD |
                        PASTRY_STATE_REPAIR  | PASTRY_STATE_JOINUPDATE)) &&
            ((row != -1) || (timestamp != -1))) {
            throw cRuntimeError("Creating STD/UPDATE State with row/hopCount"
                                " or timestamp!");
    }

    std::string typeStr = cEnum::get("PastryStateMsgType")->getStringFor(type);
    typeStr.erase(0, 13);

    EV << "[BasePastry::sendStateTables() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]\n"
       << "    creating new state message (" << typeStr << ")" //TODO
       << endl;

    // create new state msg and set special fields for some types:
    PastryStateMessage* stateMsg =
        new PastryStateMessage((std::string("STATE (") + typeStr + ")").c_str());

    stateMsg->setTimestamp((timestamp == -1) ? simTime() : timestamp);

    if (type == PASTRY_STATE_JOIN || type == PASTRY_STATE_MINJOIN) {
        stateMsg->setRow(row);
        stateMsg->setLastHop(lastHop);
    }

    // fill in standard content:
    stateMsg->setStatType(MAINTENANCE_STAT);
    stateMsg->setPastryStateMsgType(type);
    stateMsg->setSender(thisNode);

    // the following part of the new join works on the assumption,
    // that the node routing the join message is close to the joining node
    // therefore it should be switched on together with the discovery algorithm
    if (type == PASTRY_STATE_MINJOIN) {
        //send just the needed row for new join protocol
        routingTable->dumpRowToMessage(stateMsg, row);
        if (lastHop) {
            leafSet->dumpToStateMessage(stateMsg);
        } else {
            stateMsg->setLeafSetArraySize(0);
        }
        if (row == 1) {
            neighborhoodSet->dumpToStateMessage(stateMsg);
        } else {
            stateMsg->setNeighborhoodSetArraySize(0);
        }
    } else if (type == PASTRY_STATE_LEAFSET) {
        leafSet->dumpToStateMessage(stateMsg);
    } else if (type == PASTRY_STATE_ROUTINGROW) {
        routingTable->dumpRowToMessage(stateMsg, row);
    } else {
        routingTable->dumpToStateMessage(stateMsg);
        leafSet->dumpToStateMessage(stateMsg);
        neighborhoodSet->dumpToStateMessage(stateMsg);
    }

    stateMsg->setBitLength(PASTRYSTATE_L(stateMsg));

    return stateMsg;
}


bool BasePastry::isSiblingFor(const NodeHandle& node,
                              const OverlayKey& key,
                              int numSiblings,
                              bool* err)
{
    if (key.isUnspecified())
        error("Pastry::isSiblingFor(): key is unspecified!");

    if ((numSiblings == 1) && (node == thisNode)) {
        if (leafSet->isClosestNode(key)) {
            *err = false;
            return true;
        } else {
            *err = false;
            return false;
        }
    }

    NodeVector* result =  leafSet->createSiblingVector(key, numSiblings);

    if (result == NULL) {
        *err = true;
        return false;
    }

    if (result->contains(node.getKey())) {
        delete result;
        *err = false;
        return true;
    } else {
        delete result;
        *err = true;
        return false;
    }
}


void BasePastry::handleAppMessage(BaseOverlayMessage* msg)
{
    delete msg;
}


void BasePastry::updateTooltip()
{
    if (ev.isGUI()) {
        std::stringstream ttString;

        // show our predecessor and successor in tooltip
        ttString << leafSet->getPredecessor() << endl << thisNode << endl
                 << leafSet->getSuccessor();

        getParentModule()->getParentModule()->getDisplayString().
            setTagArg("tt", 0, ttString.str().c_str());
        getParentModule()->getDisplayString().
            setTagArg("tt", 0, ttString.str().c_str());
        getDisplayString().setTagArg("tt", 0, ttString.str().c_str());

        // draw arrows:
        showOverlayNeighborArrow(leafSet->getSuccessor(), true,
                                 "m=m,50,0,50,0;ls=red,1");
        showOverlayNeighborArrow(leafSet->getPredecessor(), false,
                                 "m=m,50,100,50,100;ls=green,1");
    }
}

BasePastry::~BasePastry()
{
    purgeVectors();
}

void BasePastry::finishOverlay()
{
    // remove this node from the bootstrap list
    if (!thisNode.getKey().isUnspecified()) bootstrapList->removeBootstrapNode(thisNode);

    // collect statistics
    simtime_t time = globalStatistics->calcMeasuredLifetime(creationTime);
    if (time < GlobalStatistics::MIN_MEASURED) return;

    // join statistics: only evaluate join from nodes
    // that have been created in measurement phase
    if (globalStatistics->getMeasureNetwInitPhase() ||
        time >= (simTime() - creationTime)) {
        // join is on the way...
        if (joinTries > 0 && state == JOIN) joinTries--;
        if (joinTries > 0) {
            globalStatistics->addStdDev("BasePastry: join success ratio", (double)joins / (double)joinTries);
            globalStatistics->addStdDev("BasePastry: join tries", joinTries);
        }
    }

    // TODO new methods Pastry::finnishOverlay() / Bamboo::finishOverlay()
    globalStatistics->addStdDev("Pastry: joins with missing replies from routing path/s",
                                joinPartial / time);
    globalStatistics->addStdDev("Pastry: JOIN Messages seen/s", joinSeen / time);
    globalStatistics->addStdDev("Pastry: bytes of JOIN Messages seen/s", joinBytesSeen / time);
    globalStatistics->addStdDev("Pastry: JOIN Messages received/s", joinReceived / time);
    globalStatistics->addStdDev("Pastry: bytes of JOIN Messages received/s",
                                joinBytesReceived / time);
    globalStatistics->addStdDev("Pastry: JOIN Messages sent/s", joinSent / time);
    globalStatistics->addStdDev("Pastry: bytes of JOIN Messages sent/s", joinBytesSent / time);

    globalStatistics->addStdDev("Pastry: REPAIR Requests sent/s", repairReqSent / time);
    globalStatistics->addStdDev("Pastry: bytes of REPAIR Requests sent/s",
                                repairReqBytesSent / time);
    globalStatistics->addStdDev("Pastry: REPAIR Requests received/s", repairReqReceived / time);
    globalStatistics->addStdDev("Pastry: bytes of REPAIR Requests received/s",
                                repairReqBytesReceived / time);

    globalStatistics->addStdDev("Pastry: STATE Requests sent/s", stateReqSent / time);
    globalStatistics->addStdDev("Pastry: bytes of STATE Requests sent/s", stateReqBytesSent / time);
    globalStatistics->addStdDev("Pastry: STATE Requests received/s", stateReqReceived / time);
    globalStatistics->addStdDev("Pastry: bytes of STATE Requests received/s",
                                stateReqBytesReceived / time);
    globalStatistics->addStdDev("Pastry: STATE Messages sent/s", stateSent / time);
    globalStatistics->addStdDev("Pastry: bytes of STATE Messages sent/s", stateBytesSent / time);
    globalStatistics->addStdDev("Pastry: STATE Messages received/s", stateReceived / time);
    globalStatistics->addStdDev("Pastry: bytes of STATE Messages received/s",
                                stateBytesReceived / time);

    globalStatistics->addStdDev("BasePastry: LEAFSET Requests sent/s", leafsetReqSent / time);
    globalStatistics->addStdDev("BasePastry: bytes of LEAFSET Requests sent/s", leafsetReqBytesSent / time);
    globalStatistics->addStdDev("BasePastry: LEAFSET Requests received/s", leafsetReqReceived / time);
    globalStatistics->addStdDev("BasePastry: bytes of LEAFSET Requests received/s",
                                leafsetReqBytesReceived / time);
    globalStatistics->addStdDev("BasePastry: LEAFSET Messages sent/s", leafsetSent / time);
    globalStatistics->addStdDev("BasePastry: bytes of LEAFSET Messages sent/s", leafsetBytesSent / time);
    globalStatistics->addStdDev("BasePastry: LEAFSET Messages received/s", leafsetReceived / time);
    globalStatistics->addStdDev("BasePastry: bytes of LEAFSET Messages received/s",
                                leafsetBytesReceived / time);

    globalStatistics->addStdDev("BasePastry: ROUTING TABLE ROW Requests sent/s", routingTableRowReqSent / time);
    globalStatistics->addStdDev("BasePastry: bytes of ROUTING TABLE ROW Requests sent/s", routingTableRowReqBytesSent / time);
    globalStatistics->addStdDev("BasePastry: ROUTING TABLE ROW Requests received/s", routingTableRowReqReceived / time);
    globalStatistics->addStdDev("BasePastry: bytes of ROUTING TABLE ROW Requests received/s",
                                routingTableRowReqBytesReceived / time);
    globalStatistics->addStdDev("BasePastry: ROUTING TABLE ROW Messages sent/s", routingTableRowSent / time);
    globalStatistics->addStdDev("BasePastry: bytes of ROUTING TABLE ROW Messages sent/s", routingTableRowBytesSent / time);
    globalStatistics->addStdDev("BasePastry: ROUTING TABLE ROW Messages received/s", routingTableRowReceived / time);
    globalStatistics->addStdDev("BasePastry: bytes of ROUTING TABLE ROW Messages received/s",
                                routingTableRowBytesReceived / time);

    globalStatistics->addStdDev("BasePastry: total number of lookups", totalLookups);
    globalStatistics->addStdDev("BasePastry: responsible lookups", responsibleLookups);
    globalStatistics->addStdDev("BasePastry: lookups in routing table", routingTableLookups);
    globalStatistics->addStdDev("BasePastry: lookups using closerNode()", closerNodeLookups);
    globalStatistics->addStdDev("BasePastry: lookups using closerNode() with result from "
                                "neighborhood set", closerNodeLookupsFromNeighborhood);
}


int BasePastry::getMaxNumSiblings()
{
    return (int)floor(numberOfLeaves / 2.0);
}


int BasePastry::getMaxNumRedundantNodes()
{
    return (int)floor(numberOfLeaves);
}


NodeVector* BasePastry::findNode(const OverlayKey& key,
                                 int numRedundantNodes,
                                 int numSiblings,
                                 BaseOverlayMessage* msg)
{
    if ((numRedundantNodes > getMaxNumRedundantNodes()) ||
        (numSiblings > getMaxNumSiblings())) {

        opp_error("(Pastry::findNode()) numRedundantNodes or numSiblings "
                  "too big!");
    }
    RECORD_STATS(totalLookups++);

    NodeVector* nextHops = new NodeVector(numRedundantNodes);

    if (state != READY) {
        return nextHops;
    } else if (key.isUnspecified() || leafSet->isClosestNode(key)) {
        RECORD_STATS(responsibleLookups++);
        nextHops->add(thisNode);
    } else {
        const NodeHandle* next = &(leafSet->getDestinationNode(key));

        if (next->isUnspecified()) {
            next = &(routingTable->lookupNextHop(key));
            if (!next->isUnspecified()) {
                RECORD_STATS(routingTableLookups++);
            }
        } else {
            RECORD_STATS(responsibleLookups++);
        }

        if (next->isUnspecified()) {
            RECORD_STATS(closerNodeLookups++);
            // call findCloserNode() on all state objects
            if (optimizeLookup) {
                const NodeHandle* tmp;
                next = &(routingTable->findCloserNode(key, true));
                tmp = &(neighborhoodSet->findCloserNode(key, true));

                if ((! tmp->isUnspecified()) &&
                    (leafSet->isCloser(*tmp, key, *next))) {
                    RECORD_STATS(closerNodeLookupsFromNeighborhood++);
                    next = tmp;
                }

                tmp = &(leafSet->findCloserNode(key, true));
                if ((! tmp->isUnspecified()) &&
                    (leafSet->isCloser(*tmp, key, *next))) {
                    RECORD_STATS(closerNodeLookupsFromNeighborhood--);
                    next = tmp;
                }
            } else {
                next = &(routingTable->findCloserNode(key));

                if (next->isUnspecified()) {
                    RECORD_STATS(closerNodeLookupsFromNeighborhood++);
                    next = &(neighborhoodSet->findCloserNode(key));
                }

                if (next->isUnspecified()) {
                    RECORD_STATS(closerNodeLookupsFromNeighborhood--);
                    next = &(leafSet->findCloserNode(key));
                }
            }
        }

        iterativeJoinHook(msg, !next->isUnspecified());

        if (!next->isUnspecified()) {
            nextHops->add(*next);
        }
    }

    bool err;

    // if we're a sibling, return all numSiblings
    if ((numSiblings >= 0) && isSiblingFor(thisNode, key, numSiblings, &err)) {
        if (err == false) {
            delete nextHops;
            return  leafSet->createSiblingVector(key, numSiblings);
        }
    }

    if (/*(nextHops->size() > 0) &&*/ (numRedundantNodes > 1)) {

        //memleak... comp should be a ptr and deleted in NodeVector::~NodeVector()...
        //KeyDistanceComparator<KeyRingMetric>* comp =
        //    new KeyDistanceComparator<KeyRingMetric>( key );

        KeyDistanceComparator<KeyRingMetric> comp(key);
        //KeyDistanceComparator<KeyPrefixMetric> comp(key);
        NodeVector* additionalHops = new NodeVector( numRedundantNodes, &comp );

        routingTable->findCloserNodes(key, additionalHops);
        leafSet->findCloserNodes(key, additionalHops);
        neighborhoodSet->findCloserNodes(key, additionalHops);

        if (useRegularNextHop && (nextHops->size() > 0) &&
            (*additionalHops)[0] != (*nextHops)[0]) {
            for (uint32_t i = 0; i < additionalHops->size(); i++) {
                if ((*additionalHops)[i] != (*nextHops)[0])
                    nextHops->push_back((*additionalHops)[i]);
            }
            delete additionalHops;
        } else {
            delete nextHops;
            return additionalHops;
        }
    }
    return nextHops;
}


AbstractLookup* BasePastry::createLookup(RoutingType routingType,
                                         const BaseOverlayMessage* msg,
                                         const cPacket* dummy,
                                         bool appLookup)
{
    assert(dummy == NULL);

    PastryFindNodeExtData* findNodeExt =
        new PastryFindNodeExtData("findNodeExt");

    if (msg &&
        dynamic_cast<const PastryJoinCall*>(msg->getEncapsulatedPacket())) {
        const PastryJoinCall* joinCall =
            static_cast<const PastryJoinCall*>(msg->getEncapsulatedPacket());
        assert(!joinCall->getSrcNode().isUnspecified());
        findNodeExt->setSendStateTo(joinCall->getSrcNode());
        findNodeExt->setJoinHopCount(1);
    }
    findNodeExt->setBitLength(PASTRYFINDNODEEXTDATA_L);

    AbstractLookup* newLookup = BaseOverlay::createLookup(routingType,
                                                          msg, findNodeExt,
                                                          appLookup);

    delete findNodeExt;
    return newLookup;
}


bool stateMsgIsSmaller(const PastryStateMsgHandle& hnd1,
                       const PastryStateMsgHandle& hnd2)
{
    return (hnd1.msg->getRow() < hnd2.msg->getRow());
}


uint8_t BasePastry::getRTLastRow() const
{
    return routingTable->getLastRow();
};


std::vector<TransportAddress>* BasePastry::getRTRow(uint8_t index) const
{
    return routingTable->getRow(index);
};


std::vector<TransportAddress>* BasePastry::getLeafSet() const
{
    std::vector<TransportAddress>* ret = new std::vector<TransportAddress>;
    leafSet->dumpToVector(*ret);

    return ret;
};


std::ostream& operator<<(std::ostream& os, const PastryStateMsgProximity& pr)
{
    os << "PastryStateMsgProximity {" << endl;
    os << "  pr_rt {" << endl;
    for (std::vector<simtime_t>::const_iterator i = pr.pr_rt.begin();
         i != pr.pr_rt.end(); ++i) {
        os << "    " << *i << endl;
    }
    os << "  }" << endl;
    os << "  pr_ls {" << endl;
    for (std::vector<simtime_t>::const_iterator i = pr.pr_ls.begin();
         i != pr.pr_ls.end(); ++i) {
        os << "    " << *i << endl;
    }
    os << "  }" << endl;
    os << "  pr_ns {" << endl;
    for (std::vector<simtime_t>::const_iterator i = pr.pr_ns.begin();
         i != pr.pr_ns.end(); ++i) {
        os << "    " << *i << endl;
    }
    os << "  }" << endl;
    os << "}" << endl;
    return os;
}


OverlayKey BasePastry::estimateMeanDistance()
{
    return leafSet->estimateMeanDistance();
}


//virtual public: distance metric
OverlayKey BasePastry::distance(const OverlayKey& x,
                                const OverlayKey& y,
                                bool useAlternative) const
{
    if (!useAlternative) return KeyRingMetric().distance(x, y);
    return KeyPrefixMetric().distance(x, y);
}
