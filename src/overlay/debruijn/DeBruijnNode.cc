//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

/**
 * @file DeBruijnNode.cc
 * @author Jan Peter Drees
 */

#include "DeBruijnNode.h"
#include "DeBruijnNodeHandle.h"
#include "DeBruijnPresentationMessage_m.h"
#include "GeneralDeBruijnMessage_m.h"

// Define the module for OMNet++
Define_Module(DeBruijnNode);

enum command {LINEARIZE, DELEGATE, INTRODUCE, PROBE, PROBE_DONE, GENERAL_PROBE, GENERAL_PROBE_DONE, SEARCH, SEARCH_DONE};

/**
 * Gets called when the node is initialized.
 */
void DeBruijnNode::onInitialize() {
    //Initialize my own handle
    DeBruijnNodeHandle* vd = new DeBruijnNodeHandle();
    v = *vd;
    v.setId(joinID);
    v.handle = *SimpleOverlayUtils::getNode(joinID);
    EV << v.id << ": My Hash is " << v.hash << endl;

    //Set up topology change logging
    topologyChangedVec.setName("Topology Changed");

    //Initialize the different connection sets
    //nullhandle, left, right
    DeBruijnNodeHandle* nullhandled = new DeBruijnNodeHandle();
    nullhandled->handle = NodeHandle::UNSPECIFIED_NODE;
    nullhandled->id = -10.0;
    nullhandle = *nullhandled;
    left = nullhandle;
    right = nullhandle;
    
    //q-neighborhood
    std::set<DeBruijnNodeHandle>* edgeqp = new std::set<DeBruijnNodeHandle>();
    edgeq = *edgeqp;
    qrobinposition = 0;
    c = 3;
    d = par("dimensions");
    int nguess = 5;
    q=ceil(pow(nguess,1.0/d));

    //de Bruijn edges
    edgedb = std::vector<std::vector<DeBruijnNodeHandle> > (
        2,
        std::vector<DeBruijnNodeHandle>(2));
    edgedb.at(0).at(0) = nullhandle;
    edgedb.at(0).at(1) = nullhandle;
    edgedb.at(1).at(0) = nullhandle;
    edgedb.at(1).at(1) = nullhandle;
    
    //general de Bruijn
    dbrobiniposition = 2;
    dbrobinjposition = 0;
    expand_edgedb();

    //Start bootstrapping
    if (joinID > 0) {
        //Save a single bootstrapping node
        int bootstrapid = intuniform(0,joinID-1);
        DeBruijnNodeHandle* bootstrapnode = new DeBruijnNodeHandle();
        DeBruijnNodeHandle bootstraphandle = *bootstrapnode;
        bootstraphandle.setId(bootstrapid);
        bootstraphandle.handle = *SimpleOverlayUtils::getNode(bootstrapid);
        if(intuniform(0,1) == 0){
            //Insert the bootstrapping node in the list edge set
            left = bootstraphandle;
            edgeq.insert(bootstraphandle);
            yell("Creating outgoing bootstrapping edge");
        }else{
            yell("Creating incoming bootstrapping edge");
            this->call_linearize(bootstraphandle, v);
        }
        EV << v.id << ": My bootstrapping node key is " << bootstraphandle.id << endl;
        EV << v.id << ": My bootstrapping node hash is " << bootstraphandle.hash << endl;
    }

    // update the GUI
    icon = "question";
    visnode = par("visnode");
    darkcolors = par("darkcolors");
    generaldbsize = 0;
    showlist = par("showlist");
    showq = par("showq");
    showdb = par("showdb");
    showgeneraldb = par("showgeneraldb");
    nodeNumber = par("nodeNumber");
    updateIcon();
    updateTooltip();
}

/**
 * Gets called continuously during simulation.
 */
void DeBruijnNode::onTimeout() {
    EV << v.id << ": calling Timeout" << endl;
    icon = "hourglass";
    updateIcon();
    //Call the timeout methods for all subprotocols
    this->buildListTimeout();
    this->qNeighborhoodTimeout();
    this->deBruijnTimeout();
    this->generalDeBruijnTimeout();
    if(v.id==visnode){
        DeBruijnNodeHandle* tnode = new DeBruijnNodeHandle();
        DeBruijnNodeHandle thandle = *tnode;
        int searchid = intuniform(0,nodeNumber-1);
        thandle.setId(searchid);
        thandle.handle = NodeHandle::UNSPECIFIED_NODE;
        std::stringstream ttStream;
        ttStream << "Searching for node " << searchid;
        yell(ttStream.str().c_str());
        this->call_search(v, thandle, v, d*q, d-1);
    }
    updateTooltip();
}

/**
 * Gets called when a message for the Overlay Protocol is received.
 */
