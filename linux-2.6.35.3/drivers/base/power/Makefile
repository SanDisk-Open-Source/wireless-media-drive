
ifdef CONFIG_MACH_MX50_NIMBUS_TI_WIFI_PM_PATCH
obj-$(CONFIG_PM)	+= sysfs_nimbus.o
obj-$(CONFIG_PM_SLEEP)	+= main.o wakeup.o
obj-$(CONFIG_PM_RUNTIME)	+= runtime_nimbus.o
else
obj-$(CONFIG_PM)	+= sysfs.o
obj-$(CONFIG_PM_SLEEP)	+= main.o
obj-$(CONFIG_PM_RUNTIME)	+= runtime.o
endif

obj-$(CONFIG_PM_OPS)	+= generic_ops.o
obj-$(CONFIG_PM_TRACE_RTC)	+= trace.o
ifdef CONFIG_MACH_MX50_NIMBUS_TI_WIFI_PM_PATCH
obj-$(CONFIG_PM_OPP)	+= opp.o
endif

ccflags-$(CONFIG_DEBUG_DRIVER) := -DDEBUG
ccflags-$(CONFIG_PM_VERBOSE)   += -DDEBUG
