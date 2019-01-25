/*
 * DiscoveryMode.cc
 *
 *  Created on: Sep 27, 2010
 *      Author: heep
 */

#include <BasePastry.h>

#include <DiscoveryMode_m.h>

#include "DiscoveryMode.h"



void DiscoveryMode::init(NeighborCache* neighborCache)
{
    this->neighborCache = neighborCache;
    nearNodeRtt = MAXTIME;
    nearNodeImproved = false;
    firstRtt = SIMTIME_DBL(MAXTIME);
    improvement = 0;
    maxIndex = -1;
    finished = false;

    basePastry = NULL;//dynamic_cast<BasePastry*>(neighborCache->overlay);

    numCloseNodes = neighborCache->par("discoveryModeNumCloseNodes");
    numSpreadedNodes = neighborCache->par("discoveryModeNumSpreadedNodes");
    maxSteps = neighborCache->par("discoveryModeMaxSteps");
    spreadedSteps = (int)neighborCache->par("discoveryModeSpreadedSteps");
}

void DiscoveryMode::start(const TransportAddress& bootstrapNode)
{
    step = 0;
    queries = 0;

    if (bootstrapNode.isUnspecified()) {
        stop();
    } else {

        EV << "[DiscoveryMode::start() @ " << neighborCache->getThisNode().getIp()
           << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
           << "    starting Discovery Mode (first asking "
           << bootstrapNode.getIp() << " for "
           << (basePastry ? "leafSet" : "spreaded nodes)") << ")"
           << endl;

        // 1. ask bootstrap node for other nodes and his coordinates
        DiscoveryCall* discoveryCall = new DiscoveryCall("DiscoveryCall");
        discoveryCall->setNodesType(SPREADED_NODES);
        discoveryCall->setNumberOfNodes(numSpreadedNodes);

        neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                        bootstrapNode, discoveryCall, NULL,
                                        NO_OVERLAY_ROUTING, -1, 0, -1, this);
        ++queries;

        // 2. probe other nodes and optionally ask them for more nodes
        //    (try to get close as well as distant nodes)
        // sendReadyMessage();
    }
}

void DiscoveryMode::stop()
{
    improvement = firstRtt - SIMTIME_DBL(nearNodeRtt);

    EV << "[DiscoveryMode::stop() @ " << neighborCache->getThisNode().getIp()
       << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
       << "    stopping Discovery mode with "
       << nearNode.getIp() << " as closest node, which is "
       << improvement << " s closer (RTT) than the original bootstrap node."
       << endl;

    neighborCache->getParentModule()->bubble("Discovery Mode finished!");
    finished = true;

    //RECORD_STATS(
    if (neighborCache->globalStatistics->isMeasuring()) {
        neighborCache->globalStatistics->addStdDev("NeighborCache: Discovery Mode Improvement",
                                                   getImprovement());
        if (dynamic_cast<const Vivaldi*>(&(neighborCache->getNcsAccess()))) {
            neighborCache->globalStatistics->addStdDev("NeighborCache: Vivaldi-Error after Discovery Mode",
                                                       static_cast<const Vivaldi&>(neighborCache->getNcsAccess()).getOwnError());
        }
    }
    //);
    const SVivaldi& svivaldi = dynamic_cast<const SVivaldi&>(neighborCache->getNcsAccess());
    std::cout << "step: " << step << ", e: " << svivaldi.getOwnError() << ", l: " << svivaldi.getLoss() << std::endl;

    const_cast<SVivaldi&>(svivaldi).stopAdaptation();
    neighborCache->prepareOverlay();
}

