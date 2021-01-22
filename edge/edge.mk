include ../make.settings
CFLAGS  = -g -Wall -O2 -D_GNU_SOURCE -std=c99

EDGE_LIB     = libedge.a

INCLUDE_PATH =  -I$(PWD)
INCLUDE      = $(INCLUDE_PATH)/build/cjson $(INCLUDE_PATH)/build/nats $(INCLUDE_PATH)/build/utils 

OBJS = ./common.o \
       ./client.o \
       ./edge.o \

all : $(EDGE_LIB) install

$(EDGE_LIB): $(OBJS)
	$(AR) cr $@ $(OBJS)

$(OBJS):%o:%c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

install:
	mkdir -p $(PWD)/build/edge
	cp $(EDGE_LIB) $(PWD)/build/edge
	cp *.h $(PWD)/build/edge
clean:
	-$(RM) -r $(EDGE_LIB) $(OBJS)
