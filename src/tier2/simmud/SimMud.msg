//
// Copyright (C) 2006 Institut fuer Telematik, Universitaet Karlsruhe (TH)
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

// @file SimMud.msg
// @author Stephan Krause
//

cplusplus {{
#include "common/NodeHandle.h"
#include "common/CommonMessages_m.h"

#define SIMMUD_MOVE_L(msg) ( NODEHANDLE_L + 2*sizeof(double) + 8 )
}}



class noncobject NodeHandle;

packet SimMudMoveMessage
{
        NodeHandle src;
        double posX;
        double posY;
        simtime_t timestamp;
        bool leaveRegion = false;
}

