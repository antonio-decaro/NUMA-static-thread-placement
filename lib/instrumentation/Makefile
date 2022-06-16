#################################################################################
#            Shared Memory Communications Recorder Compilation chain            #
#             - Compile pintool to record memory accesses                       #
#             - Compile build_affinity to build affinity matrix out of trace    #
#################################################################################

all: pintool

clean:
	rm -f $(PART) $(OBJ)
	make -C src clean

include Makefile.inc

ifdef MAX_THREADS
CXXFLAGS+=-DMAX_THREADS=$(MAX_THREADS)
endif

#enable debug ?
ifdef DEBUG
CXXFLAGS+=-g -O0 -DDEBUG
else
CXXFLAGS+=-O2
endif

ifndef PIN_ROOT
pintool:
	$(warning PIN_ROOT is not set. Use make PIN_ROOT=... to build pin tool)

clean_pintool:
	$(warning PIN_ROOT is not set. Use make PIN_ROOT=... to build pin tool)
else

ifdef DEBUG
pintool:
	make -C src DEBUG=$(DEBUG)
else
pintool:
	make -C src
endif

endif