bool DiscoveryMode::handleRpcCall(BaseCallMessage* msg)
{
    if (dynamic_cast<DiscoveryCall*>(msg)) {
        DiscoveryCall* discoveryCall = static_cast<DiscoveryCall*>(msg);

        std::vector<TransportAddress>* temp;

        DiscoveryResponse* discoveryResponse =
            new DiscoveryResponse("DiscoveryResponse");

        if (discoveryCall->getNodesType() == SPREADED_NODES) {
            if (basePastry) {
                temp = basePastry->getLeafSet();
            } else {
                temp = neighborCache->getSpreadedNodes(discoveryCall->getNumberOfNodes());
            }
        } else {
            if (basePastry) {
                if (discoveryCall->getIndex() == 0) {
                    discoveryResponse->setMaxIndex(basePastry->getRTLastRow() - 1);
                }
                int16_t rowIndex = basePastry->getRTLastRow()
                                - discoveryCall->getIndex();
                if (rowIndex < 1) rowIndex = 1;
                temp = basePastry->getRTRow(rowIndex);
            } else {
                temp = neighborCache->getClosestNodes(discoveryCall->getNumberOfNodes());
            }
        }

        discoveryResponse->setNodesArraySize(temp->size());
        for (uint16_t i = 0; i < temp->size(); ++i) {
            discoveryResponse->setNodes(i, (*temp)[i]);
        }
        delete temp;

        neighborCache->sendRpcResponse(discoveryCall, discoveryResponse);

        return true;
    }
    return false;
}

void DiscoveryMode::handleRpcResponse(BaseResponseMessage* msg,
                                      cPolymorphic* context,
                                      int rpcId, simtime_t rtt)
{
    if (dynamic_cast<DiscoveryResponse*>(msg)) {
        DiscoveryResponse* discoveryResponse =
            static_cast<DiscoveryResponse*>(msg);

        --queries;

        EV << "[DiscoveryMode::handleRpcResponse() @ " << neighborCache->getThisNode().getIp()
           << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
           << "    receiving DiscoveryResponse from "
           << msg->getSrcNode().getIp()
           << " (step = " << (int)step << ")"
           << endl;


        if (nearNodeRtt > rtt) {
            if (nearNode.isUnspecified() || nearNode != discoveryResponse->getSrcNode()) {
                EV << "    , which is now the closest known neighbor."
                   << endl;
                nearNodeImproved = true;
                nearNode = discoveryResponse->getSrcNode();
            }
            nearNodeRtt = rtt;

            if (step == 0) firstRtt = SIMTIME_DBL(rtt);
        }

        const TransportAddress* node;

        for (uint16_t i = 0; i < discoveryResponse->getNodesArraySize(); i++) {
            node = &(discoveryResponse->getNodes(i));
            // unspecified nodes not considered
            if (!(node->isUnspecified() || *node == neighborCache->getThisNode())) {
                Prox temp = neighborCache->getProx(*node, NEIGHBORCACHE_EXACT,
                                                   -1, this, NULL);
                if (temp == Prox::PROX_TIMEOUT
                    || temp == Prox::PROX_UNKNOWN
                    || temp == Prox::PROX_WAITING) {
                    ++queries;
                } else if (nearNodeRtt > temp.proximity) {
                    nearNode = *node;
                    nearNodeRtt = temp.proximity;
                    nearNodeImproved = true;
                }


            }
        }
        int16_t ind = discoveryResponse->getMaxIndex();
        if (ind != -1) maxIndex = ind;

        EV << "[DiscoveryMode::handleRpcResponse() @ " << neighborCache->getThisNode().getIp()
                 << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
                 << "    maxIndex of " << msg->getSrcNode().getIp()
                 << " is " << (int)maxIndex << "."
                 << endl;

        //if (--spreadedSteps >= 0) {
            sendNewRequest(SPREADED_NODES, numSpreadedNodes);
        //} else {
            //sendNewRequest(CLOSE_NODES, numCloseNodes);
        //}
    }
}

void DiscoveryMode::handleRpcTimeout(BaseCallMessage* msg,
                                     const TransportAddress& dest,
                                     cPolymorphic* context, int rpcId,
                                     const OverlayKey& destKey)
{
    --queries;
}


void DiscoveryMode::proxCallback(const TransportAddress& node, int rpcId,
                                 cPolymorphic *contextPointer, Prox prox)
{
    --queries;

    if (prox != Prox::PROX_TIMEOUT  && prox != Prox::PROX_SELF &&
        nearNodeRtt > prox.proximity) {
        nearNode = node;
        nearNodeRtt = prox.proximity;
        nearNodeImproved = true;
    }

    sendNewRequest(SPREADED_NODES, numSpreadedNodes);
    //sendNewRequest(CLOSE_NODES, numCloseNodes);
}

