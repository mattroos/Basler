# Makefile for Basler pylon sample programs
.PHONY            : all clean

# The programs to build
SUBDIRS           := Grab \
                     Grab_CameraEvents \
                     Grab_ChunkImage \
                     Grab_MultiCast \
                     Grab_MultipleCameras \
                     Grab_Strategies \
                     Grab_UsingExposureEndEvent \
                     Grab_UsingGrabLoopThread \
                     Grab_UsingSequencer \
                     ParametrizeCamera_AutoFunctions \
                     ParametrizeCamera_Configurations \
                     ParametrizeCamera_GenericParameterAccess \
                     ParametrizeCamera_LoadAndSave \
                     ParametrizeCamera_LoadAndSaveSequenceSets \
                     ParametrizeCamera_LookupTable \
                     ParametrizeCamera_NativeParameterAccess \
                     ParametrizeCamera_Shading \
                     ParametrizeCamera_UserSets \
                     Utility_Image \
                     Utility_ImageFormatConverter \
                     Utility_ImageLoadAndSave


ifndef PYLON_ROOT
$(error PYLON_ROOT is not defined. Run 'source /opt/pylon3/bin/pylon-setup-env.sh /opt/pylon3' before calling make)
endif

GENICAM_ROOT	?= $(PYLON_ROOT)/genicam


# Rules for building
all:
	for subDirectory in $(SUBDIRS); do \
      $(MAKE) \
        -C $$subDirectory \
        GENICAM_ROOT="$(GENICAM_ROOT)" \
        PYLON_ROOT="$(PYLON_ROOT)" \
        all; \
      if test $$? -ne 0; then exit 1; fi; \
    done

clean:
	for subDirectory in $(SUBDIRS); do \
      $(MAKE) \
        -C $$subDirectory \
        GENICAM_ROOT="$(GENICAM_ROOT)" \
        PYLON_ROOT="$(PYLON_ROOT)" \
        clean; \
    done

.PHONY: install
install:
