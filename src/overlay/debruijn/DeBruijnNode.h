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
 * @file DeBruijnNode.h
 * @author Jan Peter Drees
 */

#ifndef __OVERSIM_DEBRUIJNNODE_H_
#define __OVERSIM_DEBRUIJNNODE_H_

#include <omnetpp.h>
#include "SimpleOverlayNode.h"
#include "CheersMessage_m.h"
#include "SimplePresentationMessage_m.h"
#include "GeneralDeBruijnMessage_m.h"
#include "SimpleOverlayUtils.h"
#include "GlobalNodeList.h"
#include <vector>
#include "DeBruijnNodeHandle.h"
#include <string>
#include <stdlib.h>

/**
 * This class represents a de bruijn overlay node.
 */
class DeBruijnNode: public SimpleOverlayNode {

protected:
    cOutVector topologyChangedVec;

public:

    int k;

    int d;

    int c;

    double q;

    int qrobinposition;

    int dbrobiniposition;

    int dbrobinjposition;

    DeBruijnNodeHandle v;

    DeBruijnNodeHandle left;

    DeBruijnNodeHandle right;

    DeBruijnNodeHandle nullhandle;

    std::set<DeBruijnNodeHandle> edgeq;

    std::vector<std::vector<DeBruijnNodeHandle> > edgedb;

    std::string icon;

    int visnode;

    bool darkcolors;

    int generaldbsize;

    bool showq;
    bool showlist;
    bool showdb;
    bool showgeneraldb;

    int nodeNumber;


    /**
     * Gets called when the node is initialized.
     */
    void onInitialize();

    /**
     * Gets called continuously during simulation.
     */
    void onTimeout();

    /**
     * Gets called when a message for the Overlay Protocol is received.
     */
    void onMessageReceived(BaseOverlayMessage* msg);

    /**
     * Send the GeneralDeBruijnMessage msg with the procedure call command to the node destination
     */
    void callFunction(DeBruijnNodeHandle destination, int command, GeneralDeBruijnMessage* msg);

    /**
     * Document all topology changes. Takes an optional boolean argument of whether it actually changed.
     */
    void topology_changed(bool did_change);

    /**
     * Updates the GUI.
     */
    void updateTooltip();

    /**
     * Updates status icon.
     */
    void updateIcon();

    /**
     * Draws arrows to all neighbors in the GUI.
     */
    void drawAllNeighborConnections();

    /**
     * Draws arrows for all edges in the given set, using the color provided.
     */
    void drawSetNeighborConnections(std::set<DeBruijnNodeHandle> set, std::string color);

    /**
     * Searches for a node with the ID t
     */
    void deBruijnSearch(DeBruijnNodeHandle t, DeBruijnNodeHandle sender, double r, int remHops);

    /**
     * Node result was found in the de Bruijn search
     */
    void search_done(DeBruijnNodeHandle t, DeBruijnNodeHandle result);

    /**
     * Timeout method of the BuildList protocol
     */
    void buildListTimeout();

    /**
     * Timeout method of the q-neighborhood protocol
     */
    void qNeighborhoodTimeout();

    /**
     * Timeout method of the standard de Bruijn protocol
     */
    void deBruijnTimeout();

    /**
     * Timeout method of the general de Bruijn protocol
     */
    void generalDeBruijnTimeout();

    /**
     * LINEARIZE method of the BuildList protocol
     */
    void linearize(DeBruijnNodeHandle u);

    /**
     * DELEGATE method of the BuildList protocol
     */
    void delegate(DeBruijnNodeHandle u);

    /**
     * INTRODUCE method of the q-neighborhood protocol
     */
    void introduce(DeBruijnNodeHandle qtilde, DeBruijnNodeHandle sender);

    /**
     * APPROXIMATE_Q method of the q-neighborhood protocol
     */
    void approximate_q();

    /**
     * Expanding edgedb to the size mandated by q
     */
    void expand_edgedb();

    /**
     * APPROXIMATE_LOG_N method of the q-neighborhood protocol
     */
    void approximate_log_n();

    /**
     * PROBE method of the standard de Bruijn protocol
     */
    void probe(DeBruijnNodeHandle sender, long double t, std::string mode);

    /**
     * PROBE_DONE method of the standard de Bruijn protocol
     */
    void probe_done(DeBruijnNodeHandle t, DeBruijnNodeHandle result);

    /**
     * GENERAL_PROBE method of the general de Bruijn protocol
     */
    void general_probe(DeBruijnNodeHandle sender,long double t, int i, int j, bool dB);

    /**
     * GENERAL_PROBE_DONE method of the general de Bruijn protocol
     */
    void general_probe_done(DeBruijnNodeHandle result, int i, int j);

    /**
     * Message generation wrapper for LINEARIZE calls
     */
    void call_linearize(DeBruijnNodeHandle target, DeBruijnNodeHandle content);

    /**
     * Message generation wrapper for DELEGATE calls
     */
    void call_delegate(DeBruijnNodeHandle target, DeBruijnNodeHandle content);

    /**
     * Message generation wrapper for INTRODUCE calls
     */
    void call_introduce(DeBruijnNodeHandle target, DeBruijnNodeHandle qtilde, DeBruijnNodeHandle sender);

    /**
     * Message generation wrapper for PROBE calls
     */
    void call_probe(DeBruijnNodeHandle target, DeBruijnNodeHandle sender, long double t, std::string mode);

    /**
     * Message generation wrapper for PROBE_DONE calls
     */
    void call_probe_done(DeBruijnNodeHandle target, long double t, DeBruijnNodeHandle result);

    /**
     * Message generation wrapper for GENERAL_PROBE calls
     */
    void call_general_probe(DeBruijnNodeHandle target, DeBruijnNodeHandle sender, long double t, int i, int j, bool dB);

    /**
     * Message generation wrapper for GENERAL_PROBE_DONE calls
     */
    void call_general_probe_done(DeBruijnNodeHandle target, DeBruijnNodeHandle result, int i, int j);

    /**
     * Message generation wrapper for SEARCH calls
     */
    void call_search(DeBruijnNodeHandle target, DeBruijnNodeHandle t, DeBruijnNodeHandle sender, double r, int remHops);

    /**
     * Message generation wrapper for SEARCH_DONE calls
     */
    void call_search_done(DeBruijnNodeHandle target, DeBruijnNodeHandle t, DeBruijnNodeHandle result);

    /**
     * Utility method for edgeq searches
     */
    DeBruijnNodeHandle closest_edgeq(long double t, bool findClosest);

    /**
     * Utility method for edgedb searches
     */
    DeBruijnNodeHandle closest_edgedb(long double t, bool findClosest);

};

#endif