void DiscoveryMode::sendNewRequest(DiscoveryNodesType type, uint8_t numNodes)
{
    const SVivaldi& svivaldi = dynamic_cast<const SVivaldi&>(neighborCache->getNcsAccess());

    if (queries == 0) {
        if (/*(step < maxSteps || maxSteps == -1) && */svivaldi.getOwnError() > 0.02 && svivaldi.getLoss() < 0.95
            /*(nearNodeImproved || step <= maxIndex)*/) {
            //std::cout << (int)step << " " << (int)maxIndex << std::endl;
            ++step;
            nearNodeImproved = false;

            DiscoveryCall* discoveryCall1 = new DiscoveryCall("DiscoveryCall");
            DiscoveryCall* discoveryCall2 = new DiscoveryCall("DiscoveryCall");
            discoveryCall1->setNodesType(CLOSE_NODES);
            discoveryCall2->setNodesType(SPREADED_NODES);
            discoveryCall1->setNumberOfNodes(numCloseNodes);
            discoveryCall2->setNumberOfNodes(numSpreadedNodes);
            //discoveryCall->setIndex(step - 1);

            //std::vector<TransportAddress>* temp =
            //    neighborCache->getClosestNodes(1);
            //if (temp->size() < 1) {
            //    throw cRuntimeError("no node for discovery available");
            //}

            EV << "[DiscoveryMode::sendNewRequest() @ " << neighborCache->getThisNode().getIp()
               << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
               << "    sending new DiscoveryCall to "
               << nearNode.getIp() << " for "
               << (basePastry ? "routing table row" : "close nodes")
               << ". (step = " << (int)step << ")"
               << endl;

            neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                            nearNode/*(*temp)[0]*/, discoveryCall1, NULL,
                                            NO_OVERLAY_ROUTING, -1, 0, -1,
                                            this);

            neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                            nearNode/*(*temp)[0]*/, discoveryCall2, NULL,
                                            NO_OVERLAY_ROUTING, -1, 0, -1,
                                            this);
            ++queries;
            //delete temp;
        } else {
            stop();
        }
    }
}

//DISCOVERY receive response
/*if (state == DISCOVERY) {
        EV << "[Pastry::pingResponse() @ " << thisNode.getAddress()
           << " (" << thisNode.getKey().toString(16) << ")]\n"
           << "    Pong (or Ping-context from NeighborCache) received (from "
           << pingResponse->getSrcNode().getAddress() << ") in DISCOVERY mode"
           << endl;

        if (nearNodeRtt > rtt) {
            nearNode = pingResponse->getSrcNode();
            nearNodeRtt = rtt;
            nearNodeImproved = true;
        }
    }

    if (state == DISCOVERY) {
            uint32_t lsSize = lmsg->getLeafSetArraySize();
            const NodeHandle* node;
            pingedNodes = 0;

            for (uint32_t i = 0; i < lsSize; i++) {
                node = &(lmsg->getLeafSet(i));
                // unspecified nodes not considered
                if ( !(node->isUnspecified()) ) {
                    pingNode(*node, discoveryTimeoutAmount, 0,
                             NULL, "PING received leafs for nearest node",
                             NULL, -1, UDP_TRANSPORT);//TODO
                    pingedNodes++;
               }
            }

            EV << "[Pastry::handleUDPMessage() @ " << thisNode.getAddress()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    received leafset, waiting for pings"
              << endl;

            if (discoveryTimeout->isScheduled()) cancelEvent(discoveryTimeout);
            scheduleAt(simTime() + discoveryTimeoutAmount, discoveryTimeout);
            delete lmsg;
        }


        if (state == DISCOVERY) {
            uint32_t nodesPerRow = rtmsg->getRoutingTableArraySize();
            const NodeHandle* node;
            if (depth == -1) {
                depth = rtmsg->getRow();
            }
            pingedNodes = 0;
            nearNodeImproved = false;

            if (depth > 0) {
                for (uint32_t i = 0; i < nodesPerRow; i++) {
                    node = &(rtmsg->getRoutingTable(i));
                    // unspecified nodes not considered
                    if ( !(node->isUnspecified()) ) {
                        // we look for best connection here, so Timeout is short and there are no retries
                        pingNode(*node, discoveryTimeoutAmount, 0, NULL,
                                 "PING received routing table for nearest node",
                                 NULL, -1, UDP_TRANSPORT); //TODO
                        pingedNodes++;
                    }
                }
                depth--;
            }
    */
