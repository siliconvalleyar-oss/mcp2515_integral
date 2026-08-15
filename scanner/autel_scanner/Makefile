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

TARGET = $(BIN_DIR)/scanner_autel$(TARGET_SUFFIX)

.PHONY: all clean build test run install

all: build

build: $(TARGET)

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
	install -m 755 $(TARGET) $(DESTDIR)/usr/bin/scanner_autel
	install -d $(DESTDIR)/etc/scanner_autel
	install -m 644 config/* $(DESTDIR)/etc/scanner_autel/
