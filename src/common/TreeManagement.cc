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
 * @file TreeManagement.cc
 * @author Daniel Lienert
 */


#include <omnetpp.h>

#include <GlobalNodeListAccess.h>
#include <NeighborCache.h>
#include <OverlayAccess.h>
#include <GlobalNodeList.h>
#include <PeerInfo.h>
#include <CommonMessages_m.h>
#include <GlobalViewBuilder.h> //TODO own file for AbstractTreeMsgClient

#include "TreeManagement.h"


TreeManagement::TreeManagement()
{
    globalNodeList = NULL;
    treeDomainKey = OverlayKey::UNSPECIFIED_KEY;
    treeBuildTimer = new cMessage("treeBuildTimer");
    currentTreeLevel = 0;
}

TreeManagement::~TreeManagement()
{
    neighborCache->cancelAndDelete(treeBuildTimer);
}

void TreeManagement::cleanup()
{
    treeDomainKey = OverlayKey::UNSPECIFIED_KEY;
    treeChildNodes.clear();
    parentNode = NodeHandle::UNSPECIFIED_NODE;
}


void TreeManagement::init(NeighborCache* neighborCache)
{
    this->neighborCache = neighborCache;
    this->overlay = neighborCache->overlay;
    this->globalNodeList = GlobalNodeListAccess().get();

    treeMgmtBuildInterval = neighborCache->par("treeMgmtBuildInterval");
    deviation = treeMgmtBuildInterval / 10;
    timeOutInSeconds =
        neighborCache->par("treeMgmtChildrenTimeOut").doubleValue();

    creationTime = simTime();

    // init stats Counter
    for(int i = 0; i < MAXTREELEVEL; i++) {
        numTMReceived[i] = 0;
        numTMSent[i] = 0;
        bytesTMReceived[i] = 0;
        bytesTMSent[i] = 0;
    }
}


void TreeManagement::handleTimerEvent(cMessage* msg)
{
    if(msg == treeBuildTimer) {
        neighborCache->cancelEvent(treeBuildTimer);
        connectToParent();
        checkTreeChildNodes();

        neighborCache->scheduleAt(simTime() +
                                  truncnormal(treeMgmtBuildInterval,
                                              deviation),
                                  msg);
    }
}


bool TreeManagement::handleRpcCall(BaseCallMessage* msg)
{
    if (neighborCache->globalStatistics->isMeasuring()) {
        numTMReceived[currentTreeLevel]++;
        bytesTMReceived[currentTreeLevel] +=  msg->getByteLength();
    }

    RPC_SWITCH_START( msg );
    RPC_DELEGATE( ParentRequest, handleParentRequestRpcCall );
    RPC_DELEGATE( ChildCheck, handleChildCheckRpcCall );
    RPC_DELEGATE( ChildRelease, handleChildReleaseRpcCall );
    RPC_SWITCH_END( );

    return RPC_HANDLED;
}


void TreeManagement::handleRpcResponse(BaseResponseMessage* msg,
                                       cPolymorphic* context,
                                       int rpcId, simtime_t rtt)
{
    if (neighborCache->globalStatistics->isMeasuring()) {
        numTMReceived[currentTreeLevel]++;
        bytesTMReceived[currentTreeLevel] +=  msg->getByteLength();
    }

    RPC_SWITCH_START( msg );
    RPC_ON_RESPONSE( ParentRequest ) {
        handleParentRequestRpcResponse(_ParentRequestResponse, context,
                                       rpcId, rtt);
    }
    RPC_ON_RESPONSE( ChildCheck ) {
        handleChildCheckRpcResponse(_ChildCheckResponse, context,
                                    rpcId, rtt);
    }
    RPC_SWITCH_END( );

    return;
}


