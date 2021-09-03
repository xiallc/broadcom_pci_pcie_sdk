SUBDIRS	= \
	  Linux/PlxApi                   \
	  Linux/Samples/ApiTest          \
	  Linux/Samples/DSlave           \
	  Linux/Samples/DSlave_BypassApi \
	  Linux/Samples/LocalToPciInt    \
	  Linux/Samples/NT_DmaTest       \
	  Linux/Samples/NT_LinkTest      \
	  Linux/Samples/NT_Sample        \
	  Linux/Samples/PerfMonitor      \
	  Linux/Samples/PlxCm            \
	  Linux/Samples/PlxDma           \
	  Linux/Samples/PlxDmaPerf       \
	  Linux/Samples/PlxDmaSglNoApi   \
	  Linux/Samples/PlxEep           \
	  Linux/Samples/PlxNotification


all:      $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i all; sleep 2; done
	@echo


clean:    $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean; sleep 1; done
	@echo


cleanall: $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i cleanall; sleep 1; done
	@echo
