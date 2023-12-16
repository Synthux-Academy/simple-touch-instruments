# Project Name
TARGET = simple-arpeggiator-touch

# Sources
CPP_SOURCES = simple-arpeggiator-touch.cpp

# Library Locations
#LIBDAISY_DIR := ../../../EmHonaiseDaisySP/libDaisy/
#DAISYSP_DIR := ../../../EmHonaiseDaisySP/DaisySP/

ifndef LIBDAISY_DIR
$(error LIBDAISY_DIR is not set)
endif

ifndef DAISYSP_DIR
$(error DAISYSP_DIR is not set)
endif


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