void TreeManagement::handleRpcTimeout(BaseCallMessage* msg,
                                      const TransportAddress& dest,
                                      cPolymorphic* context, int rpcId,
                                      const OverlayKey& destKey)
{
    RPC_SWITCH_START(msg)
    RPC_ON_CALL( ChildRelease ) {

        break;
    }
    RPC_ON_CALL( ChildCheck ) {
        EV << "[TreeManagement::handleRpcTimeout() @ "
           << neighborCache->getThisNode().getIp()
           << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
           << "    Child timout: " << dest.getIp()
           << endl;

        treeChildNodes.erase(dest);

        handleTimerEvent(treeBuildTimer);
        break;
    }
    RPC_ON_CALL( ParentRequest ) {
        EV << "[TreeManagement::handleRpcTimeout() @ "
           << neighborCache->getThisNode().getIp()
           << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
           << "    Parent timout, domainKey = " << destKey
           << endl;
        std::cout << "[TreeManagement::handleRpcTimeout() @ "
                  << neighborCache->getThisNode().getIp()
                  << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
                  << "    Parent timout, domainKey = " << destKey
                  << std::endl;

        overlay->deleteOverlayNeighborArrow(parentNode);
        parentNode = NodeHandle::UNSPECIFIED_NODE;
        treeDomainKey = OverlayKey::UNSPECIFIED_KEY;

        handleTimerEvent(treeBuildTimer);
        break;
    }
    RPC_ON_CALL( TreeApp ) {
        EV << "[TreeManagement::handleRpcTimeout() @ "
           << neighborCache->getThisNode().getIp()
           << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
           << "    Peer in tree timout: " << dest.getIp()
           << " (" << destKey << ")"
           << endl;

        if (!dest.isUnspecified() && !parentNode.isUnspecified() &&
            (dest == parentNode)) {
            EV << "[TreeManagement::handleRpcTimeout() @ "
               << neighborCache->getThisNode().getIp()
               << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
               << "    TreeAppCall (sent directly to parent "
               << dest << ") timed out, retry sending message to key " << treeDomainKey
               << endl;

            // retry to key
            parentNode = NodeHandle::UNSPECIFIED_NODE;
            handleTimerEvent(treeBuildTimer);
            //TODO delayed
            neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                            treeDomainKey,
                                            msg->dup(), NULL, DEFAULT_ROUTING,
                                            -1 , 0, -1, this);
        } else std::cout << neighborCache->getThisNode().getIp() << ": " << msg->getName()
                         << " timed out to " << dest.getIp()
                         << " (" << destKey << ")"
                         << std::endl;
    }
    RPC_SWITCH_END( )


        /*
    // check parentNode
    if (!destKey.isUnspecified() && destKey == treeDomainKey) {
        treeDomainKey = OverlayKey::UNSPECIFIED_KEY;
        overlay->deleteOverlayNeighborArrow(parentNode);
        parentNode = NodeHandle::UNSPECIFIED_NODE;
        std::cout << "parent timeout" << std::endl;
        //return;
    } else
    // check children
    if (dest.isUnspecified()) {
        for (treeNodeMap::iterator it = treeChildNodes.begin();
             it != treeChildNodes.end(); ++it) {
            if (destKey == it->second.node.getKey()) {
                treeChildNodes.erase(it);
                //return;
            }
        }
    } else if (dynamic_cast<ChildReleaseCall*>(msg)) {
        //TODO there is no response...???
        return;
    } else std::cout << msg->getName() << std::endl;

    handleTimerEvent(treeBuildTimer);
        */
}


void TreeManagement::connectToParent()
{
    EV << "|DD|> TreeManagement::connectToParent (=== PARENTCHECK for Node "
       << overlay->getThisNode().getIp() <<" ===) <||" << endl;

    if(!checkParentValid()) {
        EV << "|DD|> TreeManagement::connectToParent (Parent is not Valid) <||"
           << endl;

        // add a new connection
        registerAtParent();
    }
}


void TreeManagement::removeParentConnection()
{
    if(parentNode.isUnspecified()) return;

    //TODO
    overlay->deleteOverlayNeighborArrow(parentNode);

    EV << "|DD|> TreeManagement::removeParentConnection (RELEASE Connection From "
       << overlay->getThisNode().getIp()
       << " to Parent " << parentNode.getIp() <<  ") <||" << endl;

    sendChildReleaseCall();

    parentNode = NodeHandle::UNSPECIFIED_NODE;
}


