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

// @file TreeManagementMessage.msg
// @author Daniel Lienert
//

cplusplus {{
#include <vector>

#include <NodeHandle.h>
#include <TransportAddress.h>
#include <CommonMessages_m.h>

#include <CoordDataContainer.h>
#include <SimpleCoordDataContainer.h>
#include <RegionDataContainer.h>
#include <AreaDataContainer.h>

// constants for message length in bit
static const int NODECOUNT_L = 8;

#define TREETESTSTRATEGYCALL_L(msg) NODECOUNT_L

#define PARENTREQUESTCALL_L(msg) BASECALL_L(msg)
#define PARENTREQUESTRESPONSE_L(msg) BASERESPONSE_L(msg) 

#define CHILDRELEASECALL_L(msg) BASECALL_L(msg)
#define CHILDRELEASERESPONSE_L(msg) BASERESPONSE_L(msg)

#define CHILDCHECKCALL_L(msg) BASECALL_L(msg)
#define CHILDCHECKRESPONSE_L(msg) BASERESPONSE_L(msg)

#define TREEAPPCALL_L(msg) BASECALL_L(msg)
#define TREEAPPRESPONSE_L(msg) BASERESPONSE_L(msg)

#define GLOBALVIEWBUILDERCALL_L(msg) TREEAPPCALL_L(msg)
#define GLOBALVIEWBUILDERRESPONSE_L(msg) TREEAPPRESPONSE_L(msg)

#define SENDCOORDINATESCALL_L(msg) (GLOBALVIEWBUILDERCALL_L(msg) + msg->getCoordData().getBitLength()) 
#define SENDALLSTRATEGYCALL_L(msg) SENDCOORDINATESCALL_L(msg)
#define REMOVERANDOMSTRATEGYCALL_L(msg) SENDCOORDINATESCALL_L(msg)
#define REGIONSSTRATEGYCALL_L(msg) (GLOBALVIEWBUILDERCALL_L(msg) + msg->getRegionData().getBitLength())
#define SIMPLECOORDSTRATEGYCALL_L(msg) (GLOBALVIEWBUILDERCALL_L(msg) + msg->getCoordData().getBitLength())
}}

class BaseCallMessage;
class BaseResponseMessage;

class noncobject CoordDataContainer;
class noncobject RegionDataContainer;
class noncobject SimpleCoordDataContainer;
class noncobject AreaDataContainer;
class noncobject NodeHandle;
class noncobject OverlayKey;

packet ParentRequestCall extends BaseCallMessage 
{
    OverlayKey domainKey;
}

packet ParentRequestResponse extends BaseResponseMessage 
{
}	


packet TreeAppCall extends BaseCallMessage
{
}

packet TreeAppResponse extends BaseResponseMessage
{
}

packet GlobalViewBuilderCall extends TreeAppCall 
{
}

packet GlobalViewBuilderResponse extends TreeAppResponse
{
}


packet ChildReleaseCall extends BaseCallMessage 
{
    //NodeHandle newParent;         
}

packet ChildReleaseResponse extends BaseResponseMessage 
{
    //NodeHandle newParent;         
}


packet ChildCheckCall extends BaseCallMessage 
{
    //NodeHandle newParent;         
}

packet ChildCheckResponse extends BaseResponseMessage 
{
    //NodeHandle newParent;         
}


packet SendCoordinatesCall extends GlobalViewBuilderCall 
{
    CoordDataContainer coordData;
}

packet SendAllStrategyCall extends GlobalViewBuilderCall 
{
    CoordDataContainer coordData;
}

packet RemoveRandomStrategyCall extends GlobalViewBuilderCall 
{ 
    CoordDataContainer coordData;
}

packet TreeTestStrategyCall extends GlobalViewBuilderCall 
{
    int nodeCount;
}

packet RegionsStrategyCall extends GlobalViewBuilderCall 
{
    RegionDataContainer regionData;
}

packet SimpleCoordStrategyCall extends GlobalViewBuilderCall 
{
    SimpleCoordDataContainer coordData;
}


packet AreaDataCall extends GlobalViewBuilderCall 
{
    AreaDataContainer areaData;
}


packet CapReqCall extends BaseCallMessage
{
    
}

packet CapReqResponse extends BaseResponseMessage
{
    AreaDataContainer areaData;
}