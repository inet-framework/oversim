INETDIR = `pwd`/../../inet
REASEDIR = `pwd`/../../ReaSE

#BUILD_OPTIONS = -f --deep -linet -lgmp -o OverSim -I`pwd`/out/include -L`pwd`out/libs $(DEFS) -I$(INETDIR)/src/transport/tcp -I$(INETDIR)/src/applications/pingapp -I$(INETDIR)/src/networklayer/mpls -I$(INETDIR)/src/networklayer/ipv4 -I$(INETDIR)/src/linklayer/contract -I$(INETDIR)/src/linklayer/mfcore -I$(INETDIR)/src/networklayer/ldp -I$(INETDIR)/src/networklayer/ipv6 -I$(INETDIR)/src/networklayer/arp -I$(INETDIR)/src/networklayer/autorouting -I$(INETDIR)/src/applications/udpapp -I$(INETDIR)/src/networklayer/ted -I$(INETDIR)/src/networklayer/rsvp_te -I$(INETDIR)/src/util/headerserializers -I$(INETDIR)/src/networklayer/contract -I$(INETDIR)/src/transport/contract -I$(INETDIR)/src/networklayer/common -I$(INETDIR)/src/transport/sctp -I$(INETDIR)/src/transport/udp -I$(INETDIR)/src/base -I$(INETDIR)/src/networklayer/icmpv6 -I$(INETDIR)/src/world -I$(INETDIR)/src/util -L$(INETDIR)/src -L/usr/lib -I/usr/include/libxml2

BUILD_OPTIONS = -f --deep -I$(INETDIR)/src/networklayer/common -I$(INETDIR)/src/networklayer/rsvp_te -I$(INETDIR)/src/networklayer/icmpv6 -I$(INETDIR)/src/transport/tcp -I$(INETDIR)/src/networklayer/mpls -I$(INETDIR)/src/networklayer/ted -I$(INETDIR)/src/networklayer/contract -I$(INETDIR)/src/util -I$(INETDIR)/src/transport/contract -I$(INETDIR)/src/linklayer/mfcore -I$(INETDIR)/src/networklayer/ldp -I$(INETDIR)/src/applications/udpapp -I$(INETDIR)/src/networklayer/ipv4 -I$(INETDIR)/src/base -I$(INETDIR)/src/util/headerserializers -I$(INETDIR)/src/networklayer/ipv6 -I$(INETDIR)/src/transport/sctp -I$(INETDIR)/src/world -I$(INETDIR)/src/applications/pingapp -I$(INETDIR)/src/linklayer/contract -I$(INETDIR)/src/networklayer/arp -I$(INETDIR)/src/transport/udp -I$(INETDIR)/src/networklayer/autorouting -L$(INETDIR)/out/$$\(CONFIGNAME\)/src -linet -KINET_PROJ=$(INETDIR)

ifeq "$(REASE)" "true"
	BUILD_OPTIONS += -lrease -L$(REASEDIR)/src -KREASE_PROJ=$(REASEDIR)
endif

all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -rf src/Makefile
	rm -rf out

makefiles:
	cd src && opp_makemake $(BUILD_OPTIONS)

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
        fi
                                                                
doxy:
	doxygen doxy.cfg
	
verify:
	cd simulations && ../src/OverSim -fverify.ini -cChord | grep Fingerprint
	cd simulations && ../src/OverSim -fverify.ini -cKoorde | grep Fingerprint
	cd simulations && ../src/OverSim -fverify.ini -cKademlia | grep Fingerprint
	cd simulations && ../src/OverSim -fverify.ini -cBroose | grep Fingerprint
	cd simulations && ../src/OverSim -fverify.ini -cPastry | grep Fingerprint
	cd simulations && ../src/OverSim -fverify.ini -cBamboo | grep Fingerprint
	cd simulations && ../src/OverSim -fverify.ini -cKademliaInet | grep Fingerprint
	cd simulations && ../src/OverSim -fverify.ini -cChordSource | grep Fingerprint

dist: makefiles
	cd src && $(MAKE) MODE=release
	rm -rf dist
	mkdir dist
	mkdir dist/ned
	mkdir dist/ned/INET
	mkdir dist/ned/OverSim
	cp src/OverSim dist/
	cp simulations/default.ini dist/
	cp simulations/omnetpp.ini dist/
	cp simulations/nodes_2d_15000.xml dist/
	sed 's/^ned-path.*/ned-path = .\/ned\/inet;.\/ned\/OverSim/g' dist/default.ini > dist/defaultTemp.ini
	mv -f dist/defaultTemp.ini dist/default.ini
	(cd src && cd $(INETDIR)/src && tar -cf - `find . -name '*.ned'`) | tar -xC dist/ned/inet
	(cd src && tar -cf - `find . -name '*.ned'`) | tar -xC dist/ned/OverSim
