# Makefile for Basler Pylon sample program
.PHONY			: all clean

# The program to build
NAME			:= ParametrizeCamera_NativeParameterAccess

# Installation directories for GenICam and Pylon
PYLON_ROOT		?= /opt/pylon4
GENICAM_ROOT	?= $(PYLON_ROOT)/genicam

# Build tools and flags
CXX				?= g++
LD				:= $(CXX)
CPPFLAGS		:= -I$(GENICAM_ROOT)/library/CPP/include \
				   -I$(PYLON_ROOT)/include 
CXXFLAGS		:= #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS			:= -L$(PYLON_ROOT)/lib64 \
				   -L$(GENICAM_ROOT)/bin/Linux64_x64 \
				   -L$(GENICAM_ROOT)/bin/Linux64_x64/GenApi/Generic \
				   -Wl,-E
LIBS			:= -lpylonbase -lGenApi_gcc40_v2_3 -lGCBase_gcc40_v2_3 -lLog_gcc40_v2_3 -lMathParser_gcc40_v2_3 -lXerces-C_gcc40_v2_7_1 -llog4cpp_gcc40_v2_3

# Rules for building
all				: $(NAME)_GigE $(NAME)_Usb

%: %.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(NAME)_GigE.o: $(NAME).cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DUSE_GIGE -c -o $@ $<
	
$(NAME)_Usb.o: $(NAME).cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DUSE_USB -c -o $@ $<

clean			:
	$(RM) $(NAME)_GigE.o $(NAME)_Usb.o $(NAME)_GigE $(NAME)_Usb
