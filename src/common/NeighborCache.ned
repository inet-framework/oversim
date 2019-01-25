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

// @file NeighborCache.ned
// @author Bernhard Heep
//

package oversim.common;

//
// Module for collecting neighborhood information (rtt, coordinates, ...)
// @author Bernhard Heep
// @author Antonio Zea
//
simple NeighborCache extends BaseApp
{
    parameters:
        @class(NeighborCache);
        bool enableNeighborCache;  // use neighbor cache for reducing ping traffic?
        double rttExpirationTime @unit(s); // expiration of the validity of the measured rtts
        double maxSize;           // maximum size of the cache

        int rttHistory;
        double timeoutAccuracyLimit;

        string defaultQueryType;
        string defaultQueryTypeI;
        string defaultQueryTypeQ;


        bool doDiscovery;
        int collectClosestNodes;

        string ncsType;
        bool useNcsForTimeout;
        bool ncsPiggybackOwnCoords;

        //discovery mode settings
        int discoveryModeNumCloseNodes;
        int discoveryModeNumSpreadedNodes;
        int discoveryModeMaxSteps;
        int discoveryModeSpreadedSteps;

        //gnp settings
        int gnpDimensions;
        int gnpCoordCalcRuns;
        double gnpLandmarkTimeout @unit(s);

        //nps settings
        int npsMaxLayer;

        //vivaldi settings
        bool vivaldiShowPosition;
        double vivaldiErrorConst;
        double vivaldiCoordConst;
        double vivaldiDimConst;
        bool vivaldiEnableHeightVector;

        //svivaldi settings
        double svivaldiLossConst;
        double svivaldiLossResetLimit;

        //simpleNcs settings
        string simpleNcsDelayFaultType;

        // Tree Management Settings
        bool treeMgmtEnableTreeManagement;       // enable the tree Management System
        double treeMgmtBuildInterval @unit(s);   // the interval to check the childNodeMap
        double treeMgmtChildrenTimeOut @unit(s); // after this timeout an alive check ping is send 

        // Global View Builder Settings
        bool gvbEnableGlobalViewBuilder;        // enable the Global View Builder
        double gvbCoordSendInterval @unit(s);   // the interval to send the coordinates data
        bool gvbOnlyAcceptCompleteCCD;

        string gvbSendStrategy;                 // the send strategy to use. (sendAll|simplifyCoords|removeRandom|removeInaccurate|regions|treeTest)
        string gvbStrategyRemoveCoordsMode;     // strategy mode (percentage|limitCoords|limitTraffic)
        int gvbStrategyRemoveCoordsPercentage;
        int gvbStrategyRemoveCoordsCoordsLimit;
        int gvbStrategyRemoveCoordsTrafficLimit;
        int gvbStrategyRegionsSizePerDimension;
        int gvbStrategyRegionsMaxSpread;
        int gvbStrategySimplifyCoordsFactor;

        string gvbC2AParameter;                 // parameterstring to append to c2a.rb call
        bool gvbDistributeGlobalView;           // create areas and distribute the globalView to all nodes 

        @display("i=block/table");
}
