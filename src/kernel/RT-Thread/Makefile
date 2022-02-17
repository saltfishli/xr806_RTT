#
# Rules for building library
#

# ----------------------------------------------------------------------------
# common rules
# ----------------------------------------------------------------------------
ROOT_PATH := ../../..

include $(ROOT_PATH)/gcc.mk

# ----------------------------------------------------------------------------
# library and objects
# ----------------------------------------------------------------------------
LIBS := librtthread.a

DIRS := ./

DIRS += src
DIRS += bsp
DIRS += components/finsh
DIRS += libcpu/arm/cortex-m33
# RTOS_DIRS := src 
# RTOS_DIRS += bsp 
# RTOS_DIRS += components/finsh 
# RTOS_DIRS += libcpu/arm/cortex-m33

SRCS := $(sort $(basename $(foreach dir,$(DIRS),$(wildcard $(dir)/*.[csS]))))

OBJS := $(addsuffix .o,$(SRCS))

ASFLAGS += -Wa,-mimplicit-it=thumb

# library make rules
include $(LIB_MAKE_RULES)
