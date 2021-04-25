include ../make.settings
# 解压nats组件源码并完成编译和安装

LIB_NATS_TAR    = $(PWD)/deps/nats.c-2.1.0
LIB_CJSON_TAR   = $(PWD)/deps/cJSON-1.7.7
LIB_NATS_DIR    = $(PWD)/build/nats
LIB_CJSON_DIR   = $(PWD)/build/cjson

all :
	tar xvfz $(LIB_NATS_TAR).tar.gz
	mkdir -p $(LIB_NATS_DIR)
	cd $(LIB_NATS_TAR) && cmake . -DCMAKE_TOOLCHAIN_FILE=../deps.cmake -DNATS_BUILD_WITH_TLS=OFF -DNATS_BUILD_STREAMING=OFF -DNATS_BUILD_NO_SPIN=ON && $(MAKE)
	cp $(LIB_NATS_TAR)/src/libnats_static.a $(LIB_NATS_DIR)
	cp $(LIB_NATS_TAR)/src/nats.h $(LIB_NATS_DIR)
	cp $(LIB_NATS_TAR)/src/status.h $(LIB_NATS_DIR)
	cp $(LIB_NATS_TAR)/src/version.h $(LIB_NATS_DIR)
	mkdir -p $(LIB_CJSON_DIR)
	tar xvfz $(LIB_CJSON_TAR).tar.gz
	cd $(LIB_CJSON_TAR) && $(MAKE) clean && $(MAKE) CC=$(CC) AR=$(AR) static
	cp $(LIB_CJSON_TAR)/libcjson.a $(LIB_CJSON_DIR)
	cp $(LIB_CJSON_TAR)/cJSON.h $(LIB_CJSON_DIR)

clean :	
	rm -rf $(LIB_NATS_TAR)
	rm -rf $(LIB_CJSON_TAR)