void DeBruijnNode::onMessageReceived(BaseOverlayMessage* msg) {
    // determine message type
    if (DeBruijnPresentationMessage* message=dynamic_cast<DeBruijnPresentationMessage*>(msg)) {
        // we received a presentation message
        EV << v.id << ": Received Message!";
        // This will be some node of the network that has been introduced to us
        left = message->getContent();
        // Delete the message
        delete msg;

    } else if (GeneralDeBruijnMessage* message=dynamic_cast<GeneralDeBruijnMessage*>(msg)) {
        // We received a de Bruijn message. Call the respective function
        int com = message->getCommand();
        switch (com) {
            case LINEARIZE:
                EV << v.id << ": calling LINEARIZE" << endl;
                linearize(message->getU());
                break;
            case DELEGATE:
                EV << v.id << ": calling DELEGATE" << endl;
                delegate(message->getU());
                break;
            case INTRODUCE:
                EV << v.id << ": calling INTRODUCE" << endl;
                if(message->getQtilde().handle.isUnspecified()){
                    throw cRuntimeError("Received nullpointer in qtilde");
                }
                introduce(message->getQtilde(),message->getSender());
                break;
            case PROBE:
                EV << v.id << ": calling PROBE" << endl;
                probe(message->getSender(),message->getT().hash,*new std::string(message->getMode()));
                break;
            case PROBE_DONE:
                EV << v.id << ": calling PROBE_DONE" << endl;
                probe_done(message->getT(),message->getResult());
                break;
            case GENERAL_PROBE:
                EV << v.id << ": calling GENERAL_PROBE" << endl;
                if(message->getSender().handle.isUnspecified()){
                    throw cRuntimeError("Received nullpointer in sender");
                }
                general_probe(message->getSender(),message->getT().hash,message->getI(),message->getJ(),message->getDB());
                break;
            case GENERAL_PROBE_DONE:
                EV << v.id << ": calling GENERAL_PROBE_DONE" << endl;
                general_probe_done(message->getResult(),message->getI(),message->getJ());
                break;
            case SEARCH:
                EV << v.id << ": calling SEARCH" << endl;
                deBruijnSearch(message->getT(),message->getSender(),message->getR(),message->getRemHops());
                break;
            case SEARCH_DONE:
                EV << v.id << ": calling SEARCH_DONE" << endl;
                search_done(message->getT(),message->getResult());
                break;
            default:
                throw cRuntimeError("Message with invalid command received");
                break;
        }
        delete msg;
    }
    icon = "check";
    updateTooltip();
    updateIcon();
}

/**
 * Send the GeneralDeBruijnMessage msg with the procedure call command to the node destination
 */
void DeBruijnNode::callFunction(DeBruijnNodeHandle destination, int command, GeneralDeBruijnMessage* msg){
    if(state!=FAILED){
        if(!destination.handle.isUnspecified()){
            msg->setCommand(command);
            sendMessage(msg, destination.handle);
        }else{
            EV << v.id << ": Function "<< command << " was called on a empty node"  << endl;
            yell("NULLCALL");
        }
    }
}

/**
 * Document all topology changes. Takes an optional boolean argument of whether it actually changed.
 */
void DeBruijnNode::topology_changed(bool did_change = true){
    if(did_change){
        EV << v.id << ": Topology changed" << endl;
        simtime_t current_time = simTime();
        topologyChangedVec.record(current_time);
    }
}

/**
 * Updates the GUI.
 */
void DeBruijnNode::updateTooltip() {
    // is this module displayed?
    if (ev.isGUI()) {
        // show node hash in the tooltip
        std::stringstream ttString;
        DeBruijnNodeHandle qmin = nullhandle;
        DeBruijnNodeHandle qmax = nullhandle;
        if(edgeq.size()>=2){
            std::set<DeBruijnNodeHandle>::iterator it = edgeq.begin();
            qmin = *it;
            std::advance(it, edgeq.size()-1);
            qmax = *it;
        }
        ttString << v.hash << " q:" << q << " |Q|:" << edgeq.size()<< "/"<< c*2*q << " qmin:" << qmin.id << " qmax:" << qmax.id << " db(1,0):"<<edgedb.at(1).at(0).id<<" db(1,1):"<<edgedb.at(1).at(1).id <<" |db|:"<<generaldbsize;
        updateMouseOver(ttString.str().c_str());

        double radius = 300;
        double xoffset = 450;
        double yoffset = 320;
        double angle = v.hash * 2 * PI;
        double x = xoffset+ radius * cos(angle);
        double y = yoffset+ radius * sin(angle);
        if(angle > 90){
            x = xoffset+ radius * sin(angle);
            y = yoffset- radius * cos(angle);
        }
        std::stringstream xString;
        xString << x ;
        std::stringstream yString;
        yString << y ;
        getParentModule()->getParentModule()->getDisplayString().setTagArg("p", 0, xString.str().c_str());
        getParentModule()->getParentModule()->getDisplayString().setTagArg("p", 1, yString.str().c_str());

        std::stringstream iString;
        iString << "device/pc";
        std::stringstream isString;
        isString << "vs";
        getParentModule()->getParentModule()->getDisplayString().setTagArg("i", 0, iString.str().c_str());
        getParentModule()->getParentModule()->getDisplayString().setTagArg("is", 0, isString.str().c_str());

        if(visnode == -1 || v.id==visnode){
            this->drawAllNeighborConnections();
        }
    }
}

/**
 * Updates status icon.
 */
void DeBruijnNode::updateIcon(){
    std::stringstream i2String;
    i2String << "status/" << icon;
    getParentModule()->getParentModule()->getDisplayString().setTagArg("i2", 0, i2String.str().c_str());
}

/**
 * Draws arrows to all neighbors in the GUI
 */
