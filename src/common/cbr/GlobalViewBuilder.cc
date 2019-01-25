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
 * @GlobalViewBuilder.cc
 * @author Daniel Lienert
 */

#include <iostream>
#include <fstream>

#include <AbstractSendStrategy.h>
#include <UnderlayConfiguratorAccess.h>
#include <CoordBasedRoutingAccess.h>

#include "GlobalViewBuilder.h"

void GlobalViewBuilder::initializeViewBuilder(NeighborCache* neighborCache, BaseOverlay* overlay) {

    this->neighborCache = neighborCache;
    this->overlay = overlay;
    this->treeManager = neighborCache->getTreeManager();

    meanCoordSendInterval = neighborCache->par("gvbCoordSendInterval");
    deviation = meanCoordSendInterval / 10;

    capReady = false;
    oldCcdSize = 0;
    onlyAcceptCompleteCCD = neighborCache->par("gvbOnlyAcceptCompleteCCD");

    // initialize the send Strategy
    activeStrategy = this->neighborCache->par("gvbSendStrategy").stdstringValue();
    sendStrategy = SendStrategyFactory::getSendStrategyInstance(activeStrategy);
    sendStrategy->initialize(this);

    coordSendTimer = new cMessage("coordSendTimer");
    spreadCapTimer = new cMessage("spreadCapTimer");

    //start();
}



void GlobalViewBuilder::start()
{
    neighborCache->cancelEvent(coordSendTimer);
    neighborCache->scheduleAt(simTime() +
                              truncnormal(meanCoordSendInterval / 2.0, //TODO
                                          deviation),
                              coordSendTimer);
}


void GlobalViewBuilder::cleanup()
{
    // TODO AbstractSendStrategy::initializeStrategy()
    sendStrategy->setThisNode(neighborCache->overlay->getThisNode());
    sendStrategy->setMyCoordinates(neighborCache->getNcsAccess().getOwnNcsInfo());
    sendStrategy->cleanUpCoordData(treeManager->getChildNodes());
}


bool GlobalViewBuilder::handleRpcCall(BaseCallMessage* msg)
{
    EV << "|DD|> GlobalViewBuilder::handleRpcCall (" << ") <||" << endl;


    RPC_SWITCH_START( msg );
    RPC_DELEGATE( GlobalViewBuilder, handleCoordinateRpcCall);
    RPC_DELEGATE( CapReq, handleCapReqRpcCall);
    RPC_SWITCH_END( );

    //delete(msg);

    return RPC_HANDLED;
}

void GlobalViewBuilder::handleCoordinateRpcCall(GlobalViewBuilderCall* msg)
{
    if (overlay->getThisNode() == msg->getSrcNode()) {
        //std::cout << treeManager->getCurrentTreeLevel() << " !!!" << std::endl;
        delete msg;
        return;
    }
    // CAP received
    if (dynamic_cast<AreaDataCall*>(msg)) {
        AreaDataCall* temp = static_cast<AreaDataCall*>(msg);
        //treeManager->isParent(msg->getSrcNode())
        if (treeManager->getParentNode().isUnspecified() ||
            (msg->getSrcNode() != treeManager->getParentNode())) {
            std::cout << neighborCache->getThisNode().getIp()
                      << ": " << msg->getSrcNode().getIp() << " is not my parent! Deleting AreaDataCall!" << std::endl;
            delete msg;
            return;
        }

        cap = temp->getAreaData().CBRAreaPool;
        //std::vector<double> tempvec =
        //        neighborCache->getNcsAccess().getOwnNcsInfo().getCoords();
        //for (uint8_t i = 0; i< tempvec.size(); ++i) {
        //    std::cout << tempvec[i] << std::endl;
        //}
        //std::cout << overlay->getThisNode().getKey() << " -> "
        //          << CoordBasedRoutingAccess().get()->getNodeId(tempvec, 4, 160, &cap) << "\n" << std::endl;

        neighborCache->getParentModule()->bubble("CAP received!");
        capReady = true;

        AreaDataCall* newAreaCall = new AreaDataCall("AreaDataCall");
        newAreaCall->setAreaData(temp->getAreaData());
        newAreaCall->setBitLength(temp->getBitLength());

        //std::cout << treeManager->getCurrentTreeLevel() << std::endl;
        treeManager->sendMessageToChildren(newAreaCall);
    }
    // coordinates received from child
    else {
        // check if this a child
        if (treeManager->isChild(msg->getSrcNode())) {
             sendStrategy->handleCoordinateRpcCall(msg);
        }  else {
            //TODO do not delete msg if msg was sent to a key
            std::cout << neighborCache->getThisNode().getIp()
                      << ": " << msg->getSrcNode().getIp() //<< " (" << msg->getDestKey()
                      << ") is no child of mine! Deleting Coordinates!" << std::endl;
            delete msg;
            return;
        }
    }

    GlobalViewBuilderResponse* gvbResponse = new GlobalViewBuilderResponse("GlobalViewBuilderResponse");
    gvbResponse->setBitLength(GLOBALVIEWBUILDERRESPONSE_L(gvbResponse));

    neighborCache->sendRpcResponse(msg, gvbResponse);
}