void TreeManagement::sendChildReleaseCall()
{
    ChildReleaseCall* childReleaseRequest = new ChildReleaseCall("ChildRelease");
    simtime_t timeout = -1;

    childReleaseRequest->setBitLength(CHILDRELEASECALL_L(childReleaseRequest));

    if (neighborCache->globalStatistics->isMeasuring()) {
        numTMSent[currentTreeLevel]++;
        bytesTMSent[currentTreeLevel] +=  childReleaseRequest->getByteLength();
    }

    neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                    parentNode,
                                    childReleaseRequest,
                                    NULL,DEFAULT_ROUTING,
                                    timeout, 0, -1, this
    );
}


void TreeManagement::checkTreeChildNodes()
{
    treeNodeMap::const_iterator nodeIterator = treeChildNodes.begin();
    while (nodeIterator != treeChildNodes.end()) {
        if(nodeIterator->second.lastTouch + timeOutInSeconds < simTime()) {
            ChildCheckCall* childCheckCall = new ChildCheckCall("ChildCheckCall");
            neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                            nodeIterator->second.node,
                                            childCheckCall,
                                            NULL, DEFAULT_ROUTING,
                                            -1, 0, -1, this);
            //neighborCache->pingNode(nodeIterator->second.node, -1, -1, NULL,
            //                        "CheckChildAlive");
        }
        nodeIterator++;
    }
}


void TreeManagement::handleChildCheckRpcResponse(ChildCheckResponse* response,
                                                 cPolymorphic* context,
                                                 int rpcId, simtime_t rtt)
{
    if (treeChildNodes.find(response->getSrcNode()) != treeChildNodes.end()) {
        treeChildNodes.find(response->getSrcNode())->second.lastTouch = simTime();
    }
}


/*
void TreeManagement::pingTimeout(PingCall* call, const TransportAddress& dest,
                                 cPolymorphic* context, int rpcId)
{
    EV << "|DD|> TreeManagement::pingTimeout() " << dest << " <||" << endl;
    removeTreeChild(dest);
}
*/


bool TreeManagement::checkParentValid()
{
    bool err = false;

    if(parentNode.isUnspecified()){
        EV << "|DD|> TreeManagement::checkParentValid (Not Valid: Unspecified) <||" << endl;
        return false;
    }

    if(isRoot()) {
        OverlayKey testKey = (OverlayKey::getMax()>>1);
        if(overlay->isSiblingFor(overlay->getThisNode(), testKey ,1, &err)) {
            //std::cout << neighborCache->getThisNode().getIp() << " is still ROOT" << std::endl;
            EV << "[TreeManagement::checkParentValid() @ "
               << neighborCache->getThisNode().getIp()
               << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
               << "    I'm still ROOT!"
               << endl;
            //std::cout << neighborCache->getThisNode().getIp() << " is still ROOT" << std::endl;
            return true;
        } else {
            EV << "[TreeManagement::checkParentValid() @ "
               << neighborCache->getThisNode().getIp()
               << " (" << neighborCache->getThisNode().getKey().toString(16) << ")]\n"
               << "    I'm NOT longer ROOT!"
               << endl;
            //std::cout << simTime() << " " << neighborCache->getThisNode().getIp() << " is NOT longer ROOT" << std::endl;
            return false;
        }
    }

    // 1. if my treeDomainKey is still valid
    if(isTreeDomainKeyValid()) {
        // 2. check if parent is responsible for my TreeDomainKey
        bool isSiblingReturn = overlay->isSiblingFor(parentNode, treeDomainKey,1, &err);
        if(isSiblingReturn && !err) {
            // 3. check if I am not responsible for my TreeDomainKey
            isSiblingReturn = overlay->isSiblingFor(overlay->getThisNode(), treeDomainKey, 1, &err);
            if(!isSiblingReturn && !err) {
                //std::cout << neighborCache->getThisNode().getIp() << " 3 checks passed" << std::endl;
                EV << neighborCache->getThisNode().getIp() << " 3 checks passed" << endl;
                return true;
            }
        }
    }

    return false;
}


