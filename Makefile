CXX ?= g++
CXXFLAGS=-Wall -MMD -MP -Wno-switch -Isrc -std=c++17
LDFLAGS=
EVALFILE=nn-a99aafd2a45ad047-v3-preview.nnue

CXXFLAGS += -DEVALFILE='"$(EVALFILE)"'

OBJDIR=build
SRC=$(shell find src -name '*.cpp')
C_SRC=$(filter-out %tbchess.c,$(shell find src -name '*.c'))

# ---------------------------------------------------------------------------
# Release builds
#   make releases VERSION=1.0        -> all four variants into $(DISTDIR)/
#   make release-avx2 VERSION=1.0    -> just one variant
# Output: Onslaught-<version>-<windows|ubuntu>-x86-64[-popcnt|-avx2|-avx512]
# ---------------------------------------------------------------------------
VERSION ?= dev
DISTDIR ?= dist
ARCHS   := none popcnt avx2 avx512

ifeq ($(OS),Windows_NT)
    PLATFORM ?= windows
    EXE_EXT  := .exe
else
    PLATFORM ?= ubuntu
    EXE_EXT  :=
endif

ARCH_FLAGS_none   := -march=x86-64
ARCH_FLAGS_popcnt := -march=x86-64 -mpopcnt
ARCH_FLAGS_avx2   := -march=x86-64 -mpopcnt -mavx2 -mbmi
ARCH_FLAGS_avx512 := $(ARCH_FLAGS_avx2) -mavx512f -mavx512bw

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
# Internal goal used by the release-<arch> targets (one sub-make per variant)
ifneq ($(filter variant,$(MAKECMDGOALS)),)
ifeq ($(filter $(ARCH),$(ARCHS)),)
    $(error ARCH must be one of: $(ARCHS))
endif
    CXXFLAGS += -O3 -flto $(ARCH_FLAGS_$(ARCH))
    LDFLAGS  += -O3 -flto $(ARCH_FLAGS_$(ARCH))
ifneq ($(shell uname),Darwin)
    LDFLAGS  += -static
endif
    OBJDIR:=$(OBJDIR)/release-$(ARCH)
    ARCH_SUFFIX := $(if $(filter none,$(ARCH)),,-$(ARCH))
    EXE := $(DISTDIR)/Onslaught-$(VERSION)-$(PLATFORM)-x86-64$(ARCH_SUFFIX)$(EXE_EXT)
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
variant: $(EXE)

RELEASE_TARGETS := $(addprefix release-,$(ARCHS))

releases: $(RELEASE_TARGETS)

$(RELEASE_TARGETS): release-%:
	@$(MAKE) --no-print-directory variant ARCH=$* VERSION=$(VERSION) PLATFORM=$(PLATFORM)

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

.PHONY: all clean debug release releases variant $(RELEASE_TARGETS)
-include $(DEPS)