void GlobalViewBuilder::sendCapRequest(const TransportAddress& node)
{
    assert(!node.isUnspecified());
    neighborCache->sendRouteRpcCall(NEIGHBORCACHE_COMP,
                                    node,
                                    new CapReqCall("CapRequestCall"),
                                    NULL, DEFAULT_ROUTING, -1, 0, -1, this);
}
void GlobalViewBuilder::handleRpcResponse(BaseResponseMessage* msg,
                                          cPolymorphic* context,
                                          int rpcId, simtime_t rtt)
{
    RPC_SWITCH_START(msg)
            RPC_ON_RESPONSE(CapReq) {
        /*int bitsPerDigit = neighborCache->overlay->getBitsPerDigit();
        neighborCache->thisNode.setKey(CoordBasedRoutingAccess().get()->getNodeId(neighborCache->getNcsAccess().getOwnNcsInfo().getCoords(),
                                                                                 bitsPerDigit,
                                                                                 OverlayKey::getLength(),
                                                                                 &_CapReqResponse->getAreaData().CBRAreaPool));

        EV << "[GlobalViewBuilder::handleRpcResponse() @ "
                << neighborCache->thisNode.getIp()
                << " (" << neighborCache->thisNode.getKey().toString(16) << ")]"
                << "\n    -> nodeID ( 2): "
                << neighborCache->thisNode.getKey().toString(2)
                << "\n    -> nodeID (16): "
                << neighborCache->thisNode.getKey().toString(16) << endl;

        // returning to BaseOverlay
        neighborCache->overlay->join(neighborCache->thisNode.getKey());*/
        cap = _CapReqResponse->getAreaData().CBRAreaPool;
        //if (cap.size() > 0) //TODO
        capReady = true; //TODO
        neighborCache->prepareOverlay();
    }
    RPC_SWITCH_END( )
}


void GlobalViewBuilder::handleRpcTimeout(BaseCallMessage* msg,
                                         const TransportAddress& dest,
                                         cPolymorphic* context, int rpcId,
                                         const OverlayKey& destKey)
{
    std::cout << "THIS SHOULD NOT HAPPEN!!!" << std::endl;
    assert(false);
}


void GlobalViewBuilder::handleCapReqRpcCall(CapReqCall* call)
{
    AreaDataContainer tmpAreaDataContainer;
    tmpAreaDataContainer.CBRAreaPool = cap;
    CapReqResponse* response = new CapReqResponse();
    response->setAreaData(tmpAreaDataContainer);
    neighborCache->sendRpcResponse(call, response);
}

void GlobalViewBuilder::handleTimerEvent(cMessage* msg)
{
    if (msg == spreadCapTimer) {
        if (treeManager->isRoot()) {
            spreadGlobalView();
        } else {
            //std::cout << "msg == spreadCapTimer && !treeManager->isRoot()" << std::endl;
        }
    } else if (msg == coordSendTimer) {
        //neighborCache->cancelEvent(spreadCapTimer);
        handleCoordSendTimer(msg);
    }
}