bool TreeManagement::isTreeDomainKeyValid()
{
    if (treeDomainKey.isUnspecified()) return false;

    if(getResponsibleDomainKey() == treeDomainKey) {
        return true;
    } else {
        return false;
    }
}


bool TreeManagement::isRoot()
{
    if (!parentNode.isUnspecified() && parentNode == overlay->getThisNode()) {
        return true;
    } else {
        return false;
    }
}

bool TreeManagement::isChild(TransportAddress& node)
{
    return (treeChildNodes.find(node) != treeChildNodes.end());
}


bool TreeManagement::isParent(TransportAddress& node)
{
    assert(!node.isUnspecified());
    return (!parentNode.isUnspecified() && (node == parentNode));
}


void TreeManagement::startTreeBuilding()
{
    cleanup();
    handleTimerEvent(treeBuildTimer);
}


bool TreeManagement::registerAtParent()
{
    treeDomainKey = getResponsibleDomainKey();

    ParentRequestCall* parentReq = new ParentRequestCall("ParentRequest");
    simtime_t timeout = -1;

    parentReq->setDomainKey(getResponsibleDomainKey());

    parentReq->setBitLength(PARENTREQUESTCALL_L(parentReq));

    if (neighborCache->globalStatistics->isMeasuring()) {
        numTMSent[currentTreeLevel]++;
        bytesTMSent[currentTreeLevel] +=  parentReq->getByteLength();
    }

    neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                    treeDomainKey,
                                    parentReq,
                                    NULL,DEFAULT_ROUTING,
                                    timeout, 0, -1, this
    );

    return true;
}


void TreeManagement::handleChildReleaseRpcCall(ChildReleaseCall* msg)
{

    removeTreeChild(msg->getSrcNode());

    delete msg;
}


void TreeManagement::handleChildCheckRpcCall(ChildCheckCall* msg)
{
    if (parentNode.isUnspecified() || msg->getSrcNode() != parentNode) {
        delete msg;
        return;
    }

    ChildCheckResponse* response = new ChildCheckResponse("ChildCheckReponse");
    neighborCache->sendRpcResponse(msg, response);
}


void TreeManagement::removeTreeChild(const TransportAddress& childToRemove)
{
    if(treeChildNodes.empty()) return;

    //debugChildren();

    if(treeChildNodes.find(childToRemove) != treeChildNodes.end()) {
        treeChildNodes.erase(childToRemove);
    }

    //debugChildren();
}

void TreeManagement::handleParentRequestRpcCall(ParentRequestCall* msg)
{
    EV << "|DD|> TreeManagement::handleParentRequestRpcCall (ADDCHILDNODE!!! "
       << msg->getSrcNode().getIp() << " to "
       << overlay->getThisNode().getIp() << ") <||" << endl;

    /*
    if (dynamic_cast<OverlayCtrlInfo*>(msg->getControlInfo())) {
        //OverlayCtrlInfo* overlayCtrlInfo = static_cast<OverlayCtrlInfo*>(msg->getControlInfo());
        if (msg->getDomainKey() != getResponsibleDomainKey().second) {
            std::cout << neighborCache->getThisNode().getIp()
                      << ", msg->getDomainKey(): " << msg->getDomainKey()
                      << ", getResponsibleDomainKey().second: " << getResponsibleDomainKey().second
                      << " --> not responsible!" << std::endl;
            //delete msg;
            //return;
        }
    }
    */
    addChildNode(msg->getSrcNode());

    ParentRequestResponse* parentResp = new ParentRequestResponse("ParentResponse");
    parentResp->setBitLength(PARENTREQUESTRESPONSE_L(parentResp));
    neighborCache->sendRpcResponse(msg, parentResp);
}


