# MicroRTOS Makefile for Arduino (AVR)
#
# Usage:
#   make              - Build all (release mode)
#   make debug        - Build with debug symbols and assertions
#   make strict       - Build with maximum warnings (MISRA compliance)
#   make analyze      - Run static analysis
#   make misra-check  - Check MISRA C:2012 compliance
#   make example1     - Build basic_tasks example
#   make clean        - Clean build artifacts
#
# Standards Compliance:
#   - MISRA C:2012
#   - IEC 61508 / ISO 26262
#   - CERT C Coding Standard

#==============================================================================
# Configuration
#==============================================================================

# Arduino toolchain paths
ARDUINO_PATH = /home/sitharaj/.arduino15/packages/arduino
AVR_GCC_PATH = $(ARDUINO_PATH)/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin
AVR_INCLUDE = $(ARDUINO_PATH)/hardware/avr/1.8.7/cores/arduino

# Tools
CC = $(AVR_GCC_PATH)/avr-gcc
CXX = $(AVR_GCC_PATH)/avr-g++
AR = $(AVR_GCC_PATH)/avr-ar
OBJCOPY = $(AVR_GCC_PATH)/avr-objcopy
SIZE = $(AVR_GCC_PATH)/avr-size

# Static analysis tools (if available)
CPPCHECK ?= cppcheck
PCLINT ?= pclint

# Target MCU (ATmega328P for Arduino Uno)
MCU = atmega328p
F_CPU = 16000000UL

#==============================================================================
# Compiler Flags
#==============================================================================

# Base flags (GNU C11 for embedded compatibility)
CFLAGS_BASE = -c -std=gnu11 -ffunction-sections -fdata-sections
CFLAGS_BASE += -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DRTOS_PLATFORM_AVR=1
CFLAGS_BASE += -I./include

# Release flags (optimized, minimal warnings)
CFLAGS_RELEASE = $(CFLAGS_BASE) -Os -w -DNDEBUG

# Debug flags (with assertions and debug info)
CFLAGS_DEBUG = $(CFLAGS_BASE) -Og -g3 -DRTOS_USE_ASSERT=1 -DRTOS_SAFETY_ENABLE=1

# MISRA/Strict warning flags (for static analysis compliance)
CFLAGS_STRICT = $(CFLAGS_BASE) -O2 -g
CFLAGS_STRICT += -Wall -Wextra -Wpedantic
CFLAGS_STRICT += -Wconversion -Wsign-conversion
CFLAGS_STRICT += -Wshadow -Wundef -Wunused
CFLAGS_STRICT += -Wstrict-prototypes -Wmissing-prototypes
CFLAGS_STRICT += -Wcast-align -Wcast-qual
CFLAGS_STRICT += -Wwrite-strings
CFLAGS_STRICT += -Wswitch-default -Wswitch-enum
CFLAGS_STRICT += -Wunreachable-code
CFLAGS_STRICT += -Wformat=2 -Wformat-security
CFLAGS_STRICT += -Wdouble-promotion
CFLAGS_STRICT += -Wnull-dereference
CFLAGS_STRICT += -Wlogical-op
CFLAGS_STRICT += -Wduplicated-cond
CFLAGS_STRICT += -Wduplicated-branches
CFLAGS_STRICT += -Wrestrict
# -fanalyzer requires gcc >= 10. The bundled Arduino AVR toolchain (7.3) does
# not have it; enable via `make strict ANALYZER=1` on a newer host gcc.
ANALYZER ?= 0
ifeq ($(ANALYZER),1)
CFLAGS_STRICT += -fanalyzer
endif
CFLAGS_STRICT += -DRTOS_USE_ASSERT=1 -DRTOS_SAFETY_ENABLE=1

# Default to release build
CFLAGS = $(CFLAGS_RELEASE)

# Linker flags
LDFLAGS = -w -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections
LDFLAGS += -mmcu=$(MCU)

# Build directory
BUILD_DIR = build