void GlobalViewBuilder::handleCoordSendTimer(cMessage* msg)
{
    neighborCache->cancelEvent(msg);
    neighborCache->scheduleAt(simTime() + meanCoordSendInterval,
                              //truncnormal(meanCoordSendInterval, deviation),
                              msg);

    const NodeHandle& thisNode = this->neighborCache->overlay->getThisNode();
    if(thisNode.isUnspecified()) {
        return;
    }

    sendStrategy->setThisNode(neighborCache->overlay->getThisNode());
    sendStrategy->setMyCoordinates(neighborCache->getNcsAccess().getOwnNcsInfo());
    sendStrategy->cleanUpCoordData(treeManager->getChildNodes());

    if (!neighborCache->getTreeManager()->getParentNode().isUnspecified()) {
        std::stringstream nodeState;
        nodeState.str("");
        nodeState << "LVL: ";
        nodeState << treeManager->getCurrentTreeLevel();
        nodeState << ": ";
        nodeState << sendStrategy->getStrategyDataStatus();

        if (!treeManager->isRoot() /*neighborCache->getTreeManager()->getParentNode().getKey() !=
            neighborCache->overlay->getThisNode().getKey()*/) {
            neighborCache->getTreeManager()->sendMessageToParent(sendStrategy->getCoordinateMessage());
        } else {
            // this node is root
            if(checkOverlayReady() == true) {
                EV << "|DD|> GlobalViewBuilder::handleCoordSendTimer ("
                   << "Spread GlobaLViewData" << ") <||" << endl;
                // wait some seconds before spreading the CAP
                neighborCache->cancelEvent(spreadCapTimer);
                neighborCache->scheduleAt(simTime() + 10, spreadCapTimer); //TODO parameter
                //spreadGlobalView();
            }
        }

        //std::cout << treeManager->getCurrentTreeLevel() << "\n"
        //          << sendStrategy->getStrategyDataStatus() << std::endl;

        neighborCache->getParentModule()->getDisplayString().setTagArg("t", 0, nodeState.str().c_str());
    } else {
        std::cout << simTime() << " @ "
                  << neighborCache->getThisNode()
                  << ": I have no parent :-(" << std::endl;
        treeManager->startTreeBuilding();
    }
}


void GlobalViewBuilder::newParent()
{
    //handleCoordSendTimer(coordSendTimer);
}


bool GlobalViewBuilder::checkOverlayReady()
{
    return  !UnderlayConfiguratorAccess().get()->isInInitPhase();
    //return UnderlayConfiguratorAccess().get()->isTransitionTimeFinished() && !UnderlayConfiguratorAccess().get()->isInInitPhase();
}


