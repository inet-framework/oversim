//=========================================================================
//  CMEMCOMMBUFFER.H - part of
//
//                  OMNeT++/OMNEST
//           Discrete System Simulation in C++
//
//   Written by:  Andras Varga, 2003
//
//=========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 2003-2005 Andras Varga
  Monash University, Dept. of Electrical and Computer Systems Eng.
  Melbourne, Australia

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/
#ifndef __CNETCOMMBUFFER_H__
#define __CNETCOMMBUFFER_H__

#include "common/OverSimDefs.h"

#include "ccommbufferbase.h"


/**
 * Communication buffer that packs data into a memory buffer without any
 * transformation.
 *
 * @ingroup Parsim
 */
class cNetCommBuffer : public omnetpp::cCommBufferBase
{
  public:
    /**
     * Constructor.
     */
    cNetCommBuffer();

    /**
     * Destructor.
     */
    virtual ~cNetCommBuffer();

    /** @name Pack basic types */
    //@{
    virtual void pack(char d);
    virtual void pack(unsigned char d);
    virtual void pack(bool d);
    virtual void pack(short d);
    virtual void pack(unsigned short d);
    virtual void pack(int d);
    virtual void pack(unsigned int d);
    virtual void pack(long d);
    virtual void pack(unsigned long d);
    virtual void pack(long long d);
    virtual void pack(unsigned long long d);
    virtual void pack(float d);
    virtual void pack(double d);
    virtual void pack(long double d);
    virtual void pack(const char *d);
    virtual void pack(const omnetpp::opp_string& d);
    virtual void pack(omnetpp::SimTime d);
    //@}

    /** @name Pack arrays of basic types */
    //@{
    virtual void pack(const char *d, int size);
    virtual void pack(const unsigned char *d, int size);
    virtual void pack(const bool *d, int size);
    virtual void pack(const short *d, int size);
    virtual void pack(const unsigned short *d, int size);
    virtual void pack(const int *d, int size);
    virtual void pack(const unsigned int *d, int size);
    virtual void pack(const long *d, int size);
    virtual void pack(const unsigned long *d, int size);
    virtual void pack(const long long *d, int size);
    virtual void pack(const unsigned long long *d, int size);
    virtual void pack(const float *d, int size);
    virtual void pack(const double *d, int size);
    virtual void pack(const long double *d, int size);
    virtual void pack(const char **d, int size);
    virtual void pack(const omnetpp::opp_string *d, int size);
    virtual void pack(const omnetpp::SimTime *d, int size);
    //@}

    /** @name Unpack basic types */
    //@{
    virtual void unpack(char& d);
    virtual void unpack(unsigned char& d);
    virtual void unpack(bool& d);
    virtual void unpack(short& d);
    virtual void unpack(unsigned short& d);
    virtual void unpack(int& d);
    virtual void unpack(unsigned int& d);
    virtual void unpack(long& d);
    virtual void unpack(unsigned long& d);
    virtual void unpack(long long& d);
    virtual void unpack(unsigned long long& d);
    virtual void unpack(float& d);
    virtual void unpack(double& d);
    virtual void unpack(long double& d);
    virtual void unpack(const char *&d);
    /**
     * Unpacks a string -- storage will be allocated for it via new char[].
     */
    void unpack(char *&d)  {unpack((const char *&)d);}
    virtual void unpack(omnetpp::opp_string& d);
    virtual void unpack(omnetpp::SimTime&);

    //@}

    /** @name Unpack arrays of basic types */
    //@{
    virtual void unpack(char *d, int size);
    virtual void unpack(unsigned char *d, int size);
    virtual void unpack(bool *d, int size);
    virtual void unpack(short *d, int size);
    virtual void unpack(unsigned short *d, int size);
    virtual void unpack(int *d, int size);
    virtual void unpack(unsigned int *d, int size);
    virtual void unpack(long *d, int size);
    virtual void unpack(unsigned long *d, int size);
    virtual void unpack(long long *d, int size);
    virtual void unpack(unsigned long long *d, int size);
    virtual void unpack(float *d, int size);
    virtual void unpack(double *d, int size);
    virtual void unpack(long double *d, int size);
    virtual void unpack(const char **d, int size);
    virtual void unpack(omnetpp::opp_string *d, int size);
    virtual void unpack(omnetpp::SimTime *d, int size);
    //@}

    /**
     * return the length of the remaining buffer in bytes
     */
    size_t getRemainingMessageSize();

    /**
     * Packs an object.
     */
    virtual void packObject(omnetpp::cObject *obj);

    /**
     * Unpacks and returns an object.
     */
    virtual omnetpp::cObject *unpackObject();
};

#endif
