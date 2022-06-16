####################################################################################################
#### Pin Tool compilation

CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config

TOOL_CXXFLAGS += -std=c++11 -Wall
TOOL_LDFLAGS += -Wl,-rpath,$(PIN_ROOT)/intel64/runtime -pthread
TEST_TOOL_ROOTS := PinTool

ifdef DEBUG
TOOL_CXXFLAGS += -g -O0 -Werror -DDEBUG
endif

ifdef MAX_THREADS
TOOL_CXXFLAGS+=-DMAX_THREADS=$(MAX_THREADS)
endif


include $(TOOLS_ROOT)/Config/makefile.default.rules

