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
 * @file Pastry.cc
 * @author Felix Palmen, Gerhard Petruschat, Bernhard Heep
 */

#include <cassert>

#include <IPAddressResolver.h>
#include <IPvXAddress.h>
#include <IInterfaceTable.h>
#include <IPv4InterfaceData.h>
#include <RpcMacros.h>
#include <InitStages.h>
#include <GlobalStatistics.h>

#include "Pastry.h"


Define_Module(Pastry);

Pastry::~Pastry()
{
    // destroy self timer messages
    cancelAndDelete(readyWait);
    cancelAndDelete(joinUpdateWait);
    cancelAndDelete(secondStageWait);
    if (useDiscovery) cancelAndDelete(discoveryTimeout);
    if (routingTableMaintenanceInterval > 0) cancelAndDelete(repairTaskTimeout);

    clearVectors();
}


void Pastry::clearVectors()
{
    // purge pending state messages
    if (!stReceived.empty()) {
        for (std::vector<PastryStateMsgHandle>::iterator it =
            stReceived.begin(); it != stReceived.end(); it++) {
            // check whether one of the pointers is a duplicate of stateCache
            if (it->msg == stateCache.msg) stateCache.msg = NULL;
            if (it->prox == stateCache.prox) stateCache.prox = NULL;
            delete it->msg;
            delete it->prox;
        }
        stReceived.clear();
        stReceivedPos = stReceived.end();
    }

    // purge notify list:
    notifyList.clear();
}


void Pastry::purgeVectors(void)
{
    clearVectors();

    // purge vector of waiting sendState messages:
    if (! sendStateWait.empty()) {
        for (std::vector<PastrySendState*>::iterator it =
                 sendStateWait.begin(); it != sendStateWait.end(); it++) {
            if ( (*it)->isScheduled() ) cancelEvent(*it);
            delete *it;
        }
        sendStateWait.clear();
    }

    BasePastry::purgeVectors();
}


void Pastry::initializeOverlay(int stage)
{
    if ( stage != MIN_STAGE_OVERLAY )
        return;

    // Pastry provides KBR services
    kbr = true;

    baseInit();

    useDiscovery = par("useDiscovery");
    useSecondStage = par("useSecondStage");
    pingBeforeSecondStage = par("pingBeforeSecondStage");
    secondStageInterval = par("secondStageWait");
    discoveryTimeoutAmount = par("discoveryTimeoutAmount");
    useRoutingTableMaintenance = par("useRoutingTableMaintenance");
    routingTableMaintenanceInterval = par("routingTableMaintenanceInterval");
    sendStateAtLeafsetRepair = par("sendStateAtLeafsetRepair");
    partialJoinPath = par("partialJoinPath");
    readyWaitAmount = par("readyWait");
    minimalJoinState = par("minimalJoinState");

    overrideOldPastry = par("overrideOldPastry");
    overrideNewPastry = par("overrideNewPastry");

    if (overrideOldPastry) {
        useSecondStage = true;
        useDiscovery = false;
        useRoutingTableMaintenance = false;
        sendStateAtLeafsetRepair = true;
        minimalJoinState = false;
    }

    if (overrideNewPastry) {
        useSecondStage = false;
        useDiscovery = true;
        useRoutingTableMaintenance = true;
        sendStateAtLeafsetRepair = false;
        minimalJoinState = true;
    }

    readyWait = new cMessage("readyWait");
    secondStageWait = new cMessage("secondStageWait");
    joinUpdateWait = new cMessage("joinUpdateWait");

    discoveryTimeout =
        (useDiscovery ? new cMessage("discoveryTimeout") : NULL);
    repairTaskTimeout =
        (useRoutingTableMaintenance ? new cMessage("repairTaskTimeout") : NULL);

    updateCounter = 0;
}


void Pastry::joinOverlay()
{
    changeState(INIT);

    if (bootstrapNode.isUnspecified()) {
        // no existing pastry network -> first node of a new one
        changeState(READY);
    } else {
        // join existing pastry network
        nearNode = bootstrapNode;
        if (useDiscovery) changeState(DISCOVERY);
        else changeState(JOIN);
    }
}


void Pastry::changeState(int toState)
{
    if (readyWait->isScheduled()) cancelEvent(readyWait);
    baseChangeState(toState);

    switch (toState) {
    case INIT:
        cancelAllRpcs();
        purgeVectors();
        break;

    case DISCOVERY: {
        state = DISCOVERY;
        nearNodeRtt = MAXTIME;
        discoveryModeProbedNodes = 0;
        pingNode(bootstrapNode, discoveryTimeoutAmount, 0,
                 NULL, "PING bootstrapNode in discovery mode",
                 NULL, PING_DISCOVERY, UDP_TRANSPORT); //TODO

        RequestLeafSetCall* call =
            new RequestLeafSetCall("REQUEST LEAFSET Call");
        call->setStatType(MAINTENANCE_STAT);
        call->setBitLength(PASTRYREQUESTLEAFSETCALL_L(call));
        RECORD_STATS(leafsetReqSent++;
                     leafsetReqBytesSent += call->getByteLength());
        sendUdpRpcCall(bootstrapNode, call);

        depth = -1;
    }
    break;

    case JOIN: {
        joinHopCount = 0;

        PastryJoinCall* call = new PastryJoinCall("JOIN Call");
        call->setStatType(MAINTENANCE_STAT);
        call->setBitLength(PASTRYJOINCALL_L(msg));
        RECORD_STATS(joinSent++; joinBytesSent += call->getByteLength());
        sendRouteRpcCall(OVERLAY_COMP, nearNode, thisNode.getKey(), call);
    }
    break;

    case READY:
        // determine list of all known nodes as notifyList
        notifyList.clear();
        leafSet->dumpToVector(notifyList);
        routingTable->dumpToVector(notifyList);
        sort(notifyList.begin(), notifyList.end());
        notifyList.erase(unique(notifyList.begin(), notifyList.end()),
                         notifyList.end());

        // schedule update
        cancelEvent(joinUpdateWait);
        scheduleAt(simTime() + 0.0001, joinUpdateWait);

        // schedule second stage
        if (useSecondStage) {
            cancelEvent(secondStageWait);
            scheduleAt(simTime() + secondStageInterval, secondStageWait);
        }

        // schedule routing table maintenance task
        if (useRoutingTableMaintenance) {
            cancelEvent(repairTaskTimeout);
            scheduleAt(simTime() + routingTableMaintenanceInterval, repairTaskTimeout);
        }
        break;
    }
}