void DeBruijnNode::drawAllNeighborConnections(){
    //put list nodes into sets
    std::set<DeBruijnNodeHandle>* leftset = new std::set<DeBruijnNodeHandle>;
    leftset->insert(left);
    std::set<DeBruijnNodeHandle>* rightset = new std::set<DeBruijnNodeHandle>;
    rightset->insert(right);
    //Flush earlier arrows
    const NodeHandle& handle = nullhandle.handle;
    showOverlayNeighborArrow(handle, true, "");
    // put De Bruijn nodes into set
    std::set<DeBruijnNodeHandle>* generaldebruijnset = new std::set<DeBruijnNodeHandle>;

    generaldbsize = 0;
    for(std::vector<std::vector<DeBruijnNodeHandle> >::iterator vit = edgedb.begin(); vit != edgedb.end(); ++vit) {
        std::vector<DeBruijnNodeHandle> thisvector = *vit;
        for(std::vector<DeBruijnNodeHandle>::iterator it = thisvector.begin(); it != thisvector.end(); ++it) {
            DeBruijnNodeHandle nh = *it;
            if(!nh.handle.isUnspecified() && nh.hash != v.hash){
                generaldebruijnset->insert(*it);
            }
        }
    }
    std::set<DeBruijnNodeHandle>* debruijnset = new std::set<DeBruijnNodeHandle>;
    debruijnset->insert(edgedb.at(1).at(1));
    generaldebruijnset->erase(edgedb.at(1).at(1));
    debruijnset->insert(edgedb.at(1).at(0));
    generaldebruijnset->erase(edgedb.at(1).at(0));
    generaldbsize = generaldebruijnset->size();
    if(darkcolors){
        if(showlist){
            this->drawSetNeighborConnections(*leftset, "darkblue,3");
            this->drawSetNeighborConnections(*rightset, "darkcyan,3");
        }
        if(showdb){
            this->drawSetNeighborConnections(*debruijnset, "darkgreen,2");
        }
        if(showgeneraldb){
            this->drawSetNeighborConnections(*generaldebruijnset, "darkcyan,1");
        }
        if(showq){
            this->drawSetNeighborConnections(edgeq, "firebrick,1");
        }
    }else{
        if(showlist){
            this->drawSetNeighborConnections(*leftset, "skyblue,3");
            this->drawSetNeighborConnections(*rightset, "darkslateblue,3");
        }
        if(showdb){
            this->drawSetNeighborConnections(*debruijnset, "limegreen,2");
        }
        if(showgeneraldb){
            this->drawSetNeighborConnections(*generaldebruijnset, "forestgreen,1");
        }
        if(showq){
            this->drawSetNeighborConnections(edgeq, "darkorange,1");
        }
    }
}

void DeBruijnNode::drawSetNeighborConnections(std::set<DeBruijnNodeHandle> set, std::string color){
    std::set<DeBruijnNodeHandle>::iterator it;
    bool flush = true;
    for (it = set.begin(); it != set.end(); ++it)
    {
        DeBruijnNodeHandle dbhandle = *it;
        const NodeHandle& handle = dbhandle.handle;
        std::stringstream ttString;
        ttString << "m=m,50,100,50,100;ls=" << color;
        showOverlayNeighborArrow(handle, false, ttString.str().c_str());
    }
}

/**
 * Searches for a node with the ID t
 * TODO: FINISH IMPLEMENTING
 */
void DeBruijnNode::deBruijnSearch(DeBruijnNodeHandle t, DeBruijnNodeHandle sender, double r, int remHops){
    /*
    Pseudocode from the paper:
    procedure DeBruijnSearch(t ∈ N, r ∈ Z, remHops ∈ N_0)
        if v.id = t then
            return "Success!"
        if remHops > 0 ∧ log(q) ≤ r then
            bin_t ← Compute_Bitstring(t, r)      (First r bits of t’s bit string)
            (t_1 t_2 . . . t_k)_q ← Base_Transform(bin_t , q)        (Transform bin_t to base q)
            Determine the edge v → u with minimal value |u − (v+t_k)/q|
            u ← DeBruijnSearch(t, r − log(q), remHops − 1)      (General de Bruijn hop)
        else
            u ← argmin(v,w)∈E_q {|w − h(t)|}     (Greedy search based on u.Q)
            if |u − h(t)| < |v − h(t)| then
                u ←DeBruijnSearch(t, −1, 0)     (Forward request to closer node)
            else
                return "Failure!"
     */

    //Check whether we are the destination
    //if v.id = t then
    if(v.id == t.id){
        //return "Success!"
        call_search_done(sender, t, v);
        return;
    }
    //if remHops > 0 ∧ log(q) ≤ r then
    if(remHops>0 && log2(q) <= r){
        //bin_t ← Compute_Bitstring(t, r)      (First r bits of t’s bit string)
        //Do this in base 10 instead
        unsigned long int tent = (int) floor(t.hash * pow(2.0,r));
        //(t_1 t_2 . . . t_k)_q ← Base_Transform(bin_t , q)        (Transform bin_t to base q)
        //PROTOCOL CHANGE because everything smaller would cause a math error ("If the second operand is zero, the behavior is undefined.")
        unsigned long int tk = tent % (unsigned long int) std::max(ceil(q), 1.0);
        //Determine the edge v → u with minimal value |u − (v+t_k)/q|
        long double target = (v.hash+tk)/std::max(ceil(q), 1.0);
        DeBruijnNodeHandle u = closest_edgeq(target,true);
        //search for better options in the de Bruijn edges as well
        DeBruijnNodeHandle udb = closest_edgedb(target,true);
        if(std::abs(udb.hash-t.hash) < std::abs(u.hash-t.hash)){
            u = udb;
        }
        //u ← DeBruijnSearch(t, r − log(q), remHops − 1)      (General de Bruijn hop)
        call_search(u,t,sender,r-log2(q), remHops-1);
    }else{
        //u ← argmin(v,w)∈E_q {|w − h(t)|}
        DeBruijnNodeHandle u = closest_edgeq(t.hash, true);
        //if |u − h(t)| < |v − h(t)| then
        if(!u.handle.isUnspecified() && std::abs(u.hash-t.hash) < std::abs(v.hash-t.hash)){
            //u ←DeBruijnSearch(t, −1, 0)
            call_search(u,t,sender,-1,0);
        }else{
            call_search_done(sender, t, nullhandle);
            return;
        }
    }
}