# Source files
CORE_SRCS = \
	src/core/rtos_list.c \
	src/core/rtos_task.c \
	src/core/rtos_scheduler.c \
	src/core/rtos_kernel.c \
	src/core/rtos_lockfree.c \
	src/core/rtos_tickless.c \
	src/core/rtos_async.c \
	src/core/rtos_deadline.c \
	src/core/rtos_mpu.c \
	src/core/rtos_smp.c \
	src/sync/rtos_mutex.c \
	src/sync/rtos_semaphore.c \
	src/sync/rtos_event.c \
	src/ipc/rtos_queue.c \
	src/ipc/rtos_zerocopy.c \
	src/time/rtos_timer.c \
	src/memory/rtos_mempool.c \
	src/diag/rtos_trace.c \
	src/diag/rtos_watchdog.c \
	src/diag/rtos_safety.c \
	src/port/avr/rtos_port_avr.c

# Header files (for dependency tracking)
HEADERS = \
	include/rtos.h \
	include/rtos_config.h \
	include/rtos_types.h \
	include/rtos_misra.h \
	include/rtos_safety.h \
	include/rtos_port.h \
	include/rtos_list.h \
	include/rtos_task.h

# Object files
CORE_OBJS = $(CORE_SRCS:%.c=$(BUILD_DIR)/%.o)

# Library
RTOS_LIB = $(BUILD_DIR)/librtos.a

# Default target
all: $(BUILD_DIR) $(RTOS_LIB)
	@echo "Build complete!"
	@$(SIZE) $(RTOS_LIB) || true

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/src/core
	@mkdir -p $(BUILD_DIR)/src/sync
	@mkdir -p $(BUILD_DIR)/src/ipc
	@mkdir -p $(BUILD_DIR)/src/time
	@mkdir -p $(BUILD_DIR)/src/memory
	@mkdir -p $(BUILD_DIR)/src/diag
	@mkdir -p $(BUILD_DIR)/src/port/avr
	@mkdir -p $(BUILD_DIR)/examples

# Build library
$(RTOS_LIB): $(CORE_OBJS)
	@echo "Creating library: $@"
	@$(AR) rcs $@ $^

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) $< -o $@

# All examples
examples: example1 example2 example3 example4 example5 example7 example8
	@echo "All examples built successfully!"
	@echo "Note: example6 (SMP) needs a multi-core port; see examples/06_smp_dual_core/README.md"

# Example 1: Basic Tasks
example1: $(RTOS_LIB)
	@echo "Building example: 01_basic_tasks"
	@$(CC) $(CFLAGS) examples/01_basic_tasks/main.c -o $(BUILD_DIR)/examples/example1.o
	@$(CC) $(LDFLAGS) $(BUILD_DIR)/examples/example1.o $(RTOS_LIB) -o $(BUILD_DIR)/examples/example1.elf
	@$(SIZE) $(BUILD_DIR)/examples/example1.elf
	@echo "Example 1 built successfully!"

# Example 2: Mutex Demo
example2: $(RTOS_LIB)
	@echo "Building example: 02_mutex_demo"
	@$(CC) $(CFLAGS) examples/02_mutex_demo/main.c -o $(BUILD_DIR)/examples/example2.o
	@$(CC) $(LDFLAGS) $(BUILD_DIR)/examples/example2.o $(RTOS_LIB) -o $(BUILD_DIR)/examples/example2.elf
	@$(SIZE) $(BUILD_DIR)/examples/example2.elf
	@echo "Example 2 built successfully!"

# Example 3: Producer-Consumer
example3: $(RTOS_LIB)
	@echo "Building example: 03_producer_consumer"
	@$(CC) $(CFLAGS) examples/03_producer_consumer/main.c -o $(BUILD_DIR)/examples/example3.o
	@$(CC) $(LDFLAGS) $(BUILD_DIR)/examples/example3.o $(RTOS_LIB) -o $(BUILD_DIR)/examples/example3.elf
	@$(SIZE) $(BUILD_DIR)/examples/example3.elf
	@echo "Example 3 built successfully!"

# Example 4: Timer Events
example4: $(RTOS_LIB)
	@echo "Building example: 04_timer_events"
	@$(CC) $(CFLAGS) examples/04_timer_events/main.c -o $(BUILD_DIR)/examples/example4.o
	@$(CC) $(LDFLAGS) $(BUILD_DIR)/examples/example4.o $(RTOS_LIB) -o $(BUILD_DIR)/examples/example4.elf
	@$(SIZE) $(BUILD_DIR)/examples/example4.elf
	@echo "Example 4 built successfully!"

