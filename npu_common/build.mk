.PHONY: all clean
.PRECIOUS: %.cpp.o %.cpp.d libsystemc.a

OBJS := $(SRCS:%=%.o)
DEPS := $(SRCS:%=%.d)
EXE  := $(MODULE).x

CXX      := g++
LD       := g++
CXXFLAGS := -std=c++98 -g -DSC_INCLUDE_DYNAMIC_PROCESSES -Wno-deprecated -Wall -I. -I$(SYSTEMC)/include -I../npu_common
LDFLAGS  := -lm -lpthread $(EXTRA_LIBS)

TARGET_ARCH := linux64

all: $(EXE)

clean:
	@echo "(CLN) Clean"
	@rm -rf $(OBJS) $(DEPS) libsystemc.a

%.cpp.o: %.cpp %.cpp.d
	@echo "(CXX) $@"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%.cpp.d: %.cpp
	@echo "(DEP) $@"
	@$(CXX) $(CXXFLAGS) -M -MT $<.o -MF $@ $<

libsystemc.a: $(SYSTEMC)/lib-$(TARGET_ARCH)/libsystemc.a
	@echo "(CPY) $@"
	@objcopy -g $< $@

$(EXE): $(OBJS) libsystemc.a
	@echo "(LNK) $@"
	@$(LD) -o $@ $^ $(LDFLAGS)

-include $(DEPS)