/**
 * Node result was found in the de Bruijn search
 */
void DeBruijnNode::search_done(DeBruijnNodeHandle t, DeBruijnNodeHandle result){
    if(result.handle.isUnspecified()){
        yell("SEARCH was unsuccessful");
        EV << "SEARCH for "<< t.id <<" was unsuccessful"<<endl;
    }else{
        if(result.hash != t.hash){
            yell("SEARCH result incorrect");
            EV << "SEARCH result for "<< t.hash <<" was incorrect, got "<<result.hash<<" instead"<<endl;
        }else{
            yell("SEARCH found node");
            EV << "SEARCH for "<< t.id <<" was successful"<<endl;
        }
    }
}

/**
 * Timeout method of the BuildList protocol
 */
void DeBruijnNode::buildListTimeout(){
    if(right != nullhandle){
        if(right>v){
            call_linearize(right, v);
        }else{
            call_linearize(v, right);
            right = nullhandle;
            topology_changed();
        }
    }
    if(left != nullhandle){
        if(left<v){
            call_linearize(left, v);
        }else{
            call_linearize(v, left);
            left = nullhandle;
            topology_changed();
        }
    }
}

/**
 * Timeout method of the q-neighborhood protocol
 */
void DeBruijnNode::qNeighborhoodTimeout(){
    //Downgrade closer q edges to list
    std::set<DeBruijnNodeHandle> leftdowngraders = *new std::set<DeBruijnNodeHandle>();
    std::set<DeBruijnNodeHandle> rightdowngraders = *new std::set<DeBruijnNodeHandle>();
    std::set<DeBruijnNodeHandle>::iterator it;
    for (it = edgeq.begin(); it != edgeq.end(); ++it){
        DeBruijnNodeHandle dbhandle = *it; // Note the "*" here
        //PROTOCOL CHANGE to incorporate null right/lefts
        if(!right.handle.isUnspecified()){
            if(v<dbhandle && dbhandle<right){
                rightdowngraders.insert(dbhandle);
            }
        }
        if(!left.handle.isUnspecified()){
            if(left<dbhandle && dbhandle<v){
                leftdowngraders.insert(dbhandle);
            }
        }
    }
    for (it = leftdowngraders.begin(); it != leftdowngraders.end(); ++it){
        DeBruijnNodeHandle dbhandle = *it; // Note the "*" here
        call_linearize(v, dbhandle);
    }
    for (it = rightdowngraders.begin(); it != rightdowngraders.end(); ++it){
        DeBruijnNodeHandle dbhandle = *it; // Note the "*" here
        call_linearize(v, dbhandle);
    }
    //Upgrade list edges to q-neighborhood
    if(!right.handle.isUnspecified() && edgeq.find(right) == edgeq.end()){
        topology_changed(edgeq.insert(right).second);
    }
    if(!left.handle.isUnspecified() && edgeq.find(left) == edgeq.end()){
        topology_changed(edgeq.insert(left).second);
    }
    //Pick one q neighbor in a round-robin fashion
    //PROTOCOL CHANGE only do this if there are nonzero qedges
    if(edgeq.size() >= 1){
        //round-robin check
        if(qrobinposition < 0){
            qrobinposition = edgeq.size()-1;
        }
        it = edgeq.begin();
        DeBruijnNodeHandle qkprev = nullhandle;
        DeBruijnNodeHandle qk = nullhandle;
        DeBruijnNodeHandle qknext = nullhandle;

        if(qrobinposition>0){
            std::advance(it, qrobinposition-1);
            if(it != edgeq.end()){
                qkprev = *it;
            }
            std::advance(it, 1);
        }
        if(it != edgeq.end()){
            qk = *it;
            std::advance(it, 1);
            if(it != edgeq.end()){
                qknext = *it;
            }
        }
        qrobinposition = qrobinposition - 1;

        if(!qkprev.handle.isUnspecified() && !qk.handle.isUnspecified()){
            if(qkprev>qk){
                throw cRuntimeError("qkprev bigger than qk");
            }
        }

        if(!qk.handle.isUnspecified()){
            if(qk==right){
                call_introduce(qk,v,v);
            }else if(qk==left){
                call_introduce(qk,v,v);
            }else if(v<qk){
                if(!qkprev.handle.isUnspecified()){
                    call_introduce(qk,qkprev,v);
                }
            }else if(qk<v){
                if(!qknext.handle.isUnspecified()){
                    call_introduce(qk,qknext,v);
                }
            }
        }
    }

    //try to approximate q anew
    this->approximate_q();
    //PROTOCOL CHANGE expand the edgedb vectors accordingly
    this->expand_edgedb();
}

/**
 * Timeout method of the standard de Bruijn protocol
 */
