# Set the folders to build
SUBDIRS	= \
	  PlxApi                   \
	  Samples/ApiTest          \
	  Samples/DSlave           \
	  Samples/DSlave_BypassApi \
	  Samples/LocalToPciInt    \
	  Samples/NT_DmaTest       \
	  Samples/NT_LinkTest      \
	  Samples/NT_Sample        \
	  Samples/PerfMonitor      \
	  Samples/PlxCm            \
	  Samples/PlxDma           \
	  Samples/PlxDmaPerf       \
	  Samples/PlxDmaSglNoApi   \
	  Samples/PlxEep           \
	  Samples/PlxNotification


# Options for make
MAKEFLAGS += --no-print-directory


# Targets
all: $(SUBDIRS)
	@clear
	@for i in $(SUBDIRS); \
	 do \
	    echo '   ------------------'; \
	    $(MAKE) -C $$i PLX_NO_CLEAR_SCREEN=1; \
	    sleep 1; \
	 done
	@echo


# Parameter shortcuts
c: clean
o: cleanobj


# Clean all files
clean: $(SUBDIRS)
	@clear
	@for i in $(SUBDIRS); \
	 do \
	    echo '   ------------------'; \
	    $(MAKE) -C $$i clean; \
	 done
	@echo


# Clean only object files
cleanobj: $(SUBDIRS)
	@clear
	@for i in $(SUBDIRS); \
	 do \
	    echo '   ------------------'; \
	    $(MAKE) -C $$i cleanobj; \
	 done
	@echo
