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
 * @file GeneralDeBruijnMessage.msg
 * @author Jan Peter Drees
 */

// c++ code to include BaseOverlayMessage (is defined in CommonMessages_m.h)
cplusplus {{
#include <CommonMessages_m.h>
#include <stdlib.h>
#include <DeBruijnNodeHandle.h>
#include <cmath>
}}

// the Superclass
class BaseOverlayMessage;
class noncobject DeBruijnNodeHandle;

//
// A Message containing all different deBruijn calls and their respective parameters
//
packet GeneralDeBruijnMessage extends BaseOverlayMessage {
    int command;
    DeBruijnNodeHandle t;
    double r;
    int remHops;
    DeBruijnNodeHandle u;
    DeBruijnNodeHandle qtilde;
    DeBruijnNodeHandle sender;
    DeBruijnNodeHandle result;
    string mode;
    int i;
    int j;
    bool dB;
}