# Example 5: Tickless Low Power
example5: $(RTOS_LIB)
	@echo "Building example: 05_tickless_low_power"
	@$(CC) $(CFLAGS) examples/05_tickless_low_power/main.c -o $(BUILD_DIR)/examples/example5.o
	@$(CC) $(LDFLAGS) $(BUILD_DIR)/examples/example5.o $(RTOS_LIB) -o $(BUILD_DIR)/examples/example5.elf
	@$(SIZE) $(BUILD_DIR)/examples/example5.elf
	@echo "Example 5 built successfully!"

# Example 7: Async / Await
example7: $(RTOS_LIB)
	@echo "Building example: 07_async_await"
	@$(CC) $(CFLAGS) examples/07_async_await/main.c -o $(BUILD_DIR)/examples/example7.o
	@$(CC) $(LDFLAGS) $(BUILD_DIR)/examples/example7.o $(RTOS_LIB) -o $(BUILD_DIR)/examples/example7.elf
	@$(SIZE) $(BUILD_DIR)/examples/example7.elf
	@echo "Example 7 built successfully!"

# Example 8: EDF Deadline Scheduling
# Built against a feature-enabled library — rtos_deadline.c needs
# -DRTOS_USE_EDF_SCHEDULER=1 to produce real code instead of a stub.
EDF_LIB = $(BUILD_DIR)/librtos_edf.a
EDF_DEADLINE_OBJ = $(BUILD_DIR)/src/core/rtos_deadline.edf.o
# All core objects EXCEPT the default (stub) rtos_deadline.o; we substitute
# the EDF-enabled variant for it below.
EDF_BASE_OBJS = $(filter-out $(BUILD_DIR)/src/core/rtos_deadline.o, $(CORE_OBJS))

$(EDF_DEADLINE_OBJ): src/core/rtos_deadline.c | $(BUILD_DIR)
	@echo "Compiling (EDF on): $<"
	@$(CC) $(CFLAGS) -DRTOS_USE_EDF_SCHEDULER=1 -c $< -o $@

$(EDF_LIB): $(EDF_BASE_OBJS) $(EDF_DEADLINE_OBJ)
	@echo "Creating EDF library: $@"
	@$(AR) rcs $@ $(EDF_BASE_OBJS) $(EDF_DEADLINE_OBJ)

example8: $(EDF_LIB)
	@echo "Building example: 08_edf_deadline"
	@$(CC) $(CFLAGS) -DRTOS_USE_EDF_SCHEDULER=1 examples/08_edf_deadline/main.c -o $(BUILD_DIR)/examples/example8.o
	@$(CC) $(LDFLAGS) $(BUILD_DIR)/examples/example8.o $(EDF_LIB) -o $(BUILD_DIR)/examples/example8.elf
	@$(SIZE) $(BUILD_DIR)/examples/example8.elf
	@echo "Example 8 built successfully!"

#==============================================================================
# Flash to Arduino Uno
#==============================================================================
#
# Usage:
#   make flash EXAMPLE=1            -- flash example1 over /dev/ttyACM0
#   make flash EXAMPLE=4 PORT=/dev/ttyACM1
#
# Requires:
#   - avrdude in PATH (bundled with the Arduino toolchain)
#   - User in the 'dialout' group so /dev/ttyACM* is writable
#   - The selected example to have been built (we depend on the .elf)

# Default to example 1 and the usual Uno serial port.
EXAMPLE ?= 1
PORT    ?= /dev/ttyACM0

# Pull avrdude from the same toolchain bundle we use for avr-gcc.
AVRDUDE      = $(ARDUINO_PATH)/tools/avrdude/8.0.0-arduino1/bin/avrdude
AVRDUDE_CONF = $(ARDUINO_PATH)/tools/avrdude/8.0.0-arduino1/etc/avrdude.conf
# Arduino Uno bootloader speaks the 'arduino' (stk500) protocol at 115200.
AVRDUDE_FLAGS = -C $(AVRDUDE_CONF) -p atmega328p -c arduino -P $(PORT) -b 115200

EXAMPLE_ELF = $(BUILD_DIR)/examples/example$(EXAMPLE).elf
EXAMPLE_HEX = $(BUILD_DIR)/examples/example$(EXAMPLE).hex

# Build the .hex from the .elf by stripping debug sections.
$(EXAMPLE_HEX): $(EXAMPLE_ELF)
	@echo "Generating Intel HEX: $@"
	@$(OBJCOPY) -O ihex -R .eeprom $< $@
	@$(SIZE) $<