void Pastry::pingResponse(PingResponse* pingResponse,
                          cPolymorphic* context, int rpcId,
                          simtime_t rtt)
{
    if (state == DISCOVERY) {
        EV << "[Pastry::pingResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Pong (or Ping-context from NeighborCache) received (from "
           << pingResponse->getSrcNode().getIp() << ") in DISCOVERY mode"
           << endl;

        if (nearNodeRtt > rtt) {
            nearNode = pingResponse->getSrcNode();
            nearNodeRtt = rtt;
            nearNodeImproved = true;
        }
    }
}


void Pastry::handleTimerEvent(cMessage* msg)
{
    if (msg == readyWait) {
        if (partialJoinPath) {
            RECORD_STATS(joinPartial++);
            sort(stReceived.begin(), stReceived.end(), stateMsgIsSmaller);

            // start pinging the nodes found in the first state message:
            stReceivedPos = stReceived.begin();
            stateCache = *stReceivedPos;
            EV << "[Pastry::handleTimerEvent() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    joining despite some missing STATE messages."
               << endl;
            processState();
        } else {
            EV << "[Pastry::handleTimerEvent() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    timeout waiting for missing state messages"
               << " in JOIN state, restarting..."
               << endl;
            join();
        }
    } else if (msg == joinUpdateWait) {
        EV << "[Pastry::handleTimerEvent() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    sending state updates to all nodes."
           << endl;
        doJoinUpdate();
    } else if (msg == secondStageWait) {
        EV << "[Pastry::handleTimerEvent() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    sending STATE requests to all nodes in"
           << " second stage of initialization."
           << endl;
        doSecondStage();
    } else if (msg == discoveryTimeout) {
        if ((depth == 0) && (nearNodeImproved)) {
            depth++; //repeat last step if closer node was found
        }
        if ((depth == 0) || (discoveryModeProbedNodes < 1)) {
            changeState(JOIN);
        } else {
            RequestRoutingRowCall* call =
                new RequestRoutingRowCall("REQUEST ROUTING ROW Call");
            call->setStatType(MAINTENANCE_STAT);
            call->setRow(depth);
            call->setBitLength(PASTRYREQUESTROUTINGROWCALL_L(call));
            RECORD_STATS(routingTableRowReqSent++;
                         routingTableRowReqBytesSent += call->getByteLength());
            sendUdpRpcCall(nearNode, call);
        }
    } else if (msg == repairTaskTimeout) {
        EV << "[Pastry::handleTimerEvent() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    starting routing table maintenance"
           << endl;
        doRoutingTableMaintenance();
        scheduleAt(simTime() + routingTableMaintenanceInterval,
                   repairTaskTimeout);
    } else if (dynamic_cast<PastrySendState*>(msg)) {
        PastrySendState* sendStateMsg = static_cast<PastrySendState*>(msg);

        std::vector<PastrySendState*>::iterator pos =
            std::find(sendStateWait.begin(), sendStateWait.end(),
                      sendStateMsg);
        if (pos != sendStateWait.end()) sendStateWait.erase(pos);

        PastryStateMessage* stateMsg = createStateMessage();
        RECORD_STATS(stateSent++;
                     stateBytesSent += stateMsg->getByteLength());
        sendMessageToUDP(sendStateMsg->getDest(), stateMsg);

        delete sendStateMsg;
    }
}


void Pastry::sendStateDelayed(const TransportAddress& destination)
{
    PastrySendState* selfMsg = new PastrySendState("sendStateWait");
    selfMsg->setDest(destination);
    sendStateWait.push_back(selfMsg);
    scheduleAt(simTime() + 0.0001, selfMsg);
}


void Pastry::handleUDPMessage(BaseOverlayMessage* msg)
{
    PastryStateMessage* stateMsg = check_and_cast<PastryStateMessage*>(msg);
    uint32_t type = stateMsg->getPastryStateMsgType();

    if (debugOutput) {
        EV << "[Pastry::handleUDPMessage() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    incoming STATE message of type "
           << cEnum::get("PastryStateMsgType")->getStringFor(type) << endl;
    }

    RECORD_STATS(stateReceived++; stateBytesReceived +=
                 stateMsg->getByteLength());

    handleStateMessage(stateMsg);
}


bool Pastry::handleRpcCall(BaseCallMessage* msg)
{
    if (BasePastry::handleRpcCall(msg)) return true;

    if (state != READY) {
        EV << "[Pastry::handleRpcCall() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received RPC call and state != READY"
           << endl;
        return false;
    }

    // delegate messages
    RPC_SWITCH_START( msg )
    // RPC_DELEGATE( <messageName>[Call|Response], <methodToCall> )
    RPC_DELEGATE( PastryJoin, handlePastryJoinCall );
    RPC_DELEGATE( RequestState, handleRequestStateCall );
    RPC_DELEGATE( RequestRepair, handleRequestRepairCall );
    RPC_SWITCH_END( )

    return RPC_HANDLED;
}