void DeBruijnNode::deBruijnTimeout(){
    //downgrade De Bruijn edges to list edges if necessary
    //PROTOCOL CHANGE do not call linearize for ourself
    if(right.handle.isUnspecified() && !edgedb.at(1).at(1).handle.isUnspecified() && edgedb.at(1).at(1) != v){
        call_linearize(v,edgedb.at(1).at(1));
    }
    if(left.handle.isUnspecified() && !edgedb.at(1).at(0).handle.isUnspecified() && edgedb.at(1).at(0) != v){
        call_linearize(v,edgedb.at(1).at(0));
    }

    //fix errors in the De Bruijn edges
    if(edgedb.at(1).at(1).handle.isUnspecified() || edgedb.at(1).at(1) < v){
        if(!edgedb.at(1).at(1).handle.isUnspecified() && edgedb.at(1).at(1) < v){
            call_linearize(v, edgedb.at(1).at(1));
        }
        //PROPOSED PROTOCOL CHANGE do we really need to do this, it will explode if v.db(1, 1) is linearized
        edgedb.at(1).at(1) = v;
        topology_changed();
    }

    if(edgedb.at(1).at(0).handle.isUnspecified() || v < edgedb.at(1).at(0)){
        if(!edgedb.at(1).at(0).handle.isUnspecified() && v < edgedb.at(1).at(0)){
            call_linearize(v, edgedb.at(1).at(0));
        }
        edgedb.at(1).at(0) = v;
        topology_changed();
    }

    //probe for left and right De Bruijn nodes via the list
    //PROTOCOL CHANGE do not call this on empty nodes
    if(!left.handle.isUnspecified()){
        call_probe(left, v, v.hash/2.0L, "leftDB");

    }
    if(!right.handle.isUnspecified()){
        call_probe(right, v, (v.hash+1.0L)/2.0L, "rightDB");
    }
}

/**
 * Timeout method of the general de Bruijn protocol
 */
void DeBruijnNode::generalDeBruijnTimeout(){
    for(int i = 2; i<=log2(q)+1; i++){
        for(int j = 0; j<=pow(2.0,i)-1; j++){
            DeBruijnNodeHandle nh = edgedb.at(i).at(j);
            //insert general de Bruijn edges into list if list needs them
            if(right.handle.isUnspecified() && !nh.handle.isUnspecified() && v < nh){
                call_linearize(v, nh);
            }
            //downgrade v.left analogously
            if(left.handle.isUnspecified() && !nh.handle.isUnspecified() && nh < v){
                call_linearize(v, nh);
            }
            //downgrade incorrect general de Bruijn edges
            if(nh.handle.isUnspecified() || ((v.hash+(long double) j)/(pow(2.0,i))<v.hash && v<nh)){
                if(!nh.handle.isUnspecified()){
                    call_linearize(v,nh);
                }
                edgedb.at(i).at(j) = v;
            }
        }
    }

    //Pick i ∈ {2, ... , log2(v.q)} and j ∈ {0, ... , 2^i − 1} in a round-robin fashion
    if(dbrobiniposition>log2(q)+1){
        dbrobiniposition = 2;
    }
    if(dbrobinjposition>pow(2.0,dbrobiniposition)-1){
        dbrobinjposition = -1; //will get incremented immediately
    }
    dbrobinjposition++;
    if(dbrobinjposition>pow(2.0,dbrobiniposition)-1){
        dbrobinjposition = 0;
        dbrobiniposition++;
    }
    if(dbrobiniposition>log2(q)+1){
        dbrobiniposition = 2;
    }
    int i = dbrobiniposition;
    int j = dbrobinjposition;
    //Checking whether i and j are valid indices
    if(!(i>=2 && i<=log2(q)+1 && j>=0 && j<=pow(2.0,i)-1)){
        std::stringstream ttString;
        ttString << v.id <<": general de Bruijn timeout round robin went wrong - ";
        if(i<2){
            ttString << "i="<<i<<" too small for 2";
        }
        if(i>std::max(log2(q),2.0)+1){
            ttString << "i="<<i<<" too big for "<<log2(q)+1;
            throw cRuntimeError(ttString.str().c_str());
        }
        if(j<0){
            ttString << "j="<<j<<" too small for 0";
        }
        if(j>pow(2.0,i)-1){
            ttString << "j="<<j<<" too big for "<<pow(2.0,i)-1;
            throw cRuntimeError(ttString.str().c_str());
        }
        EV << ttString.str().c_str();
    }

    // Call General Probe on the previously selected node
    DeBruijnNodeHandle target = edgedb.at(i-1).at(j % (int) floor(pow(2.0, i-1)));
    if(!target.handle.isUnspecified()){
        call_general_probe(target, v, (v.hash + j)/pow(2.0,i), i, j, 1);
    }
}

/**
 * LINEARIZE method of the BuildList protocol
 */
void DeBruijnNode::linearize(DeBruijnNodeHandle u){
    EV << v.id << ": LINEARIZING node " << u.id << endl;
    //PROTOCOL CHANGE Handling empty left and right nodes
    if(right.handle.isUnspecified() && v<u){
        EV << v.id << ": Setting right = " << u.id << endl;
        right = u;
        topology_changed();
        return;
    }
    if(left.handle.isUnspecified() && u<v){
        EV << v.id << ": Setting left = " << u.id << endl;
        left = u;
        topology_changed();
        return;
    }

    //checking all different position cases
    //PROTOCOL CHANGE Handling empty left and right nodes
    if(!right.handle.isUnspecified()){
        if(right<u){
            EV << v.id << ": DELEGATING " << u.id << " to the far right" << endl;
            call_delegate(v, u);
        }
        if(v<u && u<right){
            EV << v.id << ": LINEARIZING our right neighbor to " << u.id << endl;
            call_linearize(u, right);
            EV << v.id << ": Setting right = " << u.id << endl;
            right = u;
            topology_changed();
        }
    }
    if(!left.handle.isUnspecified()){
        if(left<u && u<v){
            EV << v.id << ": LINEARIZING our left neighbor to " << u.id << endl;
            call_linearize(u, left);
            EV << v.id << ": Setting left = " << u.id << endl;
            left = u;
            topology_changed();
        }
        if(u<left){
            EV << v.id << ": DELEGATING " << u.id << " to the far left" << endl;
            call_delegate(v, u);
        }
    }
}