flash: $(EXAMPLE_HEX)
	@if [ ! -e $(PORT) ]; then \
		echo "ERROR: $(PORT) not found. Plug in the Uno or set PORT=..."; \
		exit 1; \
	fi
	@if [ ! -w $(PORT) ]; then \
		echo "ERROR: $(PORT) is not writable by you."; \
		echo "       Add yourself to the dialout group: sudo usermod -aG dialout $$USER"; \
		echo "       Then log out and back in, or run 'newgrp dialout'."; \
		exit 1; \
	fi
	@echo "Flashing $(EXAMPLE_HEX) to $(PORT)..."
	@$(AVRDUDE) $(AVRDUDE_FLAGS) -D -U flash:w:$(EXAMPLE_HEX):i
	@echo "Done. Reset the board if it does not start automatically."

# Read back the flash contents and print a hash. Useful for verifying
# that a flash actually wrote what we intended.
flash-verify: $(EXAMPLE_HEX)
	@$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:v:$(EXAMPLE_HEX):i

# Clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete!"

# Check syntax only (no linking)
check: $(BUILD_DIR)
	@echo "Checking syntax..."
	@for src in $(CORE_SRCS); do \
		echo "Checking: $$src"; \
		$(CC) $(CFLAGS) -fsyntax-only $$src || exit 1; \
	done
	@echo "Syntax check passed!"

# Check that every advanced feature still compiles when enabled.
# These flags are off by default, so the regular check above does not exercise them.
check-features: $(BUILD_DIR)
	@echo "Checking syntax with each advanced feature enabled..."
	@for flag in RTOS_USE_EDF_SCHEDULER RTOS_USE_MPU RTOS_USE_SMP \
	             RTOS_USE_WATCHDOG RTOS_USE_TASK_WATCHDOG \
	             RTOS_USE_TRACING RTOS_USE_ZEROCOPY; do \
		echo "  -- $$flag=1"; \
		$(CC) $(CFLAGS) -D$$flag=1 -fsyntax-only \
			src/core/rtos_deadline.c src/core/rtos_mpu.c src/core/rtos_smp.c \
			src/diag/rtos_watchdog.c src/diag/rtos_trace.c src/ipc/rtos_zerocopy.c \
			|| exit 1; \
	done
	@echo "Feature syntax check passed!"

#==============================================================================
# ARM Port Syntax Check
#==============================================================================
#
# Use arm-none-eabi-gcc if available to verify the ARM port still compiles.
# This does not link a full image; it is a structural check only.
ARM_GCC ?= arm-none-eabi-gcc
ARM_CFLAGS = -c -std=gnu11 -mcpu=cortex-m3 -mthumb \
             -ffunction-sections -fdata-sections \
             -DRTOS_PLATFORM_ARM=1 -DF_CPU=84000000UL \
             -I./include

check-arm:
	@if ! command -v $(ARM_GCC) >/dev/null 2>&1; then \
		echo "Skipping ARM check: $(ARM_GCC) not in PATH"; \
		echo "  Install with: apt install gcc-arm-none-eabi"; \
	else \
		echo "Checking ARM port syntax with $(ARM_GCC)..."; \
		$(ARM_GCC) $(ARM_CFLAGS) -fsyntax-only src/port/arm/rtos_port_arm.c || exit 1; \
		for src in $(filter-out src/port/avr/rtos_port_avr.c, $(CORE_SRCS)); do \
			echo "  -- $$src"; \
			$(ARM_GCC) $(ARM_CFLAGS) -fsyntax-only $$src || exit 1; \
		done; \
		echo "ARM syntax check passed!"; \
	fi

# Same per-feature matrix as check-features, but for the ARM Cortex-M target.
# Catches feature regressions that AVR misses (e.g. MPU support is ARM-only).
check-arm-features:
	@if ! command -v $(ARM_GCC) >/dev/null 2>&1; then \
		echo "Skipping ARM feature check: $(ARM_GCC) not in PATH"; \
	else \
		echo "Checking ARM syntax with each advanced feature enabled..."; \
		for flag in RTOS_USE_EDF_SCHEDULER RTOS_USE_MPU RTOS_USE_SMP \
		             RTOS_USE_WATCHDOG RTOS_USE_TASK_WATCHDOG \
		             RTOS_USE_TRACING RTOS_USE_ZEROCOPY; do \
			echo "  -- $$flag=1"; \
			$(ARM_GCC) $(ARM_CFLAGS) -D$$flag=1 -fsyntax-only \
				src/core/rtos_deadline.c src/core/rtos_mpu.c src/core/rtos_smp.c \
				src/diag/rtos_watchdog.c src/diag/rtos_trace.c src/ipc/rtos_zerocopy.c \
				|| exit 1; \
		done; \
		echo "ARM feature syntax check passed!"; \
	fi

