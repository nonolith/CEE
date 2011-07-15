##########################################################################
# User configuration and firmware specific object files	
##########################################################################

# The target, flash and ram of the LPC1xxx microprocessor.
# Use for the target the value: LPC11xx, LPC13xx or LPC17xx
TARGET = LPC13xx
FLASH = 32K
SRAM = 8K

# For USB HID support the LPC134x reserves 384 bytes from the sram,
# if you don't want to use the USB features, just use 0 here.
SRAM_USB = 384

##########################################################################
# Library files 
##########################################################################

BUILDDIR = build

CEE_SRC += main.c
SRC += $(CEE_SRC)

MICROBUILDER_SRC += microbuilder/sysinit.c
MICROBUILDER_SRC += microbuilder/core/i2c/i2c.c
MICROBUILDER_SRC += microbuilder/core/timer16/timer16.c
MICROBUILDER_SRC += microbuilder/core/timer32/timer32.c
MICROBUILDER_SRC += microbuilder/core/ssp/ssp.c
MICROBUILDER_SRC += microbuilder/core/iap/iap.c
MICROBUILDER_SRC += microbuilder/core/gpio/gpio.c
MICROBUILDER_SRC += microbuilder/core/uart/uart_buf.c
MICROBUILDER_SRC += microbuilder/core/uart/uart.c
MICROBUILDER_SRC += microbuilder/core/libc/string.c
MICROBUILDER_SRC += microbuilder/core/libc/ctype.c
MICROBUILDER_SRC += microbuilder/core/libc/stdio.c
MICROBUILDER_SRC += microbuilder/core/pmu/pmu.c
MICROBUILDER_SRC += microbuilder/core/cpu/cpu.c
MICROBUILDER_SRC += microbuilder/core/adc/adc.c
MICROBUILDER_SRC += microbuilder/core/systick/systick.c
#MICROBUILDER_SRC += microbuilder/core/pwm/pwm.c
MICROBUILDER_SRC += microbuilder/core/wdt/wdt.c
MICROBUILDER_SRC += microbuilder/lpc1xxx/$(TARGET)_handlers.c
MICROBUILDER_SRC += microbuilder/lpc1xxx/LPC1xxx_startup.c
SRC += $(MICROBUILDER_SRC)

LUFA_SRC+=lufa_usb/Core/Events.c
LUFA_SRC+=lufa_usb/Core/LPC13xx/Device_LPC13xx.c
LUFA_SRC+=lufa_usb/Core/LPC13xx/Endpoint_LPC13xx.c
LUFA_SRC+=lufa_usb/Core/LPC13xx/USBInterrupt_LPC13xx.c
LUFA_SRC+=lufa_usb/Core/LPC13xx/USBController_LPC13xx.c
LUFA_SRC+=lufa_usb/Core/USBTask.c
LUFA_SRC+=lufa_usb/Core/DeviceStandardReq.c
SRC += $(LUFA_SRC)

##########################################################################
# GNU GCC compiler prefix and location
##########################################################################

CROSS_COMPILE = arm-none-eabi-
AS = $(CROSS_COMPILE)gcc
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
GDB = $(CROSS_COMPILE)gdb
OUTFILE = firmware
LPCRC = lpcrc

##########################################################################
# GNU GCC compiler flags
##########################################################################
ROOT_PATH = .
INCLUDE_PATHS = -I$(ROOT_PATH) -Imicrobuilder

##########################################################################
# Startup files
##########################################################################

LD_PATH = microbuilder/lpc1xxx
LD_SCRIPT = $(LD_PATH)/linkscript.ld
LD_TEMP = $(BUILDDIR)/memory.ld

ifeq (LPC11xx,$(TARGET))
  CORTEX_TYPE=m0
else
  CORTEX_TYPE=m3
endif

CPU_TYPE = cortex-$(CORTEX_TYPE)

##########################################################################
# Compiler settings, parameters and flags
##########################################################################

OBJS = $(SRC:.c=.o)
OBJFILES = $(addprefix $(BUILDDIR)/, $(OBJS))

CFLAGS  = -c -g -Os $(INCLUDE_PATHS) -std=gnu99 -Wall -mthumb -ffunction-sections -fdata-sections -fmessage-length=0 -mcpu=$(CPU_TYPE) -DTARGET=$(TARGET) -fno-builtin -fshort-wchar
ASFLAGS = -c -g -Os $(INCLUDE_PATHS) -Wall -mthumb -ffunction-sections -fdata-sections -fmessage-length=0 -mcpu=$(CPU_TYPE) -D__ASSEMBLY__ -x assembler-with-cpp
LDFLAGS = -nostartfiles -mthumb -mcpu=$(CPU_TYPE) -Wl,--gc-sections -Wl,--no-wchar-size-warning
LDLIBS  = -lm
OCFLAGS = --strip-unneeded

all: firmware.bin

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o : %.c $(BUILDDIR)
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/%.o : %.s $(BUILDDIR)
	$(AS) $(ASFLAGS) -o $@ $<

firmware.bin: $(OBJFILES)
	-@echo "MEMORY" > $(LD_TEMP)
	-@echo "{" >> $(LD_TEMP)
	-@echo "  flash(rx): ORIGIN = 0x00000000, LENGTH = $(FLASH)" >> $(LD_TEMP)
	-@echo "  sram(rwx): ORIGIN = 0x10000000+$(SRAM_USB), LENGTH = $(SRAM)-$(SRAM_USB)" >> $(LD_TEMP)
	-@echo "}" >> $(LD_TEMP)
	-@echo "INCLUDE $(LD_SCRIPT)" >> $(LD_TEMP)
	$(LD) $(LDFLAGS) -T $(LD_TEMP) -o $(OUTFILE).elf $(OBJFILES) $(LDLIBS)
	-@echo ""
	$(SIZE) $(OUTFILE).elf
	-@echo ""
	$(OBJCOPY) $(OCFLAGS) -O binary $(OUTFILE).elf $(OUTFILE).bin
	#$(OBJCOPY) $(OCFLAGS) -O ihex $(OUTFILE).elf $(OUTFILE).hex
	-@echo ""
	$(LPCRC) firmware.bin

clean:
	rm -f $(BUILDDIR)



XPRESSO_PATH = /usr/local/LPCXpresso/bin
.PHONY: init-xpresso flash-xpresso debug-xpresso
init-xpresso:
	$(XPRESSO_PATH)/dfu-util -d 0x471:0xdf55 -c 0 -t 2048 -R -D $(XPRESSO_PATH)/LPCXpressoWIN.enc || true
	sleep 1

flash-xpresso: init-xpresso firmware.bin
	$(XPRESSO_PATH)/crt_emu_lpc11_13_nxp -wire=winusb -plpc1343 -flash-load-exec=firmware.bin
	
debug-xpresso: init-xpresso
	echo "target extended-remote | $(XPRESSO_PATH)/crt_emu_lpc11_13_nxp -pLPC1343 -wire=winusb" > xpresso.gdb
	$(GDB) -x xpresso.gdb firmware.elf
