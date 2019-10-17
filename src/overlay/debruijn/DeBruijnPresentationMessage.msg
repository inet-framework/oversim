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
 * @file DeBruijnPresentationMessage.msg
 * @author Jan Peter Drees
 */

// c++ code to include NodeHandle and BaseOverlayMessage (is defined in CommonMessages_m.h)
cplusplus {{
#include <CommonMessages_m.h>
#include <DeBruijnNodeHandle.h>
}}

// forward declare these classes
class noncobject DeBruijnNodeHandle;
class BaseOverlayMessage;

// 
// A message class used for simple presentations.
// It holds a NodeHandle of the node which should be presented.
//
packet DeBruijnPresentationMessage extends BaseOverlayMessage {
    DeBruijnNodeHandle content;
}