/**
 * DELEGATE method of the BuildList protocol
 */
void DeBruijnNode::delegate(DeBruijnNodeHandle u){
    //PROTOCOL CHANGE Upgrade list edges to q-neighborhood
    if(!right.handle.isUnspecified() && edgeq.find(right) == edgeq.end()){
        topology_changed(edgeq.insert(right).second);
    }
    if(!left.handle.isUnspecified() && edgeq.find(left) == edgeq.end()){
        topology_changed(edgeq.insert(left).second);
    }

    EV << v.id << ": Trying to delegate " << u.id <<"("<< u.hash << ")" << endl;
    DeBruijnNodeHandle qmin = nullhandle;
    long double diff = 2.0;
    std::set<DeBruijnNodeHandle>::iterator it;
    for (it = edgeq.begin(); it != edgeq.end(); ++it)
    {
        DeBruijnNodeHandle dbhandle = *it; // Note the "*" here
        //PROTOCOL CHANGE only consider other nodes between us and the target
        if((v<dbhandle && dbhandle<u) || (u<dbhandle && dbhandle<v)){
            long double newdifference = std::abs(dbhandle.hash - u.hash);
            if(qmin.handle.isUnspecified()){
                EV << v.id << ": Choosing " << dbhandle.id <<"("<< dbhandle.hash << ") because it is the first one" << endl;
                qmin = dbhandle;
                diff = newdifference;
                EV << v.id << ": New diff " << diff << endl;
            }else{
                if(newdifference < diff){
                    EV << v.id << ": Choosing " << dbhandle.id <<"("<< dbhandle.hash << ") because it is nearer than "<<qmin.hash  << endl;
                    qmin = dbhandle;
                    diff = newdifference;
                    EV << v.id << ": New diff " << diff << endl;
                }else{
                    EV << v.id << ": NOT Choosing " << dbhandle.id <<"("<< dbhandle.hash << ") because it is further away than "<<qmin.hash << endl;
                }
            }
        }
    }
    EV << v.id << ": DELEGATING node " << u.id << " to node " << qmin.id << endl;
    //PROTOCOL CHANGE to ensure no reference is lost
    if(!qmin.handle.isUnspecified()){
        call_linearize(qmin, u);
    }else{
        topology_changed(edgeq.insert(u).second);
    }
}

/**
 * INTRODUCE method of the q-neighborhood protocol
 */
void DeBruijnNode::introduce(DeBruijnNodeHandle qtilde, DeBruijnNodeHandle sender){
    //insert qtilde directly
    topology_changed(edgeq.insert(qtilde).second);

    int maxqsize = c*2*q;
    if(edgeq.size()>maxqsize){
        EV << v.id << ": Reducing q-neighborhood because " << edgeq.size() << " neighbors are too many (limit is "<<maxqsize<<")" << endl;
        //calculate how many neighbors to remove
        int tooMany = edgeq.size()-maxqsize;

        for(int i = 1; i<=tooMany; i++){
            //Finding the neighbor qmax that is furthest away from us
            DeBruijnNodeHandle qmax = closest_edgeq(v.hash,false);

            //remove qmax from the q neighborhood
            topology_changed(edgeq.erase(qmax) > 0);
            EV << v.id << ": LINEARIZING and erasing node " << qmax.id  <<" from q-neighborhood"<< endl;
            call_linearize(v,qmax);
        }
    }
    //introduce our list neighbors to the sender
    if(!sender.handle.isUnspecified()){
        if(sender < v){
            if(!right.handle.isUnspecified()){
                call_introduce(sender, right, nullhandle);
            }
        }else{
            if(!left.handle.isUnspecified()){
                call_introduce(sender, left, nullhandle);
            }
        }
    }
}

/**
 * APPROXIMATE_Q method of the q-neighborhood protocol
 */
void DeBruijnNode::approximate_q(){
    //PROTOCOL CHANGE only do this if q contains more than 2 nodes, otherwise our distance calculations are incorrect
    if(edgeq.size()>=2){
        double min_value = DBL_MAX;
        int min_key = 0;
        std::vector<DeBruijnNodeHandle> qvector(edgeq.begin(),edgeq.end());
        for(int i=-floor(log2(q)); i<=1; i++){
            int first = 1;
            int last = ceil(exp2(i)*q);
            //PROTOCOL CHANGE do not overrun array boundaries
            if(last>qvector.size()){
                last = qvector.size();
            }
            long double diff = std::abs(qvector.at(first-1).hash - qvector.at(last-1).hash);
            double value = std::abs(exp2(d) * diff - pow(1.0/(exp2(i)*q), d-1));
            if(value < min_value){
                min_value = value;
                min_key = i;
            }
        }
        if(min_key!=0){
            q = q * exp2(min_key);
            EV << v.id << ": New q value of " << q  << endl;
        }
    }else{
        int nguess = 3;
        q=ceil(pow(nguess,1.0/d));
    }
}

/**
 * expanding edgedb to the size mandated by q
 */
