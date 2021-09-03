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


# Targets
all: $(SUBDIRS)
	@clear
	@for i in $(SUBDIRS); \
	 do \
	    echo '   ------------------'; \
	    $(MAKE) -C $$i PLX_NO_CLEAR_SCREEN=1 --no-print-directory; \
	    sleep 1; \
	 done
	@echo


clean: $(SUBDIRS)
	@clear
	@for i in $(SUBDIRS); \
	 do \
	    echo '   ------------------'; \
	    $(MAKE) -C $$i clean --no-print-directory; \
	    sleep 1; \
	 done
	@echo


cleanobj: $(SUBDIRS)
	@clear
	@for i in $(SUBDIRS); \
	 do \
	    echo '   ------------------'; \
	    $(MAKE) -C $$i cleanobj --no-print-directory; \
	    sleep 1; \
	 done
	@echo