void Pastry::handlePastryJoinCall(PastryJoinCall* call)
{
    EV << "[Pastry::handlePastryJoinCall() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(joinReceived++;
                 joinBytesReceived += call->getByteLength());

    if (state != READY) {
        if (call->getSrcNode() == thisNode) {
            EV << "[Pastry::handlePastryJoinCall() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    PastryJoinCall received by originator!"
               << endl;
        } else {
            EV << "[Pastry::handlePastryJoinCall() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    received join message before reaching "
               << "READY state, dropping message!"
               << endl;
        }
    } else if (call->getSrcNode() == thisNode) {
        EV << "[Pastry::handlePastryJoinCall() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    PastryJoinCall gets dropped because it is "
           << "outdated and has been received by originator!"
           << endl;
    } else {
        OverlayCtrlInfo* overlayCtrlInfo =
            check_and_cast<OverlayCtrlInfo*>(call->getControlInfo());

        uint32_t joinHopCount =  overlayCtrlInfo->getHopCount();
        if ((joinHopCount > 1) &&
                ((defaultRoutingType == ITERATIVE_ROUTING) ||
                        (defaultRoutingType == EXHAUSTIVE_ITERATIVE_ROUTING)))
            joinHopCount--;

        // remove node from state if it is rejoining
        handleFailedNode(call->getSrcNode());

        PastryJoinResponse* response = new PastryJoinResponse("JOIN Response");

        // create new state msg and set special fields for some types:
        response->setStatType(MAINTENANCE_STAT);
        response->setTimestamp(simTime());

        response->setBitLength(PASTRYJOINRESPONSE_L(response));
        response->encapsulate(createStateMessage((minimalJoinState ?
                                                  PASTRY_STATE_MINJOIN :
                                                  PASTRY_STATE_JOIN),
                                                  -1, joinHopCount, true));

        // send...
        RECORD_STATS(stateSent++;
                     stateBytesSent += response->getByteLength());

        sendRpcResponse(call, response);
    }
}


void Pastry::handleRequestStateCall(RequestStateCall* call)
{
    EV << "[Pastry::handleRequestStateCall() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(stateReqReceived++;
                 stateReqBytesReceived += call->getByteLength());

    if (state != READY) {
        EV << "    received repair request before reaching"
                << " READY state, dropping message!"
                << endl;
        delete call;
        return;
    }

    RequestStateResponse* response =
        new RequestStateResponse("REQUEST STATE Response");
    response->setStatType(MAINTENANCE_STAT);

    response->setBitLength(PASTRYREQUESTSTATERESPONSE_L(response));
    response->encapsulate(createStateMessage());
    RECORD_STATS(stateSent++;
                 stateBytesSent += response->getByteLength());

    sendRpcResponse(call, response);
}


void Pastry::handleRequestRepairCall(RequestRepairCall* call)
{
    EV << "[Pastry::handleRequestRepairCall() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(repairReqReceived++;
                 repairReqBytesReceived += call->getByteLength());

    if (state != READY) {
        EV << "    received repair request before reaching"
           << " READY state, dropping message!"
           << endl;
        delete call;
        return;
    }

    RequestRepairResponse* response =
        new RequestRepairResponse("REQUEST REPAIR Response");
    response->setStatType(MAINTENANCE_STAT);

    response->setBitLength(PASTRYREQUESTREPAIRRESPONSE_L(response));
    response->encapsulate(createStateMessage(PASTRY_STATE_REPAIR));
    RECORD_STATS(stateSent++;
                 stateBytesSent += response->getByteLength());

    sendRpcResponse(call, response);
}


void Pastry::handleRequestRepairResponse(RequestRepairResponse* response)
{
    EV << "[Pastry::handleRequestRepairResponse() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(stateReceived++;
                 stateBytesReceived += response->getByteLength());

    if (state == READY) {
        handleStateMessage(check_and_cast<PastryStateMessage*>(response->decapsulate()));
    }
}


void Pastry::handleRpcResponse(BaseResponseMessage* msg,
                               cPolymorphic* context, int rpcId,
                               simtime_t rtt)
{
    BasePastry::handleRpcResponse(msg, context, rpcId, rtt);

    RPC_SWITCH_START(msg)
    RPC_ON_RESPONSE( PastryJoin ) {
        EV << "[Pastry::handleRpcResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received a JOIN RPC Response: id=" << rpcId << "\n"
           << "    msg=" << *_PastryJoinResponse << " rtt=" << SIMTIME_DBL(rtt)
           << endl;
        handlePastryJoinResponse(_PastryJoinResponse);
        break;
        }
    RPC_ON_RESPONSE( RequestState ) {
        EV << "[Pastry::handleRpcResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received a RequestState RPC Response: id=" << rpcId << "\n"
           << "    msg=" << *_RequestStateResponse << " rtt=" << SIMTIME_DBL(rtt)
           << endl;
        handleRequestStateResponse(_RequestStateResponse);
        break;
    }
    RPC_ON_RESPONSE( RequestRepair ) {
        EV << "[BasePastry::handleRpcResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received a Request Repair RPC Response: id=" << rpcId << "\n"
           << "    msg=" << *_RequestRepairResponse << " rtt=" << SIMTIME_DBL(rtt)
           << endl;
        handleRequestRepairResponse(_RequestRepairResponse);
        break;
    }
    RPC_ON_RESPONSE( RequestLeafSet ) {
        EV << "[Pastry::handleRpcResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received a RequestLeafSet RPC Response: id=" << rpcId << "\n"
           << "    msg=" << *_RequestLeafSetResponse << " rtt=" << SIMTIME_DBL(rtt)
           << endl;
        handleRequestLeafSetResponse(_RequestLeafSetResponse);
        break;
    }
    RPC_ON_RESPONSE( RequestRoutingRow ) {
        EV << "[Pastry::handleRpcResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Received a RequestRoutingRow RPC Response: id=" << rpcId << "\n"
           << "    msg=" << *_RequestRoutingRowResponse << " rtt=" << rtt
           << endl;
        handleRequestRoutingRowResponse(_RequestRoutingRowResponse);
        break;
    }
    RPC_SWITCH_END( )
}


