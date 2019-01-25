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
 * @file TreeManagement.h
 * @author Daniel Lienert
 */


#ifndef TREEMANAGEMENT_H_
#define TREEMANAGEMENT_H_

#define  MAXTREELEVEL 25	// 20 levels = 1 000 000 Nodes

#include <map>
#include <oversim_mapset.h>

#include <NodeHandle.h>
#include <GlobalNodeList.h>
#include <OverlayKey.h>
#include <NeighborCache.h>
#include <TreeManagementMessage_m.h>

class GlobalNodeList;
class TransportAddress;
class BaseOverlay;
class NodeHandle;
class AbstractTreeMsgClient;

struct treeNodeEntry {
    NodeHandle node;
    simtime_t lastTouch;
};

typedef std::pair<TransportAddress, treeNodeEntry> treeNodePair;
typedef UNORDERED_MAP<TransportAddress, treeNodeEntry, TransportAddress::hashFcn> treeNodeMap;


class TreeManagement: public RpcListener //,  public TopologyVis
{

private:

    GlobalNodeList* globalNodeList;
    NeighborCache* neighborCache;
    BaseOverlay* overlay;
    NodeHandle parentNode;
    OverlayKey treeDomainKey;

    treeNodeMap treeChildNodes;

    cMessage* treeBuildTimer;

    double treeMgmtBuildInterval;
    double deviation;
    double timeOutInSeconds;

    //typedef std::pair<const char*, RpcListener*> msgClientPair;
    //typedef UNORDERED_MAP<const char*, RpcListener*> msgClientMap;

    typedef std::pair<const char*, AbstractTreeMsgClient*> msgClientPair;
    typedef UNORDERED_MAP<const char*, AbstractTreeMsgClient*> msgClientMap;

    msgClientMap msgClients;

public:

    TreeManagement();

    /*
     * Inject the Neighbour Cache Object
     * @param Pointer to NeighborCache
     */
    void init(NeighborCache* neighborCache);

    virtual ~TreeManagement();

    void handleTimerEvent(cMessage* msg);

    virtual bool handleRpcCall(BaseCallMessage* msg);

    void handleRpcTimeout(BaseCallMessage* msg,
                          const TransportAddress& dest,
                          cPolymorphic* context, int rpcId,
                          const OverlayKey& destKey);

    /**
     * send a message to the parent
     * @param msg BaseCallMessage
     */
    bool sendMessageToParent(BaseCallMessage* msg);

    /*
     * send a message to all children
     * @praram msg BaseCallMessage
     */
    bool sendMessageToChildren(BaseCallMessage* msg);

    /*
     * start the process an schedule it
     */
    void startTreeBuilding();

    /*
     * return a node haNodeHandlendle to the current parent map
     * @returns NodeHandle parent node
     */
    const NodeHandle& getParentNode();

    /*
     * Check if the node is root (parent points to self)
     * @returns boolean
     */
    bool isRoot();

    bool isChild(TransportAddress& node);

    bool isParent(TransportAddress& node);


    /**
     * return the current tree level of this node
     */
    int getCurrentTreeLevel();

    /*
     * return the child nodes in an unorderd map
     * @returns treeNodeMap
     */
    const treeNodeMap& getChildNodes();

    void addMsgClient(const char* identifier, AbstractTreeMsgClient* msgClient);

    void removeMsgClient(const char* identifier);

    void handleChildCheckRpcCall(ChildCheckCall* call);

    void handleChildCheckRpcResponse(ChildCheckResponse* response,
                                     cPolymorphic* context,
                                     int rpcId, simtime_t rtt);

    //void pingTimeout(PingCall* call, const TransportAddress& dest,
    //                 cPolymorphic* context, int rpcId);

    /**
     * finish module and collect statistical data
     */
    void finishTreeManagement();

protected:

    /**
     * Current TreeLevel of this node - calculated by getResponsibleDomainKey
     */
    int currentTreeLevel;

    int numTMSent[MAXTREELEVEL];
    int numTMReceived[MAXTREELEVEL];
    int bytesTMSent[MAXTREELEVEL];
    int bytesTMReceived[MAXTREELEVEL];

    simtime_t creationTime;

    /**
     * Check if a valid parent connection exists and establish one if not
     */
    void connectToParent();

    /*
     * Remove an existing but not longer valid connection
     */
    void removeParentConnection();

    /*
     *	Check if the parent is valid
     *	@returns boolean false if no connection or a connection to a not responsible parent exists - true otherwise
     */
    bool checkParentValid();

    /*
     * Search for the parents domain of the actual node and send a parent request
     */
    bool registerAtParent();

    /*
     * Check if the the current saved treeDomainKey is still valid
     * @returns boolean
     */
    bool isTreeDomainKeyValid();


    /**
     * Crawl the ID Space to find the smallest "Domain" for which the current node is NOT responsible
     * @return OverlayKey a random key from this domain
     */
    OverlayKey getResponsibleDomainKey();

    void handleParentRequestRpcCall(ParentRequestCall* msg);
    void handleParentRequestRpcResponse(ParentRequestResponse* response,
                                        cPolymorphic* context, int rpcId, simtime_t rtt);

    void handleChildReleaseRpcCall(ChildReleaseCall* msg);

    /*
     * Draw a line from the curent node to the given node
     * @param treeNode Nodehandle to draw the line to
     * @return void
     */
    void visualizeTreeLinkToNode(const NodeHandle& treeNode);

    void addChildNode(NodeHandle& childNode);

    virtual void handleRpcResponse(BaseResponseMessage* msg,
                                   cPolymorphic* context,
                                   int rpcId, simtime_t rtt);

    /**
     * send a message to my parent to release me from his children-list
     */
    void sendChildReleaseCall();

    /**
     * check the lastTouch record of theChildNode map and send a ping if the time is over limit
     */
    void checkTreeChildNodes();

    /**
     * remove child from list
     * @param NodeHandle handle to childNode
     */
    void removeTreeChild(const TransportAddress& childToRemove);

    void debugChildren();

    void cleanup();
};

#endif /* TREEMANAGEMENT_H_ */