void DeBruijnNode::expand_edgedb(){
    for(int i = 1; i<=log2(std::max(q, 2.0))+2; i++){
        if(i>=edgedb.size()){
            //size is last element +1
            edgedb.insert(edgedb.end(), *new std::vector<DeBruijnNodeHandle>());
        }
        for(int j = 0; j<=pow(2.0,i)-1; j++){
            if(j>=edgedb.at(i).size()){
                edgedb.at(i).insert(edgedb.at(i).end(), nullhandle);
            }
        }
    }
}

/**
 * APPROXIMATE_LOG_N method of the q-neighborhood protocol
 * TODO: IMPLEMENT
 */
void DeBruijnNode::approximate_log_n(){
/*
Approximate_log_N
    for i ∈ { 1/2 v.q, ... , 2v.q} do
        a_i ← |2^d · |q_1 − q_i | − (1/i)^d−1 |
    k ← argmini∈{ 1/2 v.q, ... ,2v.q} {a_i}
    return floor(log((2k)^d))
*/
}

/**
 * PROBE method of the standard de Bruijn protocol
 */
void DeBruijnNode::probe(DeBruijnNodeHandle sender, long double t, std::string mode){
    EV << v.id << ": Received a " << mode << " probe for "<< t << endl;
    if(mode == "rightDB"){
        if(edgedb.at(1).at(1).handle.isUnspecified()){
            //Sorry, the number you dialed could not be reached. Please try again.
            call_probe_done(sender, t, v);
        }else{
            //do the De Bruijn hop
            call_probe(edgedb.at(1).at(1), sender, t, "dbh_done");
        }
    }else if(mode == "leftDB"){
        if(edgedb.at(1).at(0).handle.isUnspecified()){
            call_probe_done(sender, t, v);
        }else{
            call_probe(edgedb.at(1).at(0), sender, t, "dbh_done");
        }
    }else if(mode == "dbh_done"){
        //find the closest q-neighbor to t
        DeBruijnNodeHandle u = this->closest_edgeq(t, true);
        //PROTOCOL CHANGE check u for nullhandle
        if(!u.handle.isUnspecified() && std::abs(u.hash - t) < std::abs(v.hash - t)){
            call_probe(u, sender, t, "dbh_done");
        }else{
            call_probe_done(sender, t, v);
        }
    }
}

/**
 * PROBE_DONE method of the standard de Bruijn protocol
 */
void DeBruijnNode::probe_done(DeBruijnNodeHandle t, DeBruijnNodeHandle result){
    if(v < t){
        if(result != edgedb.at(1).at(1) && !edgedb.at(1).at(1).handle.isUnspecified()){
            //we have to get rid of the old De Bruijn edge
            call_linearize(v, edgedb.at(1).at(1));
        }
        //save the new edge
        edgedb.at(1).at(1) = result;
    }
    //analogously for the left side
    if(t < v){
        if(result != edgedb.at(1).at(0) && !edgedb.at(1).at(0).handle.isUnspecified()){
            call_linearize(v, edgedb.at(1).at(0));
        }
        edgedb.at(1).at(0) = result;
    }
}

/**
 * GENERAL_PROBE method of the general de Bruijn protocol
 */
void DeBruijnNode::general_probe(DeBruijnNodeHandle sender, long double t, int i, int j, bool dB){
    //if dB = 1 then
    if(dB){
        bool leftcondition = edgedb.at(1).at(0).handle.isUnspecified() && j < pow(2.0,i-1);
        bool rightcondition = edgedb.at(1).at(1).handle.isUnspecified() && j >= pow(2.0,i-1);
        if(leftcondition||rightcondition){
            call_general_probe_done(sender, v, i, j);
            return;
        }
        if(j < pow(2.0, i-1)){
            if(!edgedb.at(1).at(0).handle.isUnspecified()){
                call_general_probe(edgedb.at(1).at(0), sender, t, i, j, false);
            }else{
                //PROTOCOL CHANGE do not throw away the sender handle
                call_linearize(v, sender);
            }
        }else{
            if(!edgedb.at(1).at(1).handle.isUnspecified()){
                call_general_probe(edgedb.at(1).at(1), sender, t, i, j, false);
            }else{
                //PROTOCOL CHANGE do not throw away the sender handle
                call_linearize(v, sender);
            }
        }
    }else{
        DeBruijnNodeHandle u = closest_edgeq(t, true);

        if(sender.hash == v.hash){
            call_general_probe_done(sender, v, i, j);
        }else{
            if(!u.handle.isUnspecified()){
                //PROTOCOL CHANGE reply directly if we are the closest q neighbor
                if(std::abs(t-v.hash)<std::abs(t-u.hash)){
                    call_general_probe_done(sender, v, i, j);
                }else{
                    call_general_probe(u, sender, t, i, j, false);
                }
            }else{
                //PROTOCOL CHANGE do not throw away the sender handle
                call_linearize(v, sender);
            }
        }
    }
}

/**
 * GENERAL_PROBE_DONE method of the general de Bruijn protocol
 */
void DeBruijnNode::general_probe_done(DeBruijnNodeHandle result, int i, int j){
    if(result.hash != edgedb.at(i).at(j).hash){
        if(!edgedb.at(i).at(j).handle.isUnspecified()){
            call_linearize(v,edgedb.at(i).at(j));
        }
        edgedb.at(i).at(j) = result;
        topology_changed();
    }
}

/**
 * Message generation wrapper for LINEARIZE calls
 */
void DeBruijnNode::call_linearize(DeBruijnNodeHandle target, DeBruijnNodeHandle content){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    msg->setU(content);
    this->callFunction(target, LINEARIZE, msg);
}