void Pastry::handleRpcTimeout(BaseCallMessage* call,
                              const TransportAddress& dest,
                              cPolymorphic* context, int rpcId,
                              const OverlayKey& key)
{
    BasePastry::handleRpcTimeout(call, dest, context, rpcId, key);

    EV << "[Pastry::handleRpcTimeout() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]\n"
       << "    Timeout of RPC Call: id=" << rpcId << "\n"
       << "    msg=" << *call << " key=" << key
       << endl;

    if (state == DISCOVERY && dynamic_cast<RequestLeafSetCall*>(call)) {
        join();
    }
}


void Pastry::handlePastryJoinResponse(PastryJoinResponse* response)
{
    EV << "[Pastry::handlePastryJoinResponse() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]"
           << endl;

    RECORD_STATS(stateReceived++;
                 stateBytesReceived += response->getByteLength());

    if (state == JOIN) {
        handleStateMessage(check_and_cast<PastryStateMessage*>(response->decapsulate()));
    }
}

void Pastry::handleRequestStateResponse(RequestStateResponse* response)
{
    EV << "[Pastry::handleRequestStateResponse() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    RECORD_STATS(stateReceived++;
                 stateBytesReceived += response->getByteLength());

    if (state == READY) {
        handleStateMessage(check_and_cast<PastryStateMessage*>(response->decapsulate()));
    }
}


void Pastry::handleRequestLeafSetResponse(RequestLeafSetResponse* response)
{
    EV << "[Pastry::handleRequestLeafSetResponse() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    if (state == DISCOVERY) {
        const NodeHandle* node;
        discoveryModeProbedNodes = 0;
        PastryStateMessage* leaves =
            check_and_cast<PastryStateMessage*>(response->getEncapsulatedPacket());
        for (uint32_t i = 0; i < leaves->getLeafSetArraySize(); ++i) {
            node = &(leaves->getLeafSet(i));
            // unspecified nodes not considered
            if ( !(node->isUnspecified()) ) {
                pingNode(*node, discoveryTimeoutAmount, 0,
                         NULL, "PING received leaves for nearest node",
                         NULL, -1, UDP_TRANSPORT); //TODO
                discoveryModeProbedNodes++;
            }
        }

        EV << "    received leafset, waiting for pings"
           << endl;

        if (discoveryTimeout->isScheduled()) cancelEvent(discoveryTimeout);
        scheduleAt(simTime() + discoveryTimeoutAmount, discoveryTimeout);
    }
}


void Pastry::handleRequestRoutingRowResponse(RequestRoutingRowResponse* response)
{
    EV << "[Pastry::handleRequestRoutingRowResponse() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    if (state == DISCOVERY) {
        PastryStateMessage* rowState =
            check_and_cast<PastryStateMessage*>(response->getEncapsulatedPacket());
        uint32_t nodesPerRow = rowState->getRoutingTableArraySize();
        const NodeHandle* node;
        if (depth == -1) {
            depth = rowState->getRow();
        }
        discoveryModeProbedNodes = 0;
        nearNodeImproved = false;

        if (depth > 0) {
            for (uint32_t i = 0; i < nodesPerRow; i++) {
                node = &(rowState->getRoutingTable(i));
                // unspecified nodes not considered
                if ( !(node->isUnspecified()) ) {
                    // we look for best connection here,
                    // so Timeout is short and there are no retries
                    pingNode(*node, discoveryTimeoutAmount, 0, NULL,
                             "PING received routing table for nearest node",
                             NULL, -1, UDP_TRANSPORT); //TODO
                    discoveryModeProbedNodes++;
                }
            }
            depth--;
        }

        EV << "    received routing table, waiting for pings"
           << endl;

        if (discoveryTimeout->isScheduled()) cancelEvent(discoveryTimeout);
        scheduleAt(simTime() + discoveryTimeoutAmount, discoveryTimeout);
    }
}


bool Pastry::recursiveRoutingHook(const TransportAddress& dest,
                                  BaseRouteMessage* msg)
{
    if (dest == thisNode) {
        return true;
    }

    PastryJoinCall* call =
            dynamic_cast<PastryJoinCall*>(msg->getEncapsulatedPacket());

        if (call && call->getSrcNode() != thisNode) {
            RECORD_STATS(joinSeen++;
                         joinBytesSeen += call->getByteLength());
            // remove node from state if it is rejoining
            handleFailedNode(call->getSrcNode());

            PastryStateMessage* stateMsg =
                createStateMessage((minimalJoinState ?
                                    PASTRY_STATE_MINJOIN :
                                    PASTRY_STATE_JOIN),
                                   -1,
                                   check_and_cast<OverlayCtrlInfo*>(msg->getControlInfo())->getHopCount(),
                                   false);
            RECORD_STATS(stateSent++;
                         stateBytesSent += stateMsg->getByteLength());
            sendMessageToUDP(call->getSrcNode(), stateMsg);
        }

    // forward now:
    return true;
}


