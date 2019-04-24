INETDIR = `pwd`/../../inet
REASEDIR = `pwd`/../../ReaSE

BUILD_OPTIONS = -f --deep -o OverSim -I. -I$(INETDIR)/src -L$(INETDIR)/src -l'INET$$D' -KINET_PROJ=$(INETDIR)

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
