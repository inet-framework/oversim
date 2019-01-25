
#ifndef LOGINCACHE_H
#define LOGINCACHE_H

#include <omnetpp.h>
#include <NodeHandle.h>
#include <Vector2D.h>
#include <GlobalStatisticsAccess.h>

class lcEntry {
    public:
        lcEntry() {}
        lcEntry(Vector2D p, simtime_t t) : pos(p), time(t) {}
        Vector2D pos;
        simtime_t time;
};

class LoginCache : public cSimpleModule
{
    public:
        void initialize();
        void finish();
        NodeHandle getLoginNode( Vector2D pos );
        void registerPos( NodeHandle node, Vector2D pos );

    private:
        std::map<NodeHandle,lcEntry> nodelist;
        double movementSpeed;
        int bytessend;
        int bytesrecv;
        GlobalStatistics* globalStatistics;

};

#endif