void Pastry::iterativeJoinHook(BaseOverlayMessage* msg, bool incrHopCount)
{
    PastryFindNodeExtData* findNodeExt = NULL;
    if (msg && msg->hasObject("findNodeExt")) {
        findNodeExt =
            check_and_cast<PastryFindNodeExtData*>(msg->
                    getObject("findNodeExt"));
    }
    // Send state tables on any JOIN message we see:
    if (findNodeExt) {
        const TransportAddress& stateRecipient =
            findNodeExt->getSendStateTo();
        if (!stateRecipient.isUnspecified()) {
            RECORD_STATS(joinSeen++);
            PastryStateMessage* stateMsg =
                createStateMessage((minimalJoinState ?
                                    PASTRY_STATE_MINJOIN :
                                    PASTRY_STATE_JOIN),
                                   -1,
                                   findNodeExt->getJoinHopCount(),
                                   false);
            RECORD_STATS(stateSent++;
                         stateBytesSent += stateMsg->getByteLength());
            sendMessageToUDP(stateRecipient, stateMsg);
        }
        if (incrHopCount) {
            findNodeExt->setJoinHopCount(findNodeExt->getJoinHopCount() + 1);
        }
    }
}


void Pastry::doJoinUpdate(void)
{
    // send "update" state message to all nodes who sent us their state
    // during INIT, remove these from notifyList so they don't get our
    // state twice
    std::vector<TransportAddress>::iterator nListPos;
    if (!stReceived.empty()) {
        for (std::vector<PastryStateMsgHandle>::iterator it =
                 stReceived.begin(); it != stReceived.end(); ++it) {
            PastryStateMessage* stateMsg =
                 createStateMessage(PASTRY_STATE_UPDATE,
                                    it->msg->getTimestamp());
            RECORD_STATS(stateSent++;
                         stateBytesSent += stateMsg->getByteLength());
            sendMessageToUDP(it->msg->getSender(), stateMsg);

            nListPos = find(notifyList.begin(), notifyList.end(),
                            it->msg->getSender());
            if (nListPos != notifyList.end()) {
                notifyList.erase(nListPos);
            }
            delete it->msg;
            delete it->prox;
        }
        stReceived.clear();
    }

    // send a normal STATE message to all remaining known nodes
    for (std::vector<TransportAddress>::iterator it =
             notifyList.begin(); it != notifyList.end(); it++) {
        if (*it != thisNode) {
            PastryStateMessage* stateMsg =
                createStateMessage(PASTRY_STATE_JOINUPDATE);
            RECORD_STATS(stateSent++;
                         stateBytesSent += stateMsg->getByteLength());
            sendMessageToUDP(*it, stateMsg);
        }
    }
    notifyList.clear();

    updateTooltip();
}

void Pastry::doSecondStage(void)
{
    getParentModule()->getParentModule()->bubble("entering SECOND STAGE");

    // probe nodes in local state
    if (leafSet->isValid()) {
        PastryStateMessage* stateMsg = createStateMessage();

        PastryStateMsgHandle handle(stateMsg);

        if (!stateCache.msg) {
            stateCache = handle;
            processState();
        } else {
            stateCacheQueue.push(handle);
            if (stateCacheQueue.size() > 15) {
                delete stateCacheQueue.front().msg;
                stateCacheQueue.pop();
                EV << "[Pastry::doSecondStage() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    stateCacheQueue full -> pop()" << endl;
            }
            if (proximityNeighborSelection) {
                prePing(stateMsg);
            }
        }
    }

    // "second stage" for locality:
    notifyList.clear();
    routingTable->dumpToVector(notifyList);
    neighborhoodSet->dumpToVector(notifyList);
    sort(notifyList.begin(), notifyList.end());
    notifyList.erase(unique(notifyList.begin(), notifyList.end()),
                     notifyList.end());
    for (std::vector<TransportAddress>::iterator it = notifyList.begin();
         it != notifyList.end(); it++) {
        if (*it == thisNode) continue;

        EV << "[Pastry::doSecondStage() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    second stage: requesting state from " << *it
           << endl;

        RequestStateCall* call =
            new RequestStateCall("REQUEST STATE Call");
        call->setBitLength(PASTRYREQUESTREPAIRCALL_L(call));
        RECORD_STATS(stateReqSent++;
                     stateReqBytesSent += call->getByteLength());
        sendUdpRpcCall(*it, call);
    }
    notifyList.clear();
}


void Pastry::doRoutingTableMaintenance()
{
    for (int i = 0; i < routingTable->getLastRow(); i++) {
        const TransportAddress& ask4row = routingTable->getRandomNode(i);

        assert(!dynamic_cast<const NodeHandle&>(ask4row).getKey().isUnspecified());

        if ((!ask4row.isUnspecified()) && (ask4row != thisNode)) {
            RequestRoutingRowCall* call =
                new RequestRoutingRowCall("REQUEST ROUTING ROW Call");
            call->setStatType(MAINTENANCE_STAT);
            call->setRow(i + 1);
            call->setBitLength(PASTRYREQUESTROUTINGROWCALL_L(call));
            RECORD_STATS(routingTableRowReqSent++;
            routingTableRowReqBytesSent += call->getByteLength());
            sendUdpRpcCall(ask4row, call);
        } else {
            EV << "[Pastry::doRoutingTableMaintenance() @ "
               << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    could not send Message to Node in Row" << i
               << endl;
        }
    }
}