void GlobalViewBuilder::spreadGlobalView()
{
    CoordsVec ccd = sendStrategy->getGlobalViewData();
    //std::cout << "CCD: " << ccd.size() << std::endl;
    uint32_t tempSize = 0;
    if ((oldCcdSize == 0) ||
        ((ccd.size() < (oldCcdSize * 1.5)) &&
         (ccd.size() > (oldCcdSize * 0.5)))) {
        tempSize = oldCcdSize;
        oldCcdSize = ccd.size();
    }

    CoordBasedRouting* cbr = CoordBasedRoutingAccess().get();

    //special case: only accept complete CCD
    if (onlyAcceptCompleteCCD &&
        ((int)ccd.size() !=
            neighborCache->underlayConfigurator->getOverlayTerminalCount())) {
        EV << "[GlobalViewBuilder::spreadGlobalView() @ "
           << overlay->getThisNode().getIp()
           << " (" << overlay->getThisNode().getKey().toString(16)
           << ")]\n    CAP.size() " << ccd.size()
           << " != numTerminalCount: no spreading!" << endl;
        std::cout << "[GlobalViewBuilder::spreadGlobalView() @ "
                  << overlay->getThisNode().getIp()
                  << " (" << overlay->getThisNode().getKey().toString(16)
                  << ")]\n    CCD.size() " << ccd.size()
                  << " != numTerminalCount: no spreading!" << std::endl;
        return;
    } else if (cbr->changeIdLater() &&
               (simTime() > cbr->getChangeIdStart()) &&
               (simTime() < (cbr->getChangeIdStop() + 3000))) {
        EV << "[GlobalViewBuilder::spreadGlobalView() @ "
           << overlay->getThisNode().getIp()
           << " (" << overlay->getThisNode().getKey().toString(16)
           << ")]\n    CAP.size() " << ccd.size()
           << " in id change phase, no spreading" << endl;
        std::cout << "[GlobalViewBuilder::spreadGlobalView() @ "
                  << overlay->getThisNode().getIp()
                  << " (" << overlay->getThisNode().getKey().toString(16)
                  << ")]\n    CAP.size() " << ccd.size()
                  << " in id change phase, no spreading" << std::endl;
        return;

    } else tempSize = oldCcdSize = ccd.size();

    //std::cout << "tempSize: " << tempSize << std::endl;
    // CCD size check
    if ((ccd.size() > (tempSize * 1.2)) ||
        (ccd.size() < (tempSize * 0.8))) {
        EV << "[GlobalViewBuilder::spreadGlobalView() @ "
           << overlay->getThisNode().getIp()
           << " (" << overlay->getThisNode().getKey().toString(16)
           << ")]\n    CAP.size() is too fluctuant: no spreading!" << endl;
        std::cout << "[GlobalViewBuilder::spreadGlobalView() @ "
                  << overlay->getThisNode().getIp()
                  << " (" << overlay->getThisNode().getKey().toString(16)
                  << ")]\n    CAP.size() is too fluctuant: no spreading!" << std::endl;
        return;
    }

    /*
    for (uint i = 0; i < ccd.size(); ++i) {
        std::cout << ccd[i] << ", ";
    }
    std::cout << std::endl;

    uint32_t overlaySize  = overlay->estimateOverlaySize();
    if ((ccd.size() > (2.0 * overlaySize)) ||
        (ccd.size() < (0.5 * overlaySize))) {
        //oldCcdSize = ccd.size();
        std::cout << "CCD: " << ccd.size()
                  << ", overlay->estimateOverlaySize(): " << overlaySize
                  << std::endl;
        //return;
    }
    */

    //tmpAreaDataContainer.CBRAreaPool = c2aAdapter->getCBRAreas();

    /*
    std::cout << "\n\n\nGlobalViewBuilder::spreadGlobalView(): The CCD with size = " << coordsVec.size() << ":" << std::endl;
    for (uint32_t i = 0; i < coordsVec.size(); ++i) {
            std::cout << coordsVec[i] << " ";
    }
    std::cout << endl;
     */

    const AP* cap =
        CoordBasedRoutingAccess().get()->calculateCapFromCcd(ccd,
                                                             neighborCache->overlay->getBitsPerDigit());

    /*
    C2AAdapter* c2aAdapter = C2AAdapter::getInstance();
    c2aAdapter->initialize(this, sendStrategy->getStrategyCombinedParams());
    c2aAdapter->setCoordinates(coordsVec);
    c2aAdapter->createAreas();

    const AP cap_test = c2aAdapter->getCBRAreas();

    std::cout << " >----- " << cap->size() << std::endl;
    //assert (cap_test.size() == cap->size());
    std::cout << "#nodes: " << coordsVec.size() << ", #areas: " << cap->size() << std::endl;
    for (uint32_t i = 0; i < cap->size(); ++i) {
    std::cout << cap->at(i)->prefix << " ("
              << cap->at(i)->min[0] << ", "
              << cap->at(i)->min[1] << ") - ("
              << cap->at(i)->max[0] << ", "
              << cap->at(i)->max[1] << ")"
              << std::endl;
    }

    std::cout << " ----- " << cap_test.size() << std::endl;
    for (uint32_t i = 0; i < cap_test.size(); ++i) {
    std::cout << cap_test[i]->prefix << " ("CBRArea
              << cap_test[i]->min[0] << ", "
              << cap_test[i]->min[1] << ") - ("
              << cap_test[i]->max[0] << ", "
              << cap_test[i]->max[1] << ")"
              << std::endl;
    }
    std::cout << " -----< " << std::endl;
    */

    if (cap == NULL) {
        EV << "[GlobalViewBuilder::spreadGlobalView() @ " << overlay->getThisNode().getIp()
           << " (" << overlay->getThisNode().getKey().toString(16) << ")]\n"
           << "    No CAP available for spreading!" << endl;
        std::cout << "[GlobalViewBuilder::spreadGlobalView() @ " << overlay->getThisNode().getIp()
                  << " (" << overlay->getThisNode().getKey().toString(16) << ")]\n"
                  << "    No CAP available for spreading!" << std::endl;
        return;
    }


    // measurement of CAP size
    /*
    std::fstream f;
    std::string name("CAP_");
    name += simTime().str();
    name += ".bin";
    f.open(name.c_str(), std::ios::binary|std::ios::out);

    for (uint32_t i = 0; i < cap->size(); ++i) {
        f.write((char*)&(cap->at(i).prefix), sizeof(cap->at(i).prefix));
        f.write((char*)&(cap->at(i).min[0]), sizeof(cap->at(i).min[0]));
        f.write((char*)&(cap->at(i).min[1]), sizeof(cap->at(i).min[1]));
        f.write((char*)&(cap->at(i).max[0]), sizeof(cap->at(i).max[0]));
        f.write((char*)&(cap->at(i).max[1]), sizeof(cap->at(i).max[1]));
    }
    f.close();
     */

    AreaDataCall* msg = new AreaDataCall("AreaDataCall");
    AreaDataContainer tmpAreaDataContainer;

    tmpAreaDataContainer.CBRAreaPool = *cap;
    delete cap;

    std::cout << "\n   " << simTime() << " CCD @ "
              << neighborCache->getThisNode().getIp()
              << ": size = " << ccd.size() << "\n" << std::endl;

    msg->setAreaData(tmpAreaDataContainer);
    treeManager->sendMessageToChildren(msg);

    neighborCache->globalStatistics->addStdDev("GlobalViewBuilder: CCD size",
                                               ccd.size());
}


cPar& GlobalViewBuilder::parProxy(const char *parname)
{
    return neighborCache->par(parname);
}

