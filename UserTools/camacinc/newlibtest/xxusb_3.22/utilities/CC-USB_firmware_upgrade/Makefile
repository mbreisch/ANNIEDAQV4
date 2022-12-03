# basic makefile

.LIBPATTERNS ="'lib%.so lib%.a' "

CXX   = g++
FC		= g77
F90		= f90
RM		= rm -f
CC		= gcc-3.3

LD            = gcc-3.3
LDFLAGS       = -O
SOFLAGS       = -shared


CXXLIBDIRS	= -L./../../libs/
CXXLIBS		= -lxx_usb
INCLUDEDIRS 	= -I./../../include/

CXXFLAGS      	= -O -Wall -fPIC -g $(INCLUDEDIRS)

CFLAGS		+= $(INCLUDEDIRS) $(SOFLAG)
CFLAGS		+= $(CXXLIBDIRS) $(CXXLIBS)
LDFLAGS		= $(CXXFLAGS)

OBJ = flashccusb

main: 
	$(CXX) $(CXXFLAGS) \
	$(CXXLIBS) $(CXXLIBDIRS) -o $(OBJ) ccusb_flash.cpp
	@echo "Done"



clean:
	$(RM) $(OBJ) 

