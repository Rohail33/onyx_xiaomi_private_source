# MIST Module Makefile
# CompilesMIST (Memory Intensive Scheduler Tuning) for Linux kernel

# For standalone module build


# Module parameters
ccflags-y += -DMIST_VERSION=\"3.5.0\"

# Debug flags
ifeq ($(CONFIG_MIST_DEBUG),y)
ccflags-y += -DMIST_DEBUG
endif

ifeq ($(CONFIG_MIST_FTRACE_DEBUG),y)
ccflags-y += -DMIST_FTRACE_DEBUG
endif

# Feature flags
ifeq ($(CONFIG_MIST_LATENCY_MONITOR),y)
ccflags-y += -DMIST_LATENCY_MONITOR
endif

ifeq ($(CONFIG_MIST_BANDWIDTH_MONITOR),y)
ccflags-y += -DMIST_BANDWIDTH_MONITOR
endif

ifeq ($(CONFIG_MIST_GPU_FLT),y)
ccflags-y += -DMIST_GPU_FLT
endif

ifeq ($(CONFIG_MIST_FREQUENCY_SCALING),y)
ccflags-y += -DMIST_FREQUENCY_SCALING
endif

ifeq ($(CONFIG_MIST_DDR_OPTIMIZATION),y)
ccflags-y += -DMIST_DDR_OPTIMIZATION
endif

ifeq ($(CONFIG_MIST_FRAME_TRACKING),y)
ccflags-y += -DMIST_FRAME_TRACKING
endif

# Parameters
MIST_MAX_DDR_IB ?= 4000
MIST_SPM_THRESHOLD ?= 75
MIST_IPS_THRESHOLD ?= 2000

ccflags-y += -DMIST_MAX_DDR_IB=$(MIST_MAX_DDR_IB)
ccflags-y += -DMIST_SPM_THRESHOLD=$(MIST_SPM_THRESHOLD)
ccflags-y += -DMIST_IPS_THRESHOLD=$(MIST_IPS_THRESHOLD)

obj-$(CONFIG_MIST_SCHEDULER) += mist.o
