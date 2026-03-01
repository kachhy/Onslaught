CXXC=g++
CXXFLAGS=-Wall -MMD -MP -Wno-switch
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
	OBJDIR:=$(OBJDIR)/lto
endif

OBJ=$(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))
DEPS=$(OBJ:.o=.d)
BIN=$(OBJDIR)/Axiom

all: debug

debug: $(BIN)
release: $(BIN)
lto: $(BIN)

$(BIN): $(OBJ)
	@echo "  LINK $@"
	@mkdir -p $(dir $@)
	@$(CXXC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: src/%.cpp src/%.h
	@echo "  CXXC $@"
	@mkdir -p $(dir $@)
	@$(CXXC) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: src/%.cpp
	@echo "  CXXC $@"
	@mkdir -p $(dir $@)
	@$(CXXC) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(BIN) $(OBJDIR)

.PHONY: all clean debug release
-include $(DEPS)