#==============================================================================
# Unit Tests (host-side, plain gcc)
#==============================================================================

HOST_CC ?= gcc
# RTOS_PLATFORM_HOST short-circuits the chip-specific includes in rtos_port.h
# so host tests link against plain libc. Host stubs provide rtos_assert_failed
# and the kernel globals/port shims that the modules under test reference.
HOST_CFLAGS = -std=gnu11 -O0 -g -Wall -Wextra -I./include \
              -DRTOS_PLATFORM_HOST=1

HOST_STUBS = tests/unit/host_stubs.c
HOST_TEST_INC = -I tests/unit

test: $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/tests
	@echo "Building host unit tests..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_TEST_INC) \
		tests/unit/test_list.c $(HOST_STUBS) src/core/rtos_list.c \
		-o $(BUILD_DIR)/tests/test_list
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_TEST_INC) \
		tests/unit/test_mempool.c $(HOST_STUBS) \
		src/core/rtos_list.c src/memory/rtos_mempool.c \
		-o $(BUILD_DIR)/tests/test_mempool
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_TEST_INC) \
		tests/unit/test_queue.c $(HOST_STUBS) \
		src/core/rtos_list.c src/ipc/rtos_queue.c \
		-o $(BUILD_DIR)/tests/test_queue
	@echo "Running host unit tests..."
	@set -e; for t in $(BUILD_DIR)/tests/test_list $(BUILD_DIR)/tests/test_mempool $(BUILD_DIR)/tests/test_queue; do \
		$$t; \
	done

#==============================================================================
# Debug Build
#==============================================================================

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: clean $(BUILD_DIR) $(RTOS_LIB)
	@echo "Debug build complete!"
	@$(SIZE) $(RTOS_LIB) || true

#==============================================================================
# Strict Build (MISRA Compliance)
#==============================================================================

strict: CFLAGS = $(CFLAGS_STRICT)
strict: clean $(BUILD_DIR)
	@echo "Building with strict MISRA-compliant warnings..."
	@for src in $(CORE_SRCS); do \
		echo "Compiling (strict): $$src"; \
		$(CC) $(CFLAGS) $$src -o $(BUILD_DIR)/$${src%.c}.o 2>&1 | tee -a $(BUILD_DIR)/warnings.log; \
	done
	@echo "Creating library..."
	@$(AR) rcs $(RTOS_LIB) $(CORE_OBJS)
	@echo ""
	@echo "=========================================="
	@echo "MISRA Compliance Build Complete"
	@echo "=========================================="
	@echo "Warnings logged to: $(BUILD_DIR)/warnings.log"
	@if [ -s $(BUILD_DIR)/warnings.log ]; then \
		echo "WARNING: Some warnings were generated. Review for MISRA compliance."; \
		wc -l $(BUILD_DIR)/warnings.log; \
	else \
		echo "SUCCESS: No warnings generated!"; \
	fi

#==============================================================================
# Static Analysis
#==============================================================================

analyze: $(BUILD_DIR)
	@echo "Running static analysis..."
	@echo ""
	@echo "=== GCC Static Analyzer ==="
	@if $(CC) -fanalyzer -E - </dev/null >/dev/null 2>&1; then \
		for src in $(CORE_SRCS); do \
			echo "Analyzing: $$src"; \
			$(CC) $(CFLAGS_STRICT) -fanalyzer $$src -o /dev/null 2>&1 | grep -E "warning:|error:" || true; \
		done; \
	else \
		echo "Skipped: $(CC) does not support -fanalyzer (needs gcc >= 10)"; \
	fi
	@echo ""
	@echo "=== cppcheck (if available) ==="
	@if command -v $(CPPCHECK) >/dev/null 2>&1; then \
		$(CPPCHECK) --enable=all --std=c99 \
			--suppress=missingIncludeSystem \
			--suppress=unusedFunction \
			-I./include \
			--force --quiet \
			$(CORE_SRCS) 2>&1; \
	else \
		echo "cppcheck not found. Install with: apt install cppcheck"; \
	fi
	@echo ""
	@echo "Static analysis complete!"

