# Include common prelude make file
include $(ROOT_DIR)base.mak

# If we're building a library, set IS_LIB and LIBNAME
# If we're building a driver, set IS_DRV and DRVNAME
# If we're building an app, set IS_APP and APPNAME
IS_LIB=1
LIBNAME=mark3

#this is the list of the objects required to build the kernel
CPP_SOURCE= \
	autoalloc.cpp \
	blocking.cpp \
	driver.cpp \
	eventflag.cpp \
	ll.cpp \
	message.cpp \
	mutex.cpp \
    notify.cpp \
	profile.cpp \
    priomap.cpp \
	quantum.cpp \
	scheduler.cpp \
	ksemaphore.cpp \
	mailbox.cpp \
	thread.cpp \
	threadlist.cpp \
	kernel.cpp \
	timer.cpp \
	timerlist.cpp \
	tracebuffer.cpp \
	kernelaware.cpp

# These files are built from source files in their respective 
# subdirectories under ./cpu/arch/variant/toolchain
PORT_CPP_SOURCE= \
	threadport.cpp \
	kernelswi.cpp \
	kerneltimer.cpp \
	kernelprofile.cpp


# Include the rest of the script that is actually used for building the 
# outputs
include $(ROOT_DIR)build.mak
