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

//
// @file ALMTestTracedMessage.msg
// @author Dimitar Toshev
//

cplusplus {{
#include "common/OverlayKey.h"
}}

class noncobject OverlayKey;

packet ALMTestTracedMessage
{
    // Group to which the message is sent.
    OverlayKey groupId;

    // identifies a message and all its copies
    long mcastId;

    // Set before sending.
    // Module id of sender.
    int senderId;

    // Set after receiving.
    // Module id of receiver.
    int receiverId;
}

