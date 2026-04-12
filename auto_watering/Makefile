# Makefile – auto_watering (ESP32-H2)
# Thin wrapper around idf.py for convenience.
#
# Usage:
#   make               → build
#   make flash         → flash (auto-detect port)
#   make monitor       → open serial monitor
#   make flash monitor → flash then monitor (two targets in one call)
#   make PORT=/dev/ttyUSB0 flash monitor

# ── Target chip ────────────────────────────────────────────────────────────
TARGET   ?= esp32h2

# ── Serial port (macOS: cu.usbserial-*, Linux: ttyUSB0 / ttyACM0) ─────────
#    AUTO-DETECT: first USB-serial port found; override with PORT=...
PORT     ?= $(firstword $(wildcard /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART \
                                   /dev/ttyUSB0 /dev/ttyACM0))
BAUD     ?= 115200

IDF_PY   := idf.py
ifdef PORT
  PORT_ARG := -p $(PORT)
endif

# ── Default target ─────────────────────────────────────────────────────────
.PHONY: all
all: build

# ── Build ──────────────────────────────────────────────────────────────────
.PHONY: build
build:
	$(IDF_PY) build

# ── Set chip target (run once after clone / clean) ─────────────────────────
.PHONY: set-target
set-target:
	$(IDF_PY) set-target $(TARGET)

# ── Interactive config ─────────────────────────────────────────────────────
.PHONY: menuconfig
menuconfig:
	$(IDF_PY) menuconfig

# ── Flash ──────────────────────────────────────────────────────────────────
.PHONY: flash
flash:
	$(IDF_PY) $(PORT_ARG) -b $(BAUD) flash

# ── Serial monitor ─────────────────────────────────────────────────────────
.PHONY: monitor
monitor:
	$(IDF_PY) $(PORT_ARG) -b $(BAUD) monitor

# ── Flash + monitor in one step ────────────────────────────────────────────
.PHONY: flash-monitor
flash-monitor:
	$(IDF_PY) $(PORT_ARG) -b $(BAUD) flash monitor

# ── Erase entire flash (use before first flash on a new board) ─────────────
.PHONY: erase
erase:
	$(IDF_PY) $(PORT_ARG) erase-flash

# ── Binary size report ─────────────────────────────────────────────────────
.PHONY: size
size:
	$(IDF_PY) size

.PHONY: size-components
size-components:
	$(IDF_PY) size-components

# ── Clean ──────────────────────────────────────────────────────────────────
.PHONY: clean
clean:
	$(IDF_PY) clean

.PHONY: fullclean
fullclean:
	$(IDF_PY) fullclean

# ── Help ───────────────────────────────────────────────────────────────────
.PHONY: help
help:
	@echo ""
	@echo "  auto_watering – ESP32-H2 build targets"
	@echo ""
	@echo "  make [all]          Build the project"
	@echo "  make set-target     Set chip to $(TARGET) (run once after clean)"
	@echo "  make menuconfig     Open interactive Kconfig menu"
	@echo "  make flash          Flash firmware  (PORT=$(PORT))"
	@echo "  make monitor        Open serial monitor"
	@echo "  make flash-monitor  Flash then open monitor"
	@echo "  make erase          Erase entire flash"
	@echo "  make size           Show binary size summary"
	@echo "  make size-components Show per-component sizes"
	@echo "  make clean          Remove build artefacts"
	@echo "  make fullclean      Remove build dir + sdkconfig"
	@echo ""
	@echo "  Override port:  make PORT=/dev/cu.usbserial-0001 flash"
	@echo "  Override baud:  make BAUD=921600 flash"
	@echo ""
