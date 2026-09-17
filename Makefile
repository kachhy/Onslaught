CXX ?= g++
CXXFLAGS=-Wall -MMD -MP -Wno-switch -Isrc -std=c++17
LDFLAGS=
EVALFILE=nn-0d2fd98ff872a9a1-v2.nnue

CXXFLAGS += -DEVALFILE='"$(EVALFILE)"'

OBJDIR=build
SRC=$(shell find src -name '*.cpp')
C_SRC=$(filter-out %tbchess.c,$(shell find src -name '*.c'))

ifeq ($(MAKECMDGOALS),)
	MAKECMDGOALS += release
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
	CXXFLAGS += -O3 -flto -march=native
ifneq ($(shell uname),Darwin)
	CXXFLAGS += -mpopcnt
	LDFLAGS += -static
endif
	LDFLAGS += -flto
	OBJDIR:=$(OBJDIR)/native
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
ifneq ($(filter spsa,$(MAKECMDGOALS)),)
	CXXFLAGS += -DSPSA_TUNE -O3 -flto
	LDFLAGS += -flto
ifneq ($(shell uname),Darwin)
	LDFLAGS += -static
endif
	OBJDIR:=$(OBJDIR)/spsa
endif
ifneq ($(filter perft,$(MAKECMDGOALS)),)
	CXXFLAGS += -O3 -flto -DPERFT
	LDFLAGS += -flto
	OBJDIR:=$(OBJDIR)/perft
endif

OBJ=$(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))
C_OBJ=$(patsubst src/%.c,$(OBJDIR)/%.o,$(C_SRC))
DEPS=$(OBJ:.o=.d) $(C_OBJ:.o=.d)
EXE ?= $(OBJDIR)/Onslaught

release: $(EXE)
debug: $(EXE)
lto: $(EXE)
native: $(EXE)
pg: $(EXE)
tune: $(EXE)
spsa: $(EXE)
perft: $(EXE)

$(EXE): $(OBJ) $(C_OBJ)
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

$(OBJDIR)/%.o: src/%.c
	@echo "  CXXC $@"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -x c++ -c $< -o $@

clean:
	rm -rf $(OBJDIR)

.PHONY: all clean debug release
-include $(DEPS)
