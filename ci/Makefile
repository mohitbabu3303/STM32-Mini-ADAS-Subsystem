# Command-line firmware build used by GitHub Actions (CI).
# STM32CubeIDE does NOT use this file; it keeps its own managed build.
#
#   make -C ci            -> ci/build/Mini_ADAS_RTOS.elf (+ .hex, .map, memory usage)
#
# Flags mirror the CubeIDE Debug configuration of Mini_ADAS_RTOS.

PROJECT  := Mini_ADAS_RTOS
ROOT     := ../Mini_ADAS_RTOS
BUILD    := build
PREFIX   ?= arm-none-eabi-
CC       := $(PREFIX)gcc
OBJCOPY  := $(PREFIX)objcopy
SIZE     := $(PREFIX)size

MCU      := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
DEFS     := -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB
INCS     := -I$(ROOT)/Core/Inc \
            -I$(ROOT)/Drivers/STM32F1xx_HAL_Driver/Inc \
            -I$(ROOT)/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy \
            -I$(ROOT)/Drivers/CMSIS/Device/ST/STM32F1xx/Include \
            -I$(ROOT)/Drivers/CMSIS/Include \
            -I$(ROOT)/Middlewares/Third_Party/FreeRTOS/Source/include \
            -I$(ROOT)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
            -I$(ROOT)/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3
CFLAGS   := $(MCU) -std=gnu11 -Os -g3 $(DEFS) $(INCS) -ffunction-sections -fdata-sections \
            -Wall --specs=nano.specs
LDFLAGS  := $(MCU) -T$(ROOT)/STM32F103C8TX_FLASH.ld --specs=nosys.specs --specs=nano.specs -static \
            -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(PROJECT).map -Wl,--print-memory-usage \
            -Wl,--start-group -lc -lm -Wl,--end-group

# Application code is built with extra warnings, vendor code with defaults.
APP_SRCS := $(wildcard $(ROOT)/Core/Src/*.c)
VND_SRCS := $(wildcard $(ROOT)/Drivers/STM32F1xx_HAL_Driver/Src/*.c) \
            $(wildcard $(ROOT)/Middlewares/Third_Party/FreeRTOS/Source/*.c) \
            $(ROOT)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c \
            $(ROOT)/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c \
            $(ROOT)/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
ASM_SRCS := $(ROOT)/Core/Startup/startup_stm32f103c8tx.s

obj = $(BUILD)/obj/$(subst /,_,$(subst $(ROOT)/,,$(1))).o
APP_OBJS := $(foreach s,$(APP_SRCS),$(call obj,$(s)))
VND_OBJS := $(foreach s,$(VND_SRCS),$(call obj,$(s)))
ASM_OBJS := $(foreach s,$(ASM_SRCS),$(call obj,$(s)))

all: $(BUILD)/$(PROJECT).elf $(BUILD)/$(PROJECT).hex
	@$(SIZE) $(BUILD)/$(PROJECT).elf

define app_rule
$(call obj,$(1)): $(1) | $(BUILD)/obj
	$(CC) $(CFLAGS) -Wextra -c $$< -o $$@
endef
define vnd_rule
$(call obj,$(1)): $(1) | $(BUILD)/obj
	$(CC) $(CFLAGS) -c $$< -o $$@
endef
define asm_rule
$(call obj,$(1)): $(1) | $(BUILD)/obj
	$(CC) $(MCU) -x assembler-with-cpp -c $$< -o $$@
endef
$(foreach s,$(APP_SRCS),$(eval $(call app_rule,$(s))))
$(foreach s,$(VND_SRCS),$(eval $(call vnd_rule,$(s))))
$(foreach s,$(ASM_SRCS),$(eval $(call asm_rule,$(s))))

$(BUILD)/$(PROJECT).elf: $(APP_OBJS) $(VND_OBJS) $(ASM_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(BUILD)/$(PROJECT).hex: $(BUILD)/$(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/obj:
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean
