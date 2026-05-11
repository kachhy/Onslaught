CXX ?= g++
CXXFLAGS=-Wall -MMD -MP -Wno-switch -Isrc -std=c++17 -DSPSA_TUNE
LDFLAGS=
OBJDIR=build
SRC=$(shell find src -name '*.cpp')

ifeq ($(MAKECMDGOALS),)
	MAKECMDGOALS += debug
endif

ifneq ($(filter debug,$(MAKECMDGOALS)),)
	CXXFLAGS += -DDEBUG -g
	OBJDIR:=$(OBJDIR)/debug
endif
ifneq ($(filter release,$(MAKECMDGOALS)),)
	CXXFLAGS += -O3
	OBJDIR:=$(OBJDIR)/release
endif
ifneq ($(filter lto,$(MAKECMDGOALS)),)
	CXXFLAGS += -O3 -flto
	LDFLAGS += -flto 
ifneq ($(shell uname),Darwin)
	LDFLAGS += -static
endif
	OBJDIR:=$(OBJDIR)/lto
endif
ifneq ($(filter native,$(MAKECMDGOALS)),)
	CXXFLAGS += -O3 -flto -march=native -DSPSA_TUNE
ifneq ($(shell uname),Darwin)
	CXXFLAGS += -mpopcnt
	LDFLAGS += -static
endif
	LDFLAGS += -flto
	OBJDIR:=$(OBJDIR)/native
endif
ifneq ($(filter spsa,$(MAKECMDGOALS)),)
	CXXFLAGS += -O3 -flto -march=native -DSPSA_TUNE
ifneq ($(shell uname),Darwin)
	CXXFLAGS += -mpopcnt
	LDFLAGS += -static
endif
	LDFLAGS += -flto
	OBJDIR:=$(OBJDIR)/spsa
endif
ifneq ($(filter pg,$(MAKECMDGOALS)),)
	CXXFLAGS += -pg
	LDFLAGS += -pg
	OBJDIR:=$(OBJDIR)/pg
endif
ifneq ($(filter tune,$(MAKECMDGOALS)),)
	CXXFLAGS += -DTUNING -O3 -flto -march=native
	OBJDIR:=$(OBJDIR)/tune
endif
ifneq ($(filter perft,$(MAKECMDGOALS)),)
	CXXFLAGS += -O3 -flto -DPERFT
	LDFLAGS += -flto
	OBJDIR:=$(OBJDIR)/perft
endif

OBJ=$(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))
DEPS=$(OBJ:.o=.d)
EXE ?= $(OBJDIR)/Axiom

debug: $(EXE)
release: $(EXE)
lto: $(EXE)
native: $(EXE)
spsa: $(EXE)
pg: $(EXE)
tune: $(EXE)
perft: $(EXE)

$(EXE): $(OBJ)
	@echo "  LINK $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: src/%.cpp src/%.h
	@echo "  CXXC $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: src/%.cpp
	@echo "  CXXC $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

.PHONY: all clean debug release
-include $(DEPS)
