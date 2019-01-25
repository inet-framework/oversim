#!/usr/bin/python

from xmlrpclib import *

def printResult(res):
    for i in res:
        print "Kind: %i, ID: %i, Value: %s" % (i[1], i[2], i[0].data)

s = ServerProxy("http://localhost:3631/");
s.lookup(Binary("123"), 1)
s.put(Binary("4"), Binary("4"), 600, "bla")
s.get(Binary("4"), 1, Binary(""), "bla")[0][0].data
s.register(Binary("alice@p2pname.org"), 3, 1, Binary("3.1.1.1"), 600)
s.register(Binary("alice@p2pname.org"), 2, 2, Binary("2.1.1.2"), 600)
s.register(Binary("alice@p2pname.org"), 2, 1, Binary("2.1.1.1"), 600)
s.register(Binary("alice@p2pname.org"), 1, 3, Binary("1.1.1.3"), 600)
print "kind = 0:"
printResult(s.resolve(Binary("alice@p2pname.org"), 0))
print "kind = 1:"
printResult(s.resolve(Binary("alice@p2pname.org"), 1))
print "kind = 2:"
printResult(s.resolve(Binary("alice@p2pname.org"), 2))
#print s.dump_dht(1)