bool Pastry::handleFailedNode(const TransportAddress& failed)
{
    if (state != READY) return false;

    bool wasValid = leafSet->isValid();

    if (failed.isUnspecified())
        opp_error("Pastry::handleFailedNode(): failed is unspecified!");

    const TransportAddress& lsAsk = leafSet->failedNode(failed);
    const TransportAddress& rtAsk = routingTable->failedNode(failed);
    neighborhoodSet->failedNode(failed);

    if (!lsAsk.isUnspecified()) {
        newLeafs();
        if (sendStateAtLeafsetRepair) {
            RequestRepairCall* call =
                new RequestRepairCall("REQUEST REPAIR Call");
            call->setBitLength(PASTRYREQUESTREPAIRCALL_L(call));
            RECORD_STATS(repairReqSent++;
                         repairReqBytesSent += call->getByteLength());
            sendUdpRpcCall(lsAsk, call);
        } else {
            RequestLeafSetCall* call =
                new RequestLeafSetCall("REQUEST LEAFSET Call");
            call->setBitLength(PASTRYREQUESTLEAFSETCALL_L(call));
            RECORD_STATS(leafsetReqSent++;
                         leafsetReqBytesSent += call->getByteLength());
            sendUdpRpcCall(lsAsk, call);
        }
    }
    if (!rtAsk.isUnspecified() && (lsAsk.isUnspecified() || lsAsk != rtAsk)) {
        RequestRepairCall* call =
            new RequestRepairCall("REQUEST REPAIR Call");
        call->setBitLength(PASTRYREQUESTREPAIRCALL_L(call));
        RECORD_STATS(repairReqSent++; repairReqBytesSent += call->getByteLength());
        sendUdpRpcCall(rtAsk, call);
    }

    if (wasValid && lsAsk.isUnspecified() && (! leafSet->isValid())) {
        EV << "[Pastry::handleFailedNode() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    lost connection to the network, trying to re-join."
           << endl;

        join();
        return false;
    }

    return true;
}


void Pastry::checkProxCache(void)
{
    EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]"
       << endl;

    // no cached STATE message?
    assert(stateCache.msg || !stateCache.prox);
    if (!stateCache.msg) {
        return;
    }

    // no entries in stateCache.prox?
    if (stateCache.prox->pr_rt.empty() &&
        stateCache.prox->pr_ls.empty() &&
        stateCache.prox->pr_ns.empty())
        throw cRuntimeError("ERROR in Pastry: stateCache.prox empty!");

    // some entries not yet determined?
    if ((find(stateCache.prox->pr_rt.begin(), stateCache.prox->pr_rt.end(),
        PASTRY_PROX_PENDING) != stateCache.prox->pr_rt.end()) ||
        (find(stateCache.prox->pr_ls.begin(), stateCache.prox->pr_ls.end(),
         PASTRY_PROX_PENDING) != stateCache.prox->pr_ls.end()) ||
        (find(stateCache.prox->pr_ns.begin(), stateCache.prox->pr_ns.end(),
         PASTRY_PROX_PENDING) != stateCache.prox->pr_ns.end())) {

        return;
    }

    EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]\n"
       << "    all proximities for current STATE message from "
       << stateCache.msg->getSender().getIp()
       << " collected!"
       << endl;

    simtime_t now = simTime();

    if (state == JOIN) {
        // save pointer to proximity vectors (it is NULL until now):
        stReceivedPos->prox = stateCache.prox;

        // collected proximities for all STATE messages?
        if (++stReceivedPos == stReceived.end()) {
            EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    proximities for all STATE messages collected!"
               << endl;
            stateCache.msg = NULL;
            stateCache.prox = NULL;
            if (debugOutput) {
                EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    [JOIN] starting to build own state from "
                   << stReceived.size() << " received state messages..."
                   << endl;
            }
            if (mergeState()) {
                changeState(READY);
                EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    changeState(READY) called"
                   << endl;
            } else {
                EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    Error initializing while joining! Restarting ..."
                   << endl;
                joinOverlay();
            }

        } else {
            EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    NOT all proximities for all STATE messages collected!"
               << endl;

            // process next state message in vector:
            if (stReceivedPos->msg == NULL)
                throw cRuntimeError("stReceivedPos->msg = NULL");
            stateCache = *stReceivedPos;
            if (stateCache.msg == NULL)
                throw cRuntimeError("msg = NULL");
            processState();
        }
    } else {
        // state == READY
        if (stateCache.msg->getPastryStateMsgType() == PASTRY_STATE_REPAIR) {
            // try to repair routingtable based on repair message:
            const TransportAddress& askRt =
                routingTable->repair(stateCache.msg, stateCache.prox);
            if (! askRt.isUnspecified()) {
                RequestRepairCall* call =
                    new RequestRepairCall("REQUEST REPAIR Call");
                call->setBitLength(PASTRYREQUESTREPAIRCALL_L(call));
                RECORD_STATS(repairReqSent++; repairReqBytesSent += call->getByteLength());
                sendUdpRpcCall(askRt, call);
            }

            // while not really known, it's safe to assume that a repair
            // message changed our state:
            lastStateChange = now;
        } else {
            if (stateCache.outdatedUpdate) {
                // send another STATE message on outdated state update:
                updateCounter++;
                sendStateDelayed(stateCache.msg->getSender());
            } else {
                // merge info in own state tables
                // except leafset (was already handled in handleStateMessage)
                if (neighborhoodSet->mergeState(stateCache.msg, stateCache.prox))
                    lastStateChange = now;
                EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    Merging nodes into routing table"
                   << endl;
                if (routingTable->mergeState(stateCache.msg, stateCache.prox)) {
                    lastStateChange = now;
                    EV << "[Pastry::checkProxCache() @ " << thisNode.getIp()
                       << " (" << thisNode.getKey().toString(16) << ")]\n"
                       << "    Merged nodes into routing table"
                       << endl;
                }
            }
        }
        updateTooltip();

        endProcessingState();
    }
}

