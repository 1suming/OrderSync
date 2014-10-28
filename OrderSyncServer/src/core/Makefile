include ./Make.conf

COMPILE_LIB_HOME=../../lib

INC     = -I. -I $(CORE_HOME) 
OPT     = -pipe -fno-ident -fPIC -shared -z defs
LINK    = -lpthread

CFLAGS += $(OPT) -g -pg -Wall -D_GNU_SOURCE -funroll-loops -MMD -D_REENTRANT

ifeq ($(MEMCHECK),1)
CFLAGS += -DMEMCHECK
endif
CXXFLAGS := $(CFLAGS)

SRCS = 	MarkupSTL.cpp \
        net.cpp \
        log.cpp \
        config.cpp \
        memcheck.cpp \
        poller.cpp \
        timerlist.cpp \
		tm_task.cpp \

DYNAMIC_NAME = libcore.so
STATIC_NAME = libcore.a
DYNAMIC_LIB	= $(COMPILE_LIB_HOME)/$(DYNAMIC_NAME)
STATIC_LIB = $(COMPILE_LIB_HOME)/$(STATIC_NAME)

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(SRCS:.cpp=.o)
	$(CXX) -pg -o $@ $^ $(CXXFLAGS) $(INC) $(LINK)
	ar -r $(STATIC_LIB) *.o
	@-rm -f $(DYNAMIC_LIB)
%.o: %.c Makefile
	$(CC) $(CFLAGS) $(INC) -c -pg -o $@ $<
	@-mv -f $*.d .dep.$@
%.o: %.cc Makefile
	$(CXX) $(CXXFLAGS) $(INC) -c -pg -o $@ $<
	@-mv -f $*.d .dep.$@
%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $(INC) -c -pg -o $@ $<
	@-mv -f $*.d .dep.$@
clean:
	rm -f *.o .po *.so *.d .dep.* $(DYNAMIC_LIB) $(COMPILE_LIB_HOME)/libcore.so
-include /dev/null $(wildcard .dep.*) 
