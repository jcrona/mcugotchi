# Board selection
BOARD ?= opentama
FWCFG += -DBOARD_IS_$(BOARD)

ROOT = ../

# GNU ARM Embedded Toolchain location
TOOLCHAIN = $(ROOT)/toolchain/gcc-arm-none-eabi-10.3-2021.10

# OpenOCD location
OPENOCD_BIN = $(ROOT)/openocd/bin/openocd

# dfu-util location
DFUUTIL_BIN = dfu-util


ifeq ($(BOARD), discovery_stm32f0)
	FWCFG  += -DSTM32F072xB
	MCU = STM32F0
	LD_FILE ?= stm32f072xb.ld
	OPENOCD_CFG_FILE = board/stm32f0discovery.cfg
	FLASHTOOL = openocd
else ifeq ($(BOARD), opentama)
	FWCFG  += -DSTM32L072xx
	MCU = STM32L0
	LD_FILE ?= stm32l072xb.ld
	FLASHTOOL = dfu-util
endif

BUILDDIR     = build/$(BOARD)
SRCDIR       = src
HALINCDIR    = $(SRCDIR)/mcu/inc
HALCOMMONDIR = $(SRCDIR)/mcu/stm32
HALDIR       = $(HALCOMMONDIR)/$(MCU)

ifeq ($(MCU), STM32F0)
CPU       = cortex-m0
STLIBROOT = libs/STM32CubeF0

INC       = -I$(STLIBROOT)/Drivers/CMSIS/Include
INC      += -I$(STLIBROOT)/Drivers/CMSIS/Device/ST/STM32F0xx/Include/
INC      += -I$(STLIBROOT)/Drivers/STM32F0xx_HAL_Driver/Inc/
INC      += -I$(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc/
INC      += -I$(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc/

STLIBDIR   = $(STLIBROOT)/Drivers/STM32F0xx_HAL_Driver/Src/
USBCORELIB = $(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/
USBMSCLIB  = $(STLIBROOT)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/
else ifeq ($(MCU), STM32L0)
CPU       = cortex-m0
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

SRCS = $(wildcard $(HALCOMMONDIR)/*.c $(HALDIR)/*.s $(SRCDIR)/*.c $(SRCDIR)/lib/*.c)
INC += -I$(HALINCDIR) -I$(HALCOMMONDIR) -I$(HALDIR) -I$(SRCDIR)

# Include only one board file
SRCS += $(filter-out $(wildcard $(HALDIR)/board_*.c), $(wildcard $(HALDIR)/*.c))
SRCS += $(HALDIR)/board_$(BOARD).c

TARGET = mcugotchi

CC    = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
AS    = $(TOOLCHAIN)/bin/arm-none-eabi-as
LN    = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
STRIP = $(TOOLCHAIN)/bin/arm-none-eabi-strip
HEX   = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy -O ihex
BIN   = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy -O binary

DEBUG = gdb

CCOPTS   = -mcpu=$(CPU) -mthumb -c -std=gnu99 -g$(DEBUG)
CCOPTS  += -fno-common -fmessage-length=0 -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -Os -Wall -Wshadow -Wstrict-aliasing -Wstrict-overflow -Wno-missing-field-initializers -flto
ASOPTS   = -mcpu=$(CPU) -mthumb -g$(DEBUG)
LNLIBS   =
LNOPTS   = -mcpu=$(CPU) -mthumb -Wl,--gc-sections -Wl,-L$(HALDIR) -Wl,-Map=$(BUILDDIR)/$(TARGET).map -Wl,-T$(LD_FILE) --specs=nano.specs -Wl,-flto

vpath %.c $(SRCDIR) $(SRCDIR)/lib $(STLIBDIR) $(USBCORELIB) $(USBMSCLIB) $(FATFSLIB) $(HALCOMMONDIR) $(HALDIR)
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

ifeq ($(FLASHTOOL), openocd)
flash: $(BUILDDIR)/$(TARGET).out
	@echo
	@$(OPENOCD_BIN) -f $(OPENOCD_CFG_FILE) -c 'reset_config srst_only connect_assert_srst' -c init -c "reset halt" -c targets -c "poll off" -c "flash write_image erase $(BUILDDIR)/$(TARGET).out" -c "sleep 100" -c "reset run" -c shutdown
else ifeq ($(FLASHTOOL), dfu-util)
flash: $(BUILDDIR)/$(TARGET).bin
	@echo
	@$(DFUUTIL_BIN) -a 0 -s 0x08000000:leave -D $(BUILDDIR)/$(TARGET).bin
else
flash:
	@echo "I do not know how to flash this board !"
	@echo
endif

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
