CXX := g++

TARGET := bin/aplicacion

CXXFLAGS := -std=c++17 \
            -Wall \
            -Wextra \
            -O2 \
            -pthread \
            -Iinclude

LDFLAGS := -lbcm2835 -pthread

SRC := \
    src/main.cpp \
    src/vehicle.cpp \
    src/elm327.cpp \
    src/mcp2515/mcp2515.cpp

OBJ := \
    obj/main.o \
    obj/vehicle.o \
    obj/elm327.o \
    obj/mcp2515/mcp2515.o

.PHONY: all clean install dirs

all: dirs $(TARGET)

dirs:
	mkdir -p bin
	mkdir -p obj
	mkdir -p obj/mcp2515

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

obj/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/vehicle.o: src/vehicle.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/elm327.o: src/elm327.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/mcp2515/mcp2515.o: src/mcp2515/mcp2515.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj/*.o
	rm -rf obj/mcp2515/*.o
	rm -f $(TARGET)

install:
	@echo "Instalando dependencias..."
	sudo apt-get update
	sudo apt-get install -y \
		g++ \
		make \
		git \
		libbcm2835-dev

	@echo "Instalacion terminada."

run: $(TARGET)
	sudo $(TARGET)
