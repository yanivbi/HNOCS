#
# OMNeT++/OMNEST Makefile for hnocs.01_02_13
#
# This file was generated with the command:
#  opp_makemake -f --deep
#

# Name of target to be created (-o option)
TARGET = hnocs.01_02_13$(EXE_SUFFIX)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = \
    -I. \
    -Icores \
    -Icores/sinks \
    -Icores/sources \
    -Irouters \
    -Irouters/hier \
    -Irouters/hier/inPort \
    -Irouters/hier/opCalc \
    -Irouters/hier/opCalc/static \
    -Irouters/hier/sched \
    -Irouters/hier/sched/wormhole \
    -Irouters/hier/vcCalc \
    -Irouters/hier/vcCalc/free \
    -Itopologies

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS =

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = /src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc and .msg files
OBJS = \
    $O/cores/sinks/InfiniteBWMultiVCSink.o \
    $O/cores/sinks/InfiniteBWMultiVCSinkperSrc.o \
    $O/cores/sources/PktFifoSrc.o \
    $O/routers/hier/inPort/InPortAsync.o \
    $O/routers/hier/inPort/InPortSync.o \
    $O/routers/hier/opCalc/static/XYOPCalc.o \
    $O/routers/hier/sched/wormhole/SchedAsync.o \
    $O/routers/hier/sched/wormhole/SchedSync.o \
    $O/routers/hier/vcCalc/free/FLUVCCalc.o \
    $O/NoCs_m.o

# Message files
MSGFILES = \
    NoCs.msg

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" -loppmain$D $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ($(MAKECMDGOALS),depend)
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $O/$(TARGET)
	$(LN) $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@$(MKPATH) $O
	$(CXX) $(LDFLAGS) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY: all clean cleanall depend msgheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE)
	@$(MKPATH) $(dir $@)
	$(CXX) -c $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(MSGC) -s _m.cc $(MSGCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

clean:
	-rm -rf $O
	-rm -f hnocs.01_02_13 hnocs.01_02_13.exe libhnocs.01_02_13.so libhnocs.01_02_13.a libhnocs.01_02_13.dll libhnocs.01_02_13.dylib
	-rm -f ./*_m.cc ./*_m.h
	-rm -f cores/*_m.cc cores/*_m.h
	-rm -f cores/sinks/*_m.cc cores/sinks/*_m.h
	-rm -f cores/sources/*_m.cc cores/sources/*_m.h
	-rm -f routers/*_m.cc routers/*_m.h
	-rm -f routers/hier/*_m.cc routers/hier/*_m.h
	-rm -f routers/hier/inPort/*_m.cc routers/hier/inPort/*_m.h
	-rm -f routers/hier/opCalc/*_m.cc routers/hier/opCalc/*_m.h
	-rm -f routers/hier/opCalc/static/*_m.cc routers/hier/opCalc/static/*_m.h
	-rm -f routers/hier/sched/*_m.cc routers/hier/sched/*_m.h
	-rm -f routers/hier/sched/wormhole/*_m.cc routers/hier/sched/wormhole/*_m.h
	-rm -f routers/hier/vcCalc/*_m.cc routers/hier/vcCalc/*_m.h
	-rm -f routers/hier/vcCalc/free/*_m.cc routers/hier/vcCalc/free/*_m.h
	-rm -f topologies/*_m.cc topologies/*_m.h

cleanall: clean
	-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES)  ./*.cc cores/*.cc cores/sinks/*.cc cores/sources/*.cc routers/*.cc routers/hier/*.cc routers/hier/inPort/*.cc routers/hier/opCalc/*.cc routers/hier/opCalc/static/*.cc routers/hier/sched/*.cc routers/hier/sched/wormhole/*.cc routers/hier/vcCalc/*.cc routers/hier/vcCalc/free/*.cc topologies/*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/cores/sinks/InfiniteBWMultiVCSink.o: cores/sinks/InfiniteBWMultiVCSink.cc \
  ./NoCs_m.h \
  cores/sinks/InfiniteBWMultiVCSink.h
$O/cores/sinks/InfiniteBWMultiVCSinkperSrc.o: cores/sinks/InfiniteBWMultiVCSinkperSrc.cc \
  cores/sinks/InfiniteBWMultiVCSinkperSrc.h \
  ./NoCs_m.h
$O/cores/sources/PktFifoSrc.o: cores/sources/PktFifoSrc.cc \
  ./NoCs_m.h \
  cores/sources/PktFifoSrc.h
$O/routers/hier/inPort/InPortAsync.o: routers/hier/inPort/InPortAsync.cc \
  routers/hier/inPort/InPortAsync.h \
  ./NoCs_m.h \
  routers/hier/FlitMsgCtrl.h
$O/routers/hier/inPort/InPortSync.o: routers/hier/inPort/InPortSync.cc \
  ./NoCs_m.h \
  routers/hier/inPort/InPortSync.h \
  routers/hier/FlitMsgCtrl.h
$O/routers/hier/opCalc/static/XYOPCalc.o: routers/hier/opCalc/static/XYOPCalc.cc \
  ./NoCs_m.h \
  routers/hier/opCalc/static/XYOPCalc.h \
  routers/hier/FlitMsgCtrl.h
$O/routers/hier/sched/wormhole/SchedAsync.o: routers/hier/sched/wormhole/SchedAsync.cc \
  ./NoCs_m.h \
  routers/hier/HierRouter.h \
  routers/hier/sched/wormhole/SchedAsync.h
$O/routers/hier/sched/wormhole/SchedSync.o: routers/hier/sched/wormhole/SchedSync.cc \
  routers/hier/sched/wormhole/SchedSync.h \
  ./NoCs_m.h \
  routers/hier/HierRouter.h
$O/routers/hier/vcCalc/free/FLUVCCalc.o: routers/hier/vcCalc/free/FLUVCCalc.cc \
  routers/hier/vcCalc/free/FLUVCCalc.h \
  ./NoCs_m.h \
  routers/hier/HierRouter.h \
  routers/hier/FlitMsgCtrl.h
