# basic makefile

.LIBPATTERNS ="'lib%.so lib%.a' "

CXX   = gcc -fPIC
FC		= g77
F90		= f90
RM		= rm -f
CC		= gcc-3.3

LD            = gcc-3.3
LDFLAGS       = -O
SOFLAGS       = -shared


CXXLIBDIRS	= -L /usr/lib64/
CXXLIBS		= -lusb
INCLUDEDIRS 	= -I./../include/

CXXFLAGS      	= -Wall -shared $(INCLUDEDIRS)

CFLAGS		+= $(INCLUDEDIRS) $(SOFLAG)
CFLAGS		+= $(CXXLIBDIRS) $(CXXLIBS)
LDFLAGS		= $(CXXFLAGS)

OBJ = libxx_usb.so

library: 
	$(CXX) $(CXXFLAGS) \
	$(CXXLIBDIRS) $(CXXLIBS) -o $(OBJ) libxxusb.c
	@echo "Done"


install:
	@cp $(OBJ) /usr/lib/


clean:
	$(RM) $(OBJ) 

