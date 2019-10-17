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
 * @file DeBruijnNodeHandle.cc
 * @author Jan Peter Drees
 */

#include "DeBruijnNodeHandle.h"
#include <limits>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdint.h>
#include <cmath>

DeBruijnNodeHandle::DeBruijnNodeHandle() {
    // TODO Auto-generated constructor stub
}

DeBruijnNodeHandle::~DeBruijnNodeHandle() {
    // TODO Auto-generated destructor stub
}

void DeBruijnNodeHandle::setId(int setid){
    this->id = setid;
    this->hash = this->getHashFromID(setid);
}

long double DeBruijnNodeHandle::getHash() const {
    return this->hash;
}

long double DeBruijnNodeHandle::getHashFromID(int ID){
    //See https://stackoverflow.com/questions/664014
    uint64_t x = ID+424242; //seeding the hash
    x = (x ^ (x >> 30)) * uint64_t(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * uint64_t(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    long double ldx = x;
    long double myhash = ldx / ((long double) exp2(64));
    return myhash;
}

bool DeBruijnNodeHandle::operator <(const DeBruijnNodeHandle& o) const{
    return this->getHash() < o.getHash();
}

bool DeBruijnNodeHandle::operator >(const DeBruijnNodeHandle& o) const{
    return this->getHash() > o.getHash();
}

bool DeBruijnNodeHandle::operator ==(const DeBruijnNodeHandle& o) const{
    if(this->getHash() == o.getHash()){
        if(this->handle.isUnspecified() || this->handle.isUnspecified()){
            return this->handle.isUnspecified() && this->handle.isUnspecified();
        }else{
            return this->handle == o.handle;
        }
    }
    return false;
}

bool DeBruijnNodeHandle::operator !=(const DeBruijnNodeHandle& o) const{
    return !operator==(o);
}

