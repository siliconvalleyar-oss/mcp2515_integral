CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lpthread -lbcm2835

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/**/*.cpp $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

INCLUDES = -I$(INC_DIR)

ARCH ?= $(shell uname -m)
ifeq ($(ARCH),aarch64)
    CXXFLAGS += -march=native
    TARGET_SUFFIX := _64
else
    CXXFLAGS += -march=native
    TARGET_SUFFIX := _32
endif

TARGET = $(BIN_DIR)/autel_scanner$(TARGET_SUFFIX)

.PHONY: all clean build test run 32 64

all: build

build: $(TARGET)

32: ARCH = armv7l
32: build

64: ARCH = aarch64
64: build

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)
	@echo "Build completado: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

test: build
	@echo "Ejecutando pruebas..."
	@$(TARGET)

run: build
	sudo $(TARGET)

install: build
	install -d $(DESTDIR)/usr/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/bin/autel_scanner
	install -d $(DESTDIR)/etc/autel_scanner
	install -m 644 config/* $(DESTDIR)/etc/autel_scanner/
