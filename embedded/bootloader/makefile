# Include common prelude make file
include $(ROOT_DIR)base.mak

#bootloader only builds for AVR
ifeq ($(ARCH), avr)

# If we're building a library, set IS_LIB and LIBNAME
# If we're building a driver, set IS_DRV and DRVNAME
# If we're building an app, set IS_APP and APPNAME
IS_APP=1
APPNAME=bootloader

#this is the list of the objects required to build the kernel
C_SOURCE=bootloader.c


ifeq ($(VARIANT), atmega328p)
    LFLAGS+=-Wl,-section-start=.text=0x7c00
    CFLAGS+=-DPAGE_SIZE=128
endif
ifeq ($(VARIANT), atmega1284p)
    LFLAGS+=-Wl,-section-start=.text=0x1e000
    CFLAGS+=-DPAGE_SIZE=256
    CFLAGS+=-DBAUD_RATE=115200
    CFLAGS+=-DSYSTEM_FREQ=20000000
endif

endif

# Include the rest of the script that is actually used for building the 
# outputs
include $(ROOT_DIR)build.mak