void TreeManagement::handleParentRequestRpcResponse(ParentRequestResponse* response,
                                                    cPolymorphic* context, int rpcId, simtime_t rtt)
{
    EV << "|DD|> TreeManagement::handleParentRequestRpcResponse ("
       << overlay->getThisNode().getIp() <<" ISCHILDOF "
       << response->getSrcNode().getIp() << ") <||" << endl;


    if(parentNode.isUnspecified() || response->getSrcNode() != parentNode) {
        // remove old connection
        removeParentConnection();

        // add new connection
        if (response->getSrcNode() == neighborCache->overlay->getThisNode()) {
            //std::cout << simTime() << " " << neighborCache->getThisNode().getIp() << " (thinks that it) is now ROOT!" << std::endl;
        }
        parentNode = response->getSrcNode();

        // TODO
        overlay->showOverlayNeighborArrow(parentNode, true,
                                          "m=m,50,0,50,0;ls=blue,1");

        //inform clients
        for (msgClientMap::iterator it = msgClients.begin();
             it != msgClients.end(); ++it) {
            it->second->newParent();
        }
    } /*else {
        std::cout << (parentNode.isUnspecified()?"parentNode.isUnspecified()":"NOT parentNode.isUnspecified()") << "\n"
                  << response->getSrcNode() << " " << parentNode << std::endl;
    }*/
}


void TreeManagement::addChildNode(NodeHandle& childNode)
{
    if (overlay->getThisNode() == childNode) return;
    if (treeChildNodes.find(childNode) == treeChildNodes.end()) {
        treeNodeEntry entry;

        entry.node = childNode;
        entry.lastTouch = simTime();
        treeChildNodes.insert(treeNodePair(childNode, entry));

        //inform clients
        for (msgClientMap::iterator it = msgClients.begin();
                it != msgClients.end(); ++it) {
            it->second->newChild(childNode);
        }
    }

    debugChildren();
}

void TreeManagement::debugChildren()
{
    std::stringstream shortChildMapString;

    shortChildMapString << "CL ";

    EV << "=== Children List of " << overlay->getThisNode().getIp() << "===" << endl;

    treeNodeMap::const_iterator nodeMapIterator = treeChildNodes.begin();

    while(nodeMapIterator != treeChildNodes.end()) {
        EV << " - " << (*nodeMapIterator).first << endl;
        shortChildMapString << (*nodeMapIterator).first.getIp() << " | ";
        ++nodeMapIterator;
    }

    //neighborCache->getParentModule()->getDisplayString().setTagArg("t2", 0, shortChildMapString.str().c_str());

    EV << "=====================" << endl;
}


OverlayKey TreeManagement::getResponsibleDomainKey()
{
    NodeHandle node = overlay->getThisNode();
    bool err = false;

    OverlayKey rightBorder = OverlayKey::getMax(); // 1
    OverlayKey leftBorder = OverlayKey::ZERO;
    OverlayKey testKey = OverlayKey::getMax()>>1;	// middle of idSpace
    OverlayKey domainKey = OverlayKey::getMax()>>1;

    int it = 1;
    int maxIteration = overlay->par("keyLength");

    while(it < maxIteration) {

        domainKey = testKey;
        testKey = (leftBorder>>1) + (rightBorder>>1);

        EV << "|DD|> TreeManagement::getResponsibleDomain (Checking: "
           << testKey <<") <||" << endl;

        if(overlay->isSiblingFor(node, testKey,1, &err)) {
            currentTreeLevel = it;
            EV << "|DD|> TreeManagement::getResponsibleDomainKey ("
               <<  "CURTREELEVEL: " << currentTreeLevel << ") <||" << endl;
            return domainKey;
        }

        if(err) {
            EV << "|DD|> TreeManagement::getResponsibleDomain (ERROR WHILE CALLING isSiblingFor) <||" << endl;
        }

        if(node.getKey().isBetween(leftBorder, testKey)) { // linke Seite
            rightBorder = testKey;
        } else if (node.getKey().isBetween((rightBorder+leftBorder)>>1,rightBorder)) {
            leftBorder=testKey;
        } else {
            EV << "|DD|> TreeManagement::getResponsibleDomain (KEY IS THE BORDER!) <||" << endl;
        }

        it++;
    }

    return OverlayKey::ZERO;
}


