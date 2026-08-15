# ============================================================================
#  mcp2515_integral - Makefile raiz
#
#  Cada rama responde a su tipo de producto:
#    - rama `emulator` -> emulador ECU  (emulator/prisma)
#    - rama `scanner`  -> escaner OBD2  (scanner/autel_scanner)
#
#  Uso (desde la raiz del repo):
#    make emulator -j4    compila solo la emulacion
#    make emulator run    compila y ejecuta el emulador (sudo)
#    make scanner         compila solo el escaner
#    make scanner run     compila y ejecuta el escaner (sudo)
#    make                 compila el tipo de la rama actual
#    make run             compila y ejecuta el tipo de la rama actual
#    make clean           limpia el tipo de la rama actual
#    make help            muestra la ayuda
#
#  Nota: emulator/multi y scanner/reader tienen builds propios
#  (ver sus Makefile/scripts/build.sh).
# ============================================================================

# Deteccion del tipo de producto presente en la rama actual: en `emulator`
# solo existe emulator/, en `scanner` solo scanner/.
HAS_EMULATOR := $(wildcard emulator/prisma)
HAS_SCANNER  := $(wildcard scanner/autel_scanner)

ifeq ($(strip $(HAS_EMULATOR)),)
  ifeq ($(strip $(HAS_SCANNER)),)
    DEFAULT_TYPE :=
  else
    DEFAULT_TYPE := scanner
  endif
else
  DEFAULT_TYPE := emulator
endif

emulator_DIR := emulator/prisma
scanner_DIR  := scanner/autel_scanner

.PHONY: all emulator scanner emulator-run scanner-run run clean help

all: $(DEFAULT_TYPE)

emulator:
	@if [ -z "$(HAS_EMULATOR)" ]; then \
		echo "ERROR: 'make emulator' no esta disponible en esta rama."; \
		echo "       La emulacion vive en la rama 'emulator'."; \
		exit 1; \
	fi
	$(MAKE) -C $(emulator_DIR)

scanner:
	@if [ -z "$(HAS_SCANNER)" ]; then \
		echo "ERROR: 'make scanner' no esta disponible en esta rama."; \
		echo "       El escaner vive en la rama 'scanner'."; \
		exit 1; \
	fi
	$(MAKE) -C $(scanner_DIR)

# "make emulator run" resuelve a: objetivo emulator + objetivo run.
emulator-run: emulator
	$(MAKE) -C $(emulator_DIR) run

scanner-run: scanner
	$(MAKE) -C $(scanner_DIR) run

run:
	@if [ -z "$(DEFAULT_TYPE)" ]; then \
		echo "No hay producto que ejecutar en esta rama."; \
		exit 1; \
	fi
	$(MAKE) $(DEFAULT_TYPE)-run

clean:
	@if [ -z "$(DEFAULT_TYPE)" ]; then \
		echo "No hay producto que limpiar en esta rama (solo hub/legacy)."; \
	else \
		$(MAKE) -C $($(DEFAULT_TYPE)_DIR) clean; \
	fi

help:
	@echo "Uso: make <objetivo> [-jN]"
	@echo ""
	@echo "Tipo de esta rama: $(DEFAULT_TYPE)"
	@echo ""
	@echo "  make emulator     compila solo la emulacion (rama emulator)"
	@echo "  make emulator run compila y ejecuta el emulador (sudo)"
	@echo "  make scanner      compila solo el escaner (rama scanner)"
	@echo "  make scanner run  compila y ejecuta el escaner (sudo)"
	@echo "  make              compila el tipo de la rama actual"
	@echo "  make run          compila y ejecuta el tipo de la rama actual"
	@echo "  make clean        limpia el tipo de la rama actual"
	@echo "  make help         muestra esta ayuda"
