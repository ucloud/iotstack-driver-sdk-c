include ../make.settings
CFLAGS  = -g -Wall -O2 -lpthread

UTILS_LIB   = libutils.a

INCLUDE     = -I$(PWD)

OBJS = ./utils_list.o 

all : $(UTILS_LIB) install

$(UTILS_LIB): $(OBJS)
	$(AR) cr $@ $(OBJS) 

$(OBJS):%o:%c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

install:
	mkdir -p $(PWD)/build/utils
	cp $(UTILS_LIB) $(PWD)/build/utils
	cp *.h $(PWD)/build/utils

clean:
	-$(RM) -r $(UTILS_LIB) $(OBJS)
