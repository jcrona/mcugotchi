# Board selection
BOARD ?= discovery_stm32f0
FWCFG += -DCFG_$(BOARD)_board

ROOT = ../

# GNU ARM Embedded Toolchain location
TOOLCHAIN = $(ROOT)/toolchain/gcc-arm-none-eabi-6-2017-q2-update

# OpenOCD location
OPENOCD_BIN = $(ROOT)/openocd/bin/openocd


ifeq ($(BOARD), discovery_stm32f0)
	FWCFG  += -DSTM32F072xB
	HALTYPE = STM32F0
	LD_FILE ?= stm32f072xb.ld
	OPENOCD_CFG_FILE = board/stm32f0discovery.cfg
else ifeq ($(BOARD), opentama)
	FWCFG  += -DSTM32L072xx
	HALTYPE = STM32L0
	LD_FILE ?= stm32l072xb.ld
endif

BUILDDIR  = build/$(BOARD)
HALDIR    = devices/$(HALTYPE)
SRCDIR    = src

ifeq ($(HALTYPE), STM32F0)
MCU       = cortex-m0
STLIBROOT = libs/STM32CubeF0

INC       = -I$(STLIBROOT)/Drivers/CMSIS/Include
INC      += -I$(STLIBROOT)/Drivers/CMSIS/Device/ST/STM32F0xx/Include/
INC      += -I$(STLIBROOT)/Drivers/STM32F0xx_HAL_Driver/Inc/
INC      += -I$(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc/
INC      += -I$(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc/

STLIBDIR   = $(STLIBROOT)/Drivers/STM32F0xx_HAL_Driver/Src/
USBCORELIB = $(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/
USBMSCLIB  = $(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/
else ifeq ($(HALTYPE), STM32L0)
MCU       = cortex-m0
STLIBROOT = libs/STM32CubeL0

INC       = -I$(STLIBROOT)/Drivers/CMSIS/Include
INC      += -I$(STLIBROOT)/Drivers/CMSIS/Device/ST/STM32L0xx/Include/
INC      += -I$(STLIBROOT)/Drivers/STM32L0xx_HAL_Driver/Inc/
INC      += -I$(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc/
INC      += -I$(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc/

STLIBDIR   = $(STLIBROOT)/Drivers/STM32L0xx_HAL_Driver/Src/
USBCORELIB = $(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/
USBMSCLIB  = $(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/
endif

INC       += -Ilibs/FatFs/src/
FATFSLIB   = libs/FatFs/src/

SRCS = $(wildcard $(HALDIR)/*.c $(HALDIR)/*.s $(SRCDIR)/*.c $(SRCDIR)/lib/*.c)
INC += -I$(HALDIR) -I$(SRCDIR)

TARGET = mcugotchi

CC    = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
AS    = $(TOOLCHAIN)/bin/arm-none-eabi-as
LN    = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
STRIP = $(TOOLCHAIN)/bin/arm-none-eabi-strip
HEX   = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy -O ihex
BIN   = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy -O binary

DEBUG = gdb

CCOPTS   = -mcpu=$(MCU) -mthumb -c -std=gnu99 -g$(DEBUG)
CCOPTS  += -fno-common -fmessage-length=0 -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -Os -Wall -Wshadow -Wstrict-aliasing -Wstrict-overflow -Wno-missing-field-initializers
ASOPTS   = -mcpu=$(MCU) -mthumb -g$(DEBUG)
LNLIBS   =
LNOPTS   = -mcpu=$(MCU) -mthumb -Wl,--gc-sections -Wl,-L$(HALDIR) -Wl,-Map=$(BUILDDIR)/$(TARGET).map -Wl,-T$(LD_FILE)

vpath %.c $(SRCDIR) $(SRCDIR)/lib $(STLIBDIR) $(USBCORELIB) $(USBMSCLIB) $(FATFSLIB) $(HALDIR)
vpath %.s $(SRCDIR) $(STLIBDIR) $(HALDIR)


# Source files from ST Library if any
ifneq ($(STLIBDIR), "")
SRCS += $(filter-out $(wildcard $(STLIBDIR)/*_template.c), $(wildcard $(STLIBDIR)/*.c))
SRCS += $(filter-out $(wildcard $(USBCORELIB)/*_template.c), $(wildcard $(USBCORELIB)/*.c))
SRCS += $(filter-out $(wildcard $(USBMSCLIB)/*_template.c), $(wildcard $(USBMSCLIB)/*.c))
SRCS += $(wildcard $(FATFSLIB)/*.c)
endif

OBJS = $(patsubst %, $(BUILDDIR)/%.o, $(notdir $(basename $(SRCS))))

all: $(BUILDDIR)/$(TARGET).bin $(BUILDDIR)/$(TARGET).hex

$(BUILDDIR)/%.o: %.c
	@echo "[CC $@]"
	@$(CC) $(CCOPTS) $(FWCFG) $(INC) $< -o$@

$(BUILDDIR)/%.o: %.s
	@echo "[AS $@]"
	@$(AS) $(ASOPTS) $(INC) $< -o $@

$(OBJS): Makefile | $(BUILDDIR) show_board

$(BUILDDIR)/%.out: $(OBJS)
	@echo
	@echo "[LD $@]"
	@$(LN) $(LNOPTS) -o $@ $^ $(LNLIBS)

$(BUILDDIR)/%.bin: $(BUILDDIR)/%.out
	@echo "[BIN $@]"
	@$(BIN) $< $(BINOPTS) $@

$(BUILDDIR)/%.hex: $(BUILDDIR)/%.out
	@echo "[HEX $@]"
	@$(HEX) $< $(BINOPTS) $@

flash: $(BUILDDIR)/$(TARGET).out
	@echo
	@$(OPENOCD_BIN) -f $(OPENOCD_CFG_FILE) -c 'reset_config srst_only connect_assert_srst' -c init -c "reset halt" -c targets -c "poll off" -c "flash write_image erase $(BUILDDIR)/$(TARGET).out" -c "sleep 100" -c "reset run" -c shutdown

clean:
	rm -rf $(BUILDDIR)

show_board:
	@echo
	@echo "Building for board $(BOARD) ..."
	@echo

$(BUILDDIR):
	mkdir -p $@

.PHONY: all flash clean show_board

.SECONDARY:
