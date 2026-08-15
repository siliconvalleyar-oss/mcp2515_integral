# ECU Multi-Emulator Makefile
# Cross-compiler: arm-linux-gnueabihf-g++ (ARM 32-bit, hard-float)

CROSS_COMPILE ?= arm-linux-gnueabihf-
CXX          := $(CROSS_COMPILE)g++
CXXFLAGS     := -std=c++17 -O2 -g -Wall -Wextra -Wno-psabi
CXXFLAGS     += -Isrc -Isrc/core -Isrc/manufacturers -Isrc/protocols
CXXFLAGS     += -Isrc/database -Isrc/diagnostics -Isrc/simulation
CXXFLAGS     += -Isrc/api -Isrc/security -Isrc/logging
LDFLAGS      := -lpthread -lsqlite3 -lrt
RM           := rm -rf
MKDIR        := mkdir -p
BUILD_DIR    := build
TARGET       := ecu_emulator

# nlohmann/json.hpp: download single-header if not reachable
JSON_TARGET  := build/include/nlohmann/json.hpp
ifeq ($(wildcard $(JSON_TARGET)),)
  $(shell mkdir -p build/include/nlohmann)
  $(shell curl -sL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -o $(JSON_TARGET) 2>/dev/null || \
            wget -q https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -O $(JSON_TARGET) 2>/dev/null)
endif
ifneq ($(wildcard $(JSON_TARGET)),)
  CXXFLAGS  += -Ibuild/include
  $(info Using nlohmann/json.hpp from $(abspath $(JSON_TARGET)))
else
  # Fallback: try host system path (header-only, compatible with cross-compiler)
  CXXFLAGS  += -I/usr/include
  $(info nlohmann/json.hpp not downloaded, using host /usr/include)
endif

# Source files by module (for readability, also used for test build)
CORE_SRCS      := $(wildcard src/core/*.cpp)
MANUF_SRCS     := $(wildcard src/manufacturers/*.cpp)
PROTO_SRCS     := $(wildcard src/protocols/*.cpp)
DB_SRCS        := $(wildcard src/database/*.cpp)
DIAG_SRCS      := $(wildcard src/diagnostics/*.cpp)
SIM_SRCS       := $(wildcard src/simulation/*.cpp)
API_SRCS       := $(wildcard src/api/*.cpp)
SEC_SRCS       := $(wildcard src/security/*.cpp)
LOG_SRCS       := $(wildcard src/logging/*.cpp)
MAIN_SRC       := src/main.cpp

# All source files (excluding tests)
ALL_SRCS       := $(MAIN_SRC) $(CORE_SRCS) $(MANUF_SRCS) $(PROTO_SRCS) \
                  $(DB_SRCS) $(DIAG_SRCS) $(SIM_SRCS) $(API_SRCS) \
                  $(SEC_SRCS) $(LOG_SRCS)

# Object files mirror source tree under build/
ALL_OBJS       := $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(ALL_SRCS))

# Test sources
TEST_SRCS      := $(wildcard src/tests/*.cpp)
TEST_OBJS      := $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(TEST_SRCS))
TEST_TARGET    := ecu_tests

# Default target
.PHONY: all
all: $(BUILD_DIR)/$(TARGET)

# Link
$(BUILD_DIR)/$(TARGET): $(ALL_OBJS)
	@echo "  LINK    $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "  Build complete: $(BUILD_DIR)/$(TARGET)"
	@$(CROSS_COMPILE)size $@

# Compilation: one rule per .o catches all src/ subdirs
$(BUILD_DIR)/%.o: src/%.cpp
	@$(MKDIR) $(dir $@)
	@echo "  CXX     $<"
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ============================================================
# Tests (requires Catch2 compiled for ARM)
# ============================================================
.PHONY: test
test: CXXFLAGS += -DCATCH_CONFIG_MAIN
test: $(BUILD_DIR)/$(TEST_TARGET)
	@echo "  Run with: ./$(BUILD_DIR)/$(TEST_TARGET) [--list-tests]"

$(BUILD_DIR)/$(TEST_TARGET): $(filter-out $(BUILD_DIR)/main.o, $(ALL_OBJS)) $(TEST_OBJS)
	@echo "  LINK    $@ (tests)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# ============================================================
# Utilities
# ============================================================
.PHONY: clean
clean:
	$(RM) $(BUILD_DIR)

.PHONY: distclean
distclean: clean
	$(RM) $(TARGET) $(TEST_TARGET)

.PHONY: rebuild
rebuild: clean all

.PHONY: info
info:
	@echo "=== Build Info ==="
	@echo "CXX:       $(CXX)"
	@echo "CXXFLAGS:  $(CXXFLAGS)"
	@echo "LDFLAGS:   $(LDFLAGS)"
	@echo "Sources:   $(words $(ALL_SRCS)) files"
	@echo "Objects:   $(words $(ALL_OBJS)) files"
	@echo "Target:    $(BUILD_DIR)/$(TARGET)"
	@echo ""
	@$(CROSS_COMPILE)g++ --version 2>/dev/null || echo "  Cross-compiler not found in PATH"

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all        - Build $(TARGET) (default)"
	@echo "  test       - Build test binary $(TEST_TARGET)"
	@echo "  clean      - Remove build directory"
	@echo "  distclean  - Remove build + binaries"
	@echo "  rebuild    - Clean + build"
	@echo "  info       - Show build configuration"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE=   Prefix for cross-compiler (default: arm-linux-gnueabihf-)"
	@echo ""
	@echo "Examples:"
	@echo "  make                           # build with arm-linux-gnueabihf-g++"
	@echo "  make CROSS_COMPILE=arm-none-eabi-  # use different prefix"
	@echo "  make CROSS_COMPILE=           # build with native g++"
	@echo "  make clean && make -j4        # parallel rebuild"

.DEFAULT_GOAL := all
