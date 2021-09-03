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


all:      $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i all; sleep 2; done
	@echo


clean:    $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean; sleep 1; done
	@echo


cleanall: $(SUBDIRS)
	for i in $(SUBDIRS); do $(MAKE) -C $$i cleanall; sleep 1; done
	@echo