void Pastry::endProcessingState(void)
{
    // if state message was not an update, send one back:
    if (stateCache.msg &&
        stateCache.msg->getPastryStateMsgType() != PASTRY_STATE_UPDATE &&
        (alwaysSendUpdate || lastStateChange == simTime()) &&
        !stateCache.msg->getSender().isUnspecified() &&
        thisNode != stateCache.msg->getSender()) {//hack
        PastryStateMessage* stateMsg =
            createStateMessage(PASTRY_STATE_UPDATE,
                               stateCache.msg->getTimestamp());
        RECORD_STATS(stateSent++;
                     stateBytesSent += stateMsg->getByteLength());

        sendMessageToUDP(stateCache.msg->getSender(), stateMsg);
    }

    delete stateCache.msg;
    stateCache.msg = NULL;
    delete stateCache.prox;
    stateCache.prox = NULL;

    // process next queued message:
    if (! stateCacheQueue.empty()) {
        stateCache = stateCacheQueue.front();
        stateCacheQueue.pop();
        processState();
    }
}


bool Pastry::mergeState(void)
{
    bool ret = true;

    if (state == JOIN) {
        // building initial state
        if (debugOutput) {
            EV << "[Pastry::mergeState() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    [JOIN] starting to build own state from "
               << stReceived.size() << " received state messages..."
               << endl;
        }
        if (stateCache.msg &&
            stateCache.msg->getNeighborhoodSetArraySize() > 0) {
            if (debugOutput) {
                EV << "[Pastry::mergeState() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    [JOIN] initializing NeighborhoodSet from "
                   << stReceived.front().msg->getRow() << ". hop"
                   << endl;
            }
            if (!neighborhoodSet->mergeState(stReceived.front().msg,
                                             stReceived.front().prox )) {
                EV << "[Pastry::mergeState() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    Error initializing own neighborhoodSet"
                   << " while joining! Restarting ..."
                   << endl;
                ret = false;
            }
        }
        if (debugOutput) {
            EV << "[Pastry::mergeState() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    [JOIN] initializing LeafSet from "
               << stReceived.back().msg->getRow() << ". hop"
               << endl;
        }

        if (!leafSet->mergeState(stReceived.back().msg,
                                 stReceived.back().prox )) {
            EV << "[Pastry::mergeState() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    Error initializing own leafSet while joining!"
               << " Restarting ..."
               << endl;

            ret = false;
        } else {
            newLeafs();
        }
        if (debugOutput) {
            EV << "[Pastry::mergeState() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    [JOIN] initializing RoutingTable from all hops"
               << endl;
        }

        assert(!stateCache.msg ||
               stateCache.msg->getRoutingTableArraySize() > 0);

        if (!routingTable->initStateFromHandleVector(stReceived)) {
            EV << "[Pastry::mergeState() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    Error initializing own routingTable while joining!"
               << " Restarting ..."
               << endl;

            ret = false;
        }
    } else if (state == READY) {
        // merging single state (stateCache.msg)
        if ((stateCache.msg->getNeighborhoodSetArraySize() > 0) &&
            (!neighborhoodSet->mergeState(stateCache.msg, NULL))) {
            ret = false;
        }
        if (!leafSet->mergeState(stateCache.msg, NULL)) {
            ret = false;
        } else {
            newLeafs();
        }
        if (!routingTable->mergeState(stateCache.msg, NULL)) {
            ret = false;
        }
    }

    if (ret) lastStateChange = simTime();
    return ret;
}