/**
 * Message generation wrapper for DELEGATE calls
 */
void DeBruijnNode::call_delegate(DeBruijnNodeHandle target, DeBruijnNodeHandle content){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    msg->setU(content);
    this->callFunction(target, DELEGATE, msg);
}

/**
 * Message generation wrapper for INTRODUCE calls
 */
void DeBruijnNode::call_introduce(DeBruijnNodeHandle target, DeBruijnNodeHandle qtilde, DeBruijnNodeHandle sender){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    msg->setQtilde(qtilde);
    msg->setSender(sender);
    this->callFunction(target, INTRODUCE, msg);
}

/**
 * Message generation wrapper for PROBE calls
 */
void DeBruijnNode::call_probe(DeBruijnNodeHandle target, DeBruijnNodeHandle sender, long double t, std::string mode){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    DeBruijnNodeHandle* tnode = new DeBruijnNodeHandle();
    DeBruijnNodeHandle thandle = *tnode;
    thandle.hash = t;
    thandle.handle = NodeHandle::UNSPECIFIED_NODE;
    msg->setT(thandle);
    msg->setSender(sender);
    msg->setMode(mode.c_str());
    this->callFunction(target, PROBE, msg);
}

/**
 * Message generation wrapper for PROBE_DONE calls
 */
void DeBruijnNode::call_probe_done(DeBruijnNodeHandle target, long double t, DeBruijnNodeHandle result){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    DeBruijnNodeHandle* tnode = new DeBruijnNodeHandle();
    DeBruijnNodeHandle thandle = *tnode;
    thandle.hash = t;
    thandle.handle = NodeHandle::UNSPECIFIED_NODE;
    msg->setT(thandle);
    msg->setResult(result);
    this->callFunction(target, PROBE_DONE, msg);
}

/**
 * Message generation wrapper for GENERAL_PROBE calls
 */
void DeBruijnNode::call_general_probe(DeBruijnNodeHandle target, DeBruijnNodeHandle sender, long double t, int i, int j, bool dB){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    DeBruijnNodeHandle* tnode = new DeBruijnNodeHandle();
    DeBruijnNodeHandle thandle = *tnode;
    thandle.hash = t;
    thandle.handle = NodeHandle::UNSPECIFIED_NODE;
    msg->setT(thandle);
    msg->setSender(sender);
    msg->setI(i);
    msg->setJ(j);
    msg->setDB(dB);
    this->callFunction(target, GENERAL_PROBE, msg);
}

/**
 * Message generation wrapper for GENERAL_PROBE_DONE calls
 */
void DeBruijnNode::call_general_probe_done(DeBruijnNodeHandle target, DeBruijnNodeHandle result, int i, int j){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    msg->setResult(result);
    msg->setI(i);
    msg->setJ(j);
    this->callFunction(target, GENERAL_PROBE_DONE, msg);
}

/**
 * Message generation wrapper for SEARCH calls
 */
void DeBruijnNode::call_search(DeBruijnNodeHandle target, DeBruijnNodeHandle t, DeBruijnNodeHandle sender, double r, int remHops){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    msg->setT(t);
    msg->setSender(sender);
    msg->setR(r);
    msg->setRemHops(remHops);
    this->callFunction(target, SEARCH, msg);
}

/**
 * Message generation wrapper for SEARCH_DONE calls
 */
void DeBruijnNode::call_search_done(DeBruijnNodeHandle target, DeBruijnNodeHandle t, DeBruijnNodeHandle result){
    GeneralDeBruijnMessage* msg = new GeneralDeBruijnMessage;
    msg->setResult(result);
    msg->setT(t);
    this->callFunction(target, SEARCH_DONE, msg);
}

/**
 * Utility method for edgeq searches
 */
DeBruijnNodeHandle DeBruijnNode::closest_edgeq(long double t, bool findClosest){
    DeBruijnNodeHandle u = nullhandle;
    long double diff = 0.0;
    if(findClosest){
        diff=2.0;
    }

    std::set<DeBruijnNodeHandle>::iterator it;
    for (it = edgeq.begin(); it != edgeq.end(); ++it)
    {
        DeBruijnNodeHandle dbhandle = *it;
        long double newdifference = std::abs(dbhandle.hash - t);
        if(u.handle.isUnspecified()){
            u = dbhandle;
            diff = newdifference;
        }else{
            if(findClosest){
                if(newdifference < diff){
                    u = dbhandle;
                    diff = newdifference;
                }
            }else{
                if(newdifference > diff){
                    u = dbhandle;
                    diff = newdifference;
                }
            }
        }
    }
    return u;
}

/**
 * Utility method for edgedb searches
 */
DeBruijnNodeHandle DeBruijnNode::closest_edgedb(long double t, bool findClosest){
    DeBruijnNodeHandle u = nullhandle;
    long double diff = 0.0;
    if(findClosest){
        diff=2.0;
    }

    for(int i = 1; i<=log2(q); i++){
        for(int j = 0; j<=pow(2.0,i)-1; j++){
            DeBruijnNodeHandle dbhandle = edgedb.at(i).at(j);
            long double newdifference = std::abs(dbhandle.hash - t);
            if(u.handle.isUnspecified()){
                u = dbhandle;
                diff = newdifference;
            }else{
                if(findClosest){
                    if(newdifference < diff){
                        u = dbhandle;
                        diff = newdifference;
                    }
                }else{
                    if(newdifference > diff){
                        u = dbhandle;
                        diff = newdifference;
                    }
                }
            }
        }
    }
    return u;
}

