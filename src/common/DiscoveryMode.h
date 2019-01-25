/*
 * DiscoveryMode.h
 *
 *  Created on: Sep 27, 2010
 *      Author: heep
 */

#ifndef DISCOVERYMODE_H_
#define DISCOVERYMODE_H_

#include <RpcListener.h>
#include <NeighborCache.h>
#include <DiscoveryMode_m.h>

class BasePastry;

class DiscoveryMode: public RpcListener, ProxListener
{
  private:
    NeighborCache* neighborCache;

    uint8_t numCloseNodes;
    uint8_t numSpreadedNodes;

    double firstRtt;
    double improvement;

    bool finished;

    BasePastry* basePastry;

  protected:
    int16_t step;
    uint8_t maxSteps;
    int8_t spreadedSteps;
    uint8_t queries;
    int8_t maxIndex;
    simtime_t nearNodeRtt;
    bool nearNodeImproved;
    TransportAddress nearNode;

    void proxCallback(const TransportAddress& node, int rpcId,
                      cPolymorphic *contextPointer, Prox prox);

    void sendNewRequest(DiscoveryNodesType type, uint8_t numNodes);

  public:
    virtual ~DiscoveryMode() {};

    void init(NeighborCache* neighborCache);
    void start(const TransportAddress& bootstrapNode);
    void stop();

    double getImprovement() { return improvement; };
    bool isFinished() { return finished; };

    bool handleRpcCall(BaseCallMessage* msg);

    void handleRpcResponse(BaseResponseMessage* msg,
                           cPolymorphic* context,
                           int rpcId, simtime_t rtt);

    void handleRpcTimeout(BaseCallMessage* msg,
                          const TransportAddress& dest,
                          cPolymorphic* context, int rpcId,
                          const OverlayKey& destKey);
};

#endif /* DISCOVERYMODE_H_ */