void Pastry::handleStateMessage(PastryStateMessage* msg)
{
    if (debugOutput) {
        EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    new STATE message to process "
           << static_cast<void*>(msg) << " in state " <<
            ((state == READY)?"READY":((state == JOIN)?"JOIN":"INIT"))
           << endl;
        if (state == JOIN) {
            EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    ***   own joinHopCount:  " << joinHopCount << endl
               << "    ***   already received:  " << stReceived.size() << endl
               << "    ***   last-hop flag:     "
               << (msg->getLastHop() ? "true" : "false") << endl
               << "    ***   msg joinHopCount:  "
               << msg->getRow() << endl;
        }
    }
    if (state == INIT || state == DISCOVERY) {
        EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    can't handle state messages until at least reaching JOIN state."
           << endl;
        delete msg;
        return;
    }

    PastryStateMsgHandle handle(msg);

    // in JOIN state, store all received state Messages, need them later:
    if (state == JOIN) {
        if (!(msg->getPastryStateMsgType() &
              (PASTRY_STATE_JOIN | PASTRY_STATE_MINJOIN))) {
            delete msg;
            return;
        }

        if (joinHopCount && stReceived.size() == joinHopCount) {
            EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    Warning: dropping state message received after "
               << "all needed state messages were collected in JOIN state."
               << endl;
            delete msg;
            return;
        }

        stReceived.push_back(handle);
        if (pingBeforeSecondStage && proximityNeighborSelection) prePing(msg);

        if (msg->getLastHop()) {
            if (joinHopCount) {
                EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    Error: received a second `last' state message! Restarting ..."
                   << endl;

                joinOverlay();
                return;
            }

            joinHopCount = msg->getRow();

            if (stReceived.size() < joinHopCount) {
                // some states still missing:
                cancelEvent(readyWait);
                scheduleAt(simTime() + readyWaitAmount, readyWait);
            }
        }

        if (joinHopCount) {
            if (stReceived.size() > joinHopCount) {
                EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    Error: too many state messages received in JOIN state! ("
                   << stReceived.size() << " > " << joinHopCount << ") Restarting ..."
                   << endl;

                joinOverlay();
                return;
            }
            if (stReceived.size() == joinHopCount) {
                // all state messages are here, sort by hopcount:
                sort(stReceived.begin(), stReceived.end(),
                     stateMsgIsSmaller);

                // start pinging the nodes found in the first state message:
                stReceivedPos = stReceived.begin();
                stateCache = *stReceivedPos;
                EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    have all STATE messages, now pinging nodes."
                   << endl;
                if (pingBeforeSecondStage && proximityNeighborSelection) {
                    pingNodes();
                } else {
                    mergeState();
                    stateCache.msg = NULL;
                    changeState(READY);

                    EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
                       << " (" << thisNode.getKey().toString(16) << ")]\n"
                       << "    changeState(READY) called"
                       << endl;
                }

                // cancel timeout:
                if (readyWait->isScheduled()) cancelEvent(readyWait);
            } else {
                //TODO occasionally, here we got a wrong hop count in
                // iterative mode due to more than one it. lookup during join
                // procedure
                EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    Still need some STATE messages."
                   << endl;
            }

        }
        return;
    }

    if (debugOutput) {
        EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    handling STATE message"
           << endl;
        EV << "        type: " << ((msg->getPastryStateMsgType()
                                    == PASTRY_STATE_UPDATE) ? "update"
                                                            :"standard")
           << endl;
        if (msg->getPastryStateMsgType() == PASTRY_STATE_UPDATE) {
            EV << "        msg timestamp:      " <<
                msg->getTimestamp() << endl;
            EV << "        last state change:  " <<
                lastStateChange << endl;
        }
    }

    if (((msg->getPastryStateMsgType() == PASTRY_STATE_UPDATE))
            && (msg->getTimestamp() <= lastStateChange)) {
        // if we received an update based on our outdated state,
        // mark handle for retrying later:
        EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    outdated state from " << msg->getSender()
           << endl;
        handle.outdatedUpdate = true;
    }

    // determine aliveTable to prevent leafSet from merging nodes that are
    // known to be dead:
    determineAliveTable(msg);

    if (msg->getPastryStateMsgType() == PASTRY_STATE_REPAIR) {
        // try to repair leafset based on repair message right now
        const TransportAddress& askLs = leafSet->repair(msg, &aliveTable);
        if (! askLs.isUnspecified()) {
            //sendRequest(askLs, PASTRY_REQ_REPAIR);
            RequestRepairCall* call =
                new RequestRepairCall("REQUEST REPAIR Call");
            call->setBitLength(PASTRYREQUESTREPAIRCALL_L(call));
            RECORD_STATS(repairReqSent++; repairReqBytesSent += call->getByteLength());
            sendUdpRpcCall(askLs, call);
        }

        // while not really known, it's safe to assume that a repair
        // message changed our state:
        lastStateChange = simTime();
        newLeafs();
    } else if (leafSet->mergeState(msg, &aliveTable)) {
        // merged state into leafset right now
        lastStateChange = simTime();
        newLeafs();
        updateTooltip();
    }
    // in READY state, only ping nodes to get proximity metric:
    if (!stateCache.msg) {
        // no state message is processed right now, start immediately:
        assert(stateCache.prox == NULL);
        stateCache = handle;
        processState();
    } else {
        if (proximityNeighborSelection && (pingBeforeSecondStage ||
            msg->getPastryStateMsgType() == PASTRY_STATE_STD)) {
            // enqueue message for later processing:
            stateCacheQueue.push(handle);
            if (stateCacheQueue.size() > 15) {
                delete stateCacheQueue.front().msg;
                stateCacheQueue.pop();
                EV << "[Pastry::handleStateMessage() @ " << thisNode.getIp()
                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                   << "    stateCacheQueue full -> pop()" << endl;
            }
            prePing(msg);
        } else {
            bool temp = true;
            if (!neighborhoodSet->mergeState(msg, NULL)) {
                temp = false;
            }
            if (!leafSet->mergeState(msg, NULL)) {
                temp = false;
            } else {
                newLeafs();
            }
            if (!routingTable->mergeState(msg, NULL)) {
                temp = false;
            }
            if (temp) lastStateChange = simTime();
            delete msg;
        }
    }
}


void Pastry::processState(void)
{
    EV << "[Pastry::processState() @ " << thisNode.getIp()
       << " (" << thisNode.getKey().toString(16) << ")]\n"
       << "    new \""
       << std::string(cEnum::find("PastryStateMsgType")
              ->getStringFor(stateCache.msg->getPastryStateMsgType())).erase(0, 13)
       << "\" STATE message " << static_cast<void*>(stateCache.msg)
       << " from " << stateCache.msg->getSender().getIp() << " to process "
       << endl;

    if (proximityNeighborSelection && (pingBeforeSecondStage ||
        stateCache.msg->getPastryStateMsgType() == PASTRY_STATE_STD)) {
        pingNodes();
    } else {
        mergeState();
        endProcessingState();
    }
}
