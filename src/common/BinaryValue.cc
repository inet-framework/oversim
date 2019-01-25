//
// Copyright (C) 2007 Institut fuer Telematik, Universitaet Karlsruhe (TH)
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

/**
 * @file BinaryValue.cc
 * @author Ingmar Baumgart
 */

#include <iterator>

#include <cnetcommbuffer.h>
#include "BinaryValue.h"

using namespace std;

// predefined BinaryValue
const BinaryValue BinaryValue::UNSPECIFIED_VALUE;

BinaryValue::BinaryValue()
{
};

BinaryValue::BinaryValue(size_t n): vector<char>(n)
{
};

BinaryValue::BinaryValue(const std::string& str) : vector<char>(str.begin(), str.end())
{
};

BinaryValue::BinaryValue(const std::vector<char>& v) : vector<char>(v)
{
};

BinaryValue::BinaryValue(const char* cStr)
{
    *this = BinaryValue(cStr, strlen(cStr));
};

BinaryValue::BinaryValue(const char* b, const size_t l) : vector<char>(b, b+l)
{
};

BinaryValue::BinaryValue(cObject* obj)
{
    packObject(obj);
};

BinaryValue& BinaryValue::operator+=(const BinaryValue& rhs)
{
    insert(end(), rhs.begin(), rhs.end());
    return *this;
}

bool BinaryValue::isUnspecified() const
{
    return empty();
}

// Allow output of vector<char> using normal notation
std::ostream& operator << (std::ostream& os, const BinaryValue& v) {
    copy(v.begin(), v.end(), ostream_iterator<char>(os, ""));
    return os;        // To allow (cout << a) << b;
}

bool BinaryValue::operator<(const BinaryValue& rhs)
{
    size_type minSize = min(this->size(), rhs.size());
    for (size_type i=0; i<minSize; i++) {
        if ((*this)[i] < rhs[i]) {
            return true;
        } else if ((*this)[i] > rhs[i]) {
            return false;
        }
    }

    return (this->size() < rhs.size()) ? true : false;
}

void BinaryValue::netPack(cCommBuffer *b)
{
    doPacking(b,(uint16_t)size());
    doPacking(b, data(), size());
}

void BinaryValue::netUnpack(cCommBuffer *b)
{
    uint16_t size;
    doUnpacking(b, size);
    resize(size);
    doUnpacking(b, data(), size);
}

void BinaryValue::packObject(cObject* obj)
{
    cNetCommBuffer* b = new cNetCommBuffer();
    b->reset();
    b->packObject(obj);
    resize(b->getMessageSize());
    memcpy(data(), b->getBuffer(), b->getMessageSize());
    delete b;
}

cObject* BinaryValue::unpackObject()
{
    cNetCommBuffer* b = new cNetCommBuffer();
    cObject* obj;

    b->reset();
    b->allocateAtLeast(size());
    memcpy(b->getBuffer(), data(), size());
    b->setMessageSize(size());

    obj = b->unpackObject();

    delete b;

    return obj;
}
