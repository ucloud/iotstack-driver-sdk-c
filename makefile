include make.settings
# default compile output
all :
	$(MAKE) edge_sdk_c
	$(MAKE) -C samples -f samples.mk

# linkedge device access sdk
edge_sdk_c :
	$(MAKE) -C utils -f utils.mk
	$(MAKE) -C deps -f deps.mk
	$(MAKE) -C edge -f edge.mk

# clean tempory compile resource
clean:
	$(MAKE) -C utils -f utils.mk clean
	$(MAKE) -C deps -f deps.mk clean
	$(MAKE) -C edge -f edge.mk clean
	$(MAKE) -C samples -f samples.mk clean
	-$(RM) -r ./build
	-$(RM) -r ./deps/cJSON-1.7.7/
	-$(RM) -r ./deps/nats.c-master/

.PHONY: utils deps edge samples 
