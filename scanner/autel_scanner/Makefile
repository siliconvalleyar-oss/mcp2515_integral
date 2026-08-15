CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
DEPFLAGS = -MMD -MP
SCANNER_VERSION := $(shell cat VERSION)
CXXFLAGS += -DSCANNER_VERSION=\"$(SCANNER_VERSION)\"
LDFLAGS = -lpthread -lbcm2835

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/**/*.cpp $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPFILES = $(OBJECTS:.o=.d)

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
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

# main.o embebe la version: recompilar si cambia VERSION
$(OBJ_DIR)/main.o: VERSION

# gcc-10 (Pi) hace ICE con -O2 + -march=native al compilar nlohmann/json
# (config.cpp). -O1 evita el crash del compilador (el -O ultimo gana).
$(OBJ_DIR)/scanner/config.o: CXXFLAGS += -O1

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

# Dependencias automaticas de headers (.d generados con -MMD -MP).
# Al final: si -include estuviera antes de 'all', el default goal pasaria
# a ser el primer objetivo de los .d (p.ej. obj/hardware/gpio.o).
-include $(DEPFILES)