bool TreeManagement::sendMessageToParent(BaseCallMessage* msg)
{
    if(parentNode.getKey().isUnspecified()) {
        return false;
    }

    if (neighborCache->globalStatistics->isMeasuring()) {
        numTMSent[currentTreeLevel]++;
        bytesTMSent[currentTreeLevel] +=  msg->getByteLength();
    }

    /*
    // send to key
    neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                    treeDomainKey,
                                    msg, NULL, DEFAULT_ROUTING,
                                    -1 , 0, -1, this);
    */

    // send directly
    neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                    parentNode, msg, NULL, DEFAULT_ROUTING,
                                    -1 , 0, -1, this);

    return true;
}


bool TreeManagement::sendMessageToChildren(BaseCallMessage* msg)
{
    simtime_t timeout = -1;

    if (neighborCache->globalStatistics->isMeasuring()) {
        numTMSent[currentTreeLevel]++;
        bytesTMSent[currentTreeLevel] +=  msg->getByteLength();
    }

    treeNodeMap::const_iterator nodeMapIterator = treeChildNodes.begin();
    while(nodeMapIterator != treeChildNodes.end()) {
        /*
        neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                        (*nodeMapIterator).second.node.getKey(),
                                        msg->dup(),
                                        NULL,DEFAULT_ROUTING,
                                        timeout, 0, -1, this);
        */
        neighborCache->sendRouteRpcCall(neighborCache->getThisCompType(),
                                        (*nodeMapIterator).second.node,
                                        msg->dup(),
                                        NULL,DEFAULT_ROUTING,
                                        timeout, 0, -1, this);
        ++nodeMapIterator;
    }

    delete msg;
    return true;
}


const NodeHandle& TreeManagement::getParentNode()
{
    return parentNode;
}


const treeNodeMap& TreeManagement::getChildNodes()
{
    return treeChildNodes;
}


void TreeManagement::addMsgClient(const char* identifier, AbstractTreeMsgClient* msgClient)
{
    if(msgClients.find(identifier) == msgClients.end()) {
        msgClients.insert(msgClientPair(identifier, msgClient));
    }

}


void TreeManagement::removeMsgClient(const char* identifier)
{
    if(msgClients.find(identifier) != msgClients.end()) {
        msgClients.erase(identifier);
    }
}


int TreeManagement::getCurrentTreeLevel()
{
    return currentTreeLevel;
}


void TreeManagement::finishTreeManagement()
{
    int i;
    std::stringstream statName;

    simtime_t time = neighborCache->globalStatistics->calcMeasuredLifetime(creationTime);

    for(i = 1; i < MAXTREELEVEL; i++) {
        if(numTMSent[i] > 0 || numTMReceived[i] > 0) {

            //int branchingFactor = 2;

            // messages sent per level
            statName.str("");
            statName << "TreeManagement Level " << i << " numTMSent/Node/s";
            neighborCache->globalStatistics->addStdDev(statName.str(), (double)numTMSent[i] / SIMTIME_DBL(time) );

            statName.str("");
            statName << "TreeManagement Level " << i << " numTMReceived/Node/s";
            neighborCache->globalStatistics->addStdDev(statName.str(), (double)numTMReceived[i] / SIMTIME_DBL(time));

            statName.str("");
            statName << "TreeManagement Level " << i << " numTMTotal/Node/s";
            neighborCache->globalStatistics->addStdDev(statName.str(), ((double)numTMReceived[i] + (double)numTMSent[i]) / SIMTIME_DBL(time));

            // bytes per time send per level
            statName.str("");
            statName << "TreeManagement Level " << i << " bytesTMSent/Node/s";
            neighborCache->globalStatistics->addStdDev(statName.str(), (double)bytesTMSent[i] / SIMTIME_DBL(time));

            statName.str("");
            statName << "TreeManagement Level " << i << " bytesTMReceived/Node/s";
            neighborCache->globalStatistics->addStdDev(statName.str(), (double)bytesTMReceived[i] / SIMTIME_DBL(time));

            statName.str("");
            statName << "TreeManagement Level " << i << " bytesTMTotal/Node/s";
            neighborCache->globalStatistics->addStdDev(statName.str(), ((double)bytesTMReceived[i] + (double)bytesTMSent[i]) / SIMTIME_DBL(time));
        }
    }
}

