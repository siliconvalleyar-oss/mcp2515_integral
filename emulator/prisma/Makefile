# ============================================================================
#  Emulador OBD2 Chevrolet Prisma (ELM327 + MCP2515 + libbcm2835)
#  Raspberry Pi (Raspberry Pi OS / Raspbian)
#
#  Uso:
#    make                  compila el emulador
#    make install-bcm2835  instala libbcm2835 (requiere sudo)
#    make run              ejecuta el emulador con sudo
#    make kill             mata las instancias activas (scripts/kill_apps.sh)
#    make clean            elimina objetos y binario
# ============================================================================

APP_VERSION := $(shell cat VERSION 2>/dev/null || echo 1.0.0)

CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++17
CXXFLAGS += -Iinclude -Itest -DAPP_VERSION=\"$(APP_VERSION)\"

# Frecuencia del cristal del módulo MCP2515 (8 o 16 MHz). El driver calcula
# con ella los registros CNF1/2/3 del bit timing CAN. Sobreescribible en la
# línea de comandos, p. ej.:  make MCP2515_OSC_HZ=16000000
# Los módulos CAN-BUS de este repo usan cristal de 8 MHz.
MCP2515_OSC_HZ ?= 8000000
CXXFLAGS += -DMCP2515_OSC_HZ=$(MCP2515_OSC_HZ)UL

LDLIBS    = -lbcm2835 -lpthread

# Binario: bin/emulator_prisma_<sufijo>. El sufijo se detecta por uname -m
# (auto): aarch64 -> _64, resto (armv6l/armv7l) -> _32. Sin targets manuales.
BIN_DIR := bin
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),aarch64)
TARGET_SUFFIX := _64
else
TARGET_SUFFIX := _32
endif
TARGET = $(BIN_DIR)/emulator_prisma$(TARGET_SUFFIX)

SRCS   = src/main.cpp src/mcp2515.cpp src/vehicle.cpp src/elm327.cpp src/monitor_log.cpp src/vehicle_config.cpp
OBJS   = $(SRCS:src/%.cpp=obj/%.o) obj/autotest.o
DEPS   = $(OBJS:.o=.d)

# Pruebas de comunicación MCP2515 <-> Raspberry Pi
TEST_BINS = obj/test_spi obj/test_loopback obj/test_bus

.PHONY: all clean run kill install-bcm2835 check-bcm2835 test test-build test-socketcan help

all: check-bcm2835 $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

obj/%.o: src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

obj/autotest.o: test/autotest.cpp | obj
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

obj:
	mkdir -p obj

-include $(DEPS)

# ----------------------------------------------------------------------------
#  Dependencia: librería bcm2835
# ----------------------------------------------------------------------------
check-bcm2835:
	@if [ ! -f /usr/local/include/bcm2835.h ] || \
	   [ ! -f /usr/local/lib/libbcm2835.a ]; then \
		echo "ERROR: libbcm2835 no está instalada."; \
		echo "Ejecute:  make install-bcm2835"; \
		exit 1; \
	fi

install-bcm2835:
	@echo ">> Descargando e instalando libbcm2835 (necesita sudo)..."
	@if [ ! -f bcm2835-1.68.tar.gz ]; then \
		wget -q http://www.airspayce.com/mikem/bcm2835/bcm2835-1.68.tar.gz; \
	fi
	@tar xzf bcm2835-1.68.tar.gz
	@cd bcm2835-1.68 && ./configure --disable-dependency-tracking \
		&& make && sudo make install
	@rm -rf bcm2835-1.68 bcm2835-1.68.tar.gz
	@echo ">> libbcm2835 instalada en /usr/local."

# ----------------------------------------------------------------------------
#  Ejecución (requiere root para acceder a /dev/mem)
# ----------------------------------------------------------------------------
run: all
	sudo ./$(TARGET)

# ----------------------------------------------------------------------------
#  Matar instancias activas del emulador (y opcionalmente las pruebas)
# ----------------------------------------------------------------------------
kill:
	sudo ./scripts/kill_apps.sh

# ----------------------------------------------------------------------------
#  Pruebas de comunicación (MCP2515 <-> Raspberry Pi)
# ----------------------------------------------------------------------------
test-build: check-bcm2835 $(TEST_BINS)

test: test-build
	sudo ./scripts/run_tests.sh

test-socketcan:
	sudo ./scripts/can_kernel_test.sh

obj/test_spi: test/test_spi.cpp test/autotest.cpp src/mcp2515.cpp | obj
	$(CXX) $(CXXFLAGS) -o $@ test/test_spi.cpp test/autotest.cpp \
		src/mcp2515.cpp $(LDLIBS)

obj/test_loopback: test/test_loopback.cpp test/autotest.cpp src/mcp2515.cpp | obj
	$(CXX) $(CXXFLAGS) -o $@ test/test_loopback.cpp test/autotest.cpp \
		src/mcp2515.cpp $(LDLIBS)

obj/test_bus: test/test_bus.cpp test/autotest.cpp src/mcp2515.cpp | obj
	$(CXX) $(CXXFLAGS) -o $@ test/test_bus.cpp test/autotest.cpp \
		src/mcp2515.cpp $(LDLIBS)

clean:
	rm -rf obj $(BIN_DIR)

help:
	@echo "Objetivos del Makefile:"
	@echo "  make                    compila el emulador"
	@echo "  make install-bcm2835    instala libbcm2835 (sudo)"
	@echo "  make run                ejecuta con sudo"
	@echo "  make kill               mata las instancias activas (sudo)"
	@echo "  make test-build         compila las pruebas de comunicación"
	@echo "  make test               compila y ejecuta las pruebas (sudo)"
	@echo "  make test-socketcan     prueba alternativa con SocketCAN (sudo)"
	@echo "  make clean              elimina objetos y binario"
