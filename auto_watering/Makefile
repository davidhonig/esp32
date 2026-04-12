# Makefile – auto_watering (ESP32-H2)
# Thin wrapper around idf.py for convenience.
#
# Usage:
#   make               → build
#   make flash         → flash (auto-detect port)
#   make monitor       → open serial monitor
#   make flash monitor → flash then monitor (two targets in one call)
#   make PORT=/dev/ttyUSB0 flash monitor
#   make idf-shell     → start an interactive shell with IDF env activated

SHELL := /bin/bash

# ── IDF activation script ──────────────────────────────────────────────────
IDF_ACTIVATE := /Users/david/.espressif/tools/activate_idf_v5.5.3.sh

# Every recipe line sources the activate script and forces the right target.
# IDF_TARGET is exported so idf.py never picks up a stale value from the env.
RUN := . '$(IDF_ACTIVATE)' && export IDF_TARGET=$(TARGET) &&

# ── Target chip ────────────────────────────────────────────────────────────
TARGET   ?= esp32h2

# ── Serial port (macOS: cu.usbserial-*, Linux: ttyUSB0 / ttyACM0) ─────────
#    AUTO-DETECT: first USB-serial port found; override with PORT=...
PORT     ?= $(firstword $(wildcard /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART \
                                   /dev/ttyUSB0 /dev/ttyACM0))
BAUD     ?= 115200

IDF_PY   := idf.py
ifneq ($(strip $(PORT)),)
  PORT_ARG := -p $(PORT)
endif

# ── Default target ─────────────────────────────────────────────────────────
.PHONY: all
all: build

# ── Open an IDF-activated shell ────────────────────────────────────────────
.PHONY: idf-shell
idf-shell:
	@echo "Starting IDF shell (IDF v5.5.3) – type 'exit' to leave."
	@bash --init-file <(echo ". '$(IDF_ACTIVATE)'")

# ── Build ──────────────────────────────────────────────────────────────────
# Auto-set the chip target when sdkconfig is absent (fresh clone / fullclean).
.PHONY: build
build:
	@if [ ! -f sdkconfig ]; then \
	    echo "No sdkconfig found – running set-target $(TARGET) first..."; \
	    . '$(IDF_ACTIVATE)' && export IDF_TARGET=$(TARGET) && $(IDF_PY) set-target $(TARGET); \
	fi
	$(RUN) $(IDF_PY) build

# ── Set chip target (run once after clone / clean) ─────────────────────────
.PHONY: set-target
set-target:
	$(RUN) $(IDF_PY) set-target $(TARGET)

# ── Interactive config ─────────────────────────────────────────────────────
.PHONY: menuconfig
menuconfig:
	$(RUN) $(IDF_PY) menuconfig

# ── Flash ──────────────────────────────────────────────────────────────────
.PHONY: flash
flash:
	$(RUN) $(IDF_PY) $(PORT_ARG) -b $(BAUD) flash

# ── Serial monitor ─────────────────────────────────────────────────────────
.PHONY: monitor
monitor:
	$(RUN) $(IDF_PY) $(PORT_ARG) -b $(BAUD) monitor

# ── Flash + monitor in one step ────────────────────────────────────────────
.PHONY: flash-monitor
flash-monitor:
	$(RUN) $(IDF_PY) $(PORT_ARG) -b $(BAUD) flash monitor

# ── Erase entire flash (use before first flash on a new board) ─────────────
.PHONY: erase
erase:
	$(RUN) $(IDF_PY) $(PORT_ARG) erase-flash

# ── Binary size report ─────────────────────────────────────────────────────
.PHONY: size
size:
	$(RUN) $(IDF_PY) size

.PHONY: size-components
size-components:
	$(RUN) $(IDF_PY) size-components

# ── Clean ──────────────────────────────────────────────────────────────────
.PHONY: clean
clean:
	$(RUN) $(IDF_PY) clean

.PHONY: fullclean
fullclean:
	$(RUN) $(IDF_PY) fullclean

# ── Help ───────────────────────────────────────────────────────────────────
.PHONY: help
help:
	@echo ""
	@echo "  auto_watering – ESP32-H2 build targets"
	@echo "  IDF: $(IDF_ACTIVATE)"
	@echo ""
	@echo "  make [all]           Build the project"
	@echo "  make idf-shell       Start an interactive shell with IDF env"
	@echo "  make set-target      Set chip to $(TARGET) (run once after clean)"
	@echo "  make menuconfig      Open interactive Kconfig menu"
	@echo "  make flash           Flash firmware  (PORT=$(PORT))"
	@echo "  make monitor         Open serial monitor"
	@echo "  make flash-monitor   Flash then open monitor"
	@echo "  make erase           Erase entire flash"
	@echo "  make size            Show binary size summary"
	@echo "  make size-components Show per-component sizes"
	@echo "  make clean           Remove build artefacts"
	@echo "  make fullclean       Remove build dir + sdkconfig"
	@echo ""
	@echo "  Override port:  make PORT=/dev/cu.usbserial-0001 flash"
	@echo "  Override baud:  make BAUD=921600 flash"
	@echo ""
