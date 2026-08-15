# =========================================================
# Makefile - Emulador Chevrolet Prisma (OBD2 / ELM327)
# Requiere la libreria bcm2835 instalada.
# Instalacion automatica: make install_deps  (ver scripts/install_dependencies.sh)
# =========================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude -pthread
LDFLAGS  := -lbcm2835 -lpthread

BIN_DIR  := bin
OBJ_DIR  := obj
SRC_DIR  := src
TARGET   := $(BIN_DIR)/aplicacion

# Fuentes que realmente se compilan.
# NOTA: src/mcp2515.cpp es solo un placeholder de compatibilidad de estructura
# y no forma parte de la compilacion; el driver real vive en src/mcp2515/mcp2515.cpp
SOURCES := \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/vehicle.cpp \
    $(SRC_DIR)/elm327.cpp \
    $(SRC_DIR)/mcp2515/mcp2515.cpp

OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

.PHONY: all clean distclean install_deps run dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)/mcp2515 $(OBJ_DIR)/oled

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo ""
	@echo "Compilacion completa: $(TARGET)"
	@echo "Ejecute con privilegios de root: sudo ./$(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Descarga, compila e instala la libreria bcm2835 en el sistema
install_deps:
	bash scripts/install_dependencies.sh

run: all
	sudo ./$(TARGET)

clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/mcp2515/*.o $(OBJ_DIR)/oled/*.o $(TARGET)

distclean: clean
	rm -rf $(BIN_DIR) $(OBJ_DIR)
