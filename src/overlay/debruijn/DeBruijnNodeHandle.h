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
 * @file DeBruijnNodeHandle.h
 * @author Jan Peter Drees
 */

#ifndef DEBRUIJNNODEHANDLE_H_
#define DEBRUIJNNODEHANDLE_H_

#include "GlobalNodeList.h"

class DeBruijnNodeHandle {
public:
    long double hash;
    int id;
    NodeHandle handle;

    DeBruijnNodeHandle();
    virtual ~DeBruijnNodeHandle();

    void setId(int setid);
    long double getHash() const;
    long double getHashFromID(int ID);
    bool operator <(const DeBruijnNodeHandle& o) const;
    bool operator >(const DeBruijnNodeHandle& o) const;
    bool operator ==(const DeBruijnNodeHandle& o) const;
    bool operator !=(const DeBruijnNodeHandle& o) const;
};

#endif /* DeBruijnNodeHandle_H_ */