#==============================================================================
# MISRA C:2012 Compliance Check
#==============================================================================

misra-check: $(BUILD_DIR)
	@echo "=========================================="
	@echo "MISRA C:2012 Compliance Check"
	@echo "=========================================="
	@echo ""
	@echo "Checking MISRA compliance using cppcheck..."
	@if command -v $(CPPCHECK) >/dev/null 2>&1; then \
		$(CPPCHECK) --addon=misra.json \
			--std=c99 \
			-I./include \
			--force \
			--xml \
			$(CORE_SRCS) 2> $(BUILD_DIR)/misra-report.xml; \
		echo "MISRA report saved to: $(BUILD_DIR)/misra-report.xml"; \
	else \
		echo "cppcheck not found. Install with: apt install cppcheck"; \
		echo "For full MISRA checking, use PC-lint Plus or Polyspace."; \
	fi
	@echo ""
	@echo "Manual MISRA compliance items to verify:"
	@echo "  - Rule 1.1: Compiler extensions (inline asm in port layer)"
	@echo "  - Rule 11.3: Pointer casts (hardware registers)"
	@echo "  - Rule 14.3: Infinite loops (scheduler, fault handlers)"
	@echo ""
	@echo "See docs/MISRA_COMPLIANCE.md for full compliance matrix."

#==============================================================================
# Code Metrics
#==============================================================================

metrics: $(BUILD_DIR)
	@echo "=========================================="
	@echo "Code Metrics Report"
	@echo "=========================================="
	@echo ""
	@echo "=== Lines of Code ==="
	@wc -l $(CORE_SRCS) | tail -1
	@echo ""
	@echo "=== Source Files ==="
	@ls -la $(CORE_SRCS) | wc -l
	@echo ""
	@echo "=== Header Files ==="
	@find include -name "*.h" | wc -l
	@echo ""
	@echo "=== Cyclomatic Complexity (estimate) ==="
	@echo "Counting control structures in source files..."
	@grep -c -E "if|while|for|switch|case" $(CORE_SRCS) || true
	@echo ""
	@echo "=== Binary Size ==="
	@if [ -f $(RTOS_LIB) ]; then \
		$(SIZE) $(RTOS_LIB); \
	else \
		echo "Build the library first with 'make'"; \
	fi

#==============================================================================
# Documentation
#==============================================================================

docs:
	@echo "Documentation available in:"
	@echo "  - docs/MISRA_COMPLIANCE.md"
	@echo "  - README.md"
	@echo ""
	@echo "To generate API documentation, install doxygen and run:"
	@echo "  doxygen Doxyfile"

#==============================================================================
# Safety Validation
#==============================================================================

safety-check: $(BUILD_DIR)
	@echo "=========================================="
	@echo "Safety Validation Check"
	@echo "=========================================="
	@echo ""
	@echo "Checking safety-critical patterns..."
	@echo ""
	@echo "=== Stack Overflow Protection ==="
	@grep -l "RTOS_CHECK_STACK_OVERFLOW" include/*.h src/**/*.c || echo "Not found"
	@echo ""
	@echo "=== Assertion Coverage ==="
	@grep -c "RTOS_ASSERT" $(CORE_SRCS) | awk -F: '{sum += $$2} END {print "Total assertions: " sum}'
	@echo ""
	@echo "=== Critical Section Usage ==="
	@grep -c "rtos_port_enter_critical\|rtos_port_exit_critical" $(CORE_SRCS) | awk -F: '{sum += $$2} END {print "Critical section calls: " sum}'
	@echo ""
	@echo "=== Volatile Qualifiers ==="
	@grep -c "volatile" include/*.h $(CORE_SRCS) | awk -F: '{sum += $$2} END {print "Volatile usages: " sum}'
	@echo ""
	@echo "Safety validation complete!"
	@echo "See docs/MISRA_COMPLIANCE.md for full safety analysis."

.PHONY: all clean examples example1 example2 example3 example4 example5 check
.PHONY: debug strict analyze misra-check metrics docs test safety-check
