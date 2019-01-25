#include "LoginCache.h"
#include "Quon_m.h"
#include <limits>

Define_Module(LoginCache);

void LoginCache::initialize()
{
    globalStatistics = GlobalStatisticsAccess().get();

    movementSpeed = par("movementSpeed");
    bytesrecv = 0;
    bytessend = 0;
}

void LoginCache::registerPos( NodeHandle node, Vector2D pos )
{
    nodelist[node] = lcEntry(pos, simTime() );

    RECORD_STATS(
        bytesrecv += (NODEHANDLE_L + QUONPOSITION_L)/8;
        );
}

NodeHandle LoginCache::getLoginNode( Vector2D pos )
{
    double bestDist = std::numeric_limits<double>::infinity();
    NodeHandle bestNode = NodeHandle::UNSPECIFIED_NODE;
    for( std::map<NodeHandle,lcEntry>::iterator it = nodelist.begin(); it != nodelist.end(); ++it ){
        double age = SIMTIME_DBL(simTime() - it->second.time);
        double dist = pos.distanceSqr(it->second.pos) + movementSpeed*movementSpeed*age*age;
        if( dist < bestDist ) {
            bestNode = it->first;
            bestDist = dist;
        }
    }
    RECORD_STATS(
        bytessend += (NODEHANDLE_L)/8;
        );
    return bestNode;
}

void LoginCache::finish() {
    
    simtime_t time = globalStatistics->calcMeasuredLifetime(0);
    if (time < GlobalStatistics::MIN_MEASURED) return;

    globalStatistics->addStdDev("LoginChache: Sent Bytes/s", bytessend / time);
    globalStatistics->addStdDev("LoginChache: Received Bytes/s", bytesrecv / time);

}
