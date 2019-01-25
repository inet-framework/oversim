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
 * @GlobalViewBuilder.h
 * @author Daniel Lienert
 */

#ifndef GLOBALVIEWBUILDER_H_
#define GLOBALVIEWBUILDER_H_

#include <string>
#include <NeighborCache.h>

#include <OverlayAccess.h>

#include <TreeManagement.h>
#include <SendStrategyFactory.h>
#include <StrategySendAll.h>
#include <RpcListener.h>
#include <TreeManagementMessage_m.h>
#include <CoordBasedRouting.h>

class BaseOverlay;
class TreeManagement;
class NeighborCache;
class NodeHandle;


class AbstractTreeMsgClient : public RpcListener
{
public:
    virtual bool handleRpcCall(BaseCallMessage* msg) = 0;

    virtual void newParent() { };

    virtual void newChild(const TransportAddress& child =
                          TransportAddress::UNSPECIFIED_NODE) { };
};


class GlobalViewBuilder : public AbstractTreeMsgClient
{
  private:
    double meanCoordSendInterval; //!< mean time interval between sending test messages
    double deviation; //!< deviation of time interval

    cMessage* coordSendTimer;
    cMessage* spreadCapTimer;

    std::string activeStrategy;
    bool onlyAcceptCompleteCCD;

    SendStrategyFactory sendStrategyFactory;

    AP cap;
    bool capReady;
    uint32_t oldCcdSize;

  protected:
    NeighborCache* neighborCache;
    TreeManagement* treeManager;
    AbstractSendStrategy* sendStrategy;

    BaseOverlay* overlay;  // TODO baseoverlay is only needed to fulfill a dependency in the AUTHBLOCK_L Macro

    /**
     * Send the calculated global view down the tree to the children
     */
    void spreadGlobalView();


    bool checkOverlayReady();

  public:
    GlobalViewBuilder() {
        sendStrategy = NULL;
        coordSendTimer = NULL;
        spreadCapTimer = NULL;
    };

    virtual ~GlobalViewBuilder() {
        delete sendStrategy;
        neighborCache->cancelAndDelete(coordSendTimer);
        neighborCache->cancelAndDelete(spreadCapTimer);
    };

    void initializeViewBuilder(NeighborCache* neighborCache, BaseOverlay* overlay);

    void start();

    void cleanup();

    void handleTimerEvent(cMessage* msg);

    void handleCoordSendTimer(cMessage* msg);

    void handleCoordinateRpcCall(GlobalViewBuilderCall* globalViewBuilderCall);
    void handleCapReqRpcCall(CapReqCall* call);

    void sendCapRequest(const TransportAddress& node);

    bool handleRpcCall(BaseCallMessage* msg);
    void handleRpcResponse(BaseResponseMessage* msg,
                           cPolymorphic* context,
                           int rpcId, simtime_t rtt);
    void handleRpcTimeout(BaseCallMessage* msg,
                          const TransportAddress& dest,
                          cPolymorphic* context, int rpcId,
                          const OverlayKey& destKey);

    cPar& parProxy(const char *parname);

    void newParent();

    bool isCapReady() { return capReady; };
    bool isCapValid() { return (cap.size() > 0); };
    const AP* getCAP() { return &cap; };
};

#endif /* GLOBALVIEWBUILDER_H_ */
