//
// Copyright (C) 2019 OpenSim Ltd.
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

#ifndef __OVERSIM_DEFS_H_
#define __OVERSIM_DEFS_H_

#include "inet/common/INETDefs.h"

#if OMNETPP_VERSION < 0x0501 || OMNETPP_BUILDNUM < 1010
#  error At least OMNeT++/OMNEST version 5.1 required
#endif // if OMNETPP_VERSION < 0x0501

#if INET_VERSION < 0x0306 || (INET_VERSION == 0x0306 && INET_PATCH_LEVEL < 0x05) || INET_VERSION > 0x0310
#  error At least INET version 3.6.5 required from INET version 3.x family
#endif // INET_VERSION

#endif  // __OVERSIM_DEFS_H_
