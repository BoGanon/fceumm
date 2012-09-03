include Makefile.objs

FCEU_FLAGS =-DFCEU_VERSION_NUMERIC=9813 -DFCEU_VERSION=\"0.98.13mm\" -DLSB_FIRST -DHAVE_ASPRINTF -DNO_MOVIE -DNETWORK -DPSS_STYLE=1

EE_BIN = fceumm.elf
EE_INCS = -I./ -I../ -I../../ -I../../boards -I../../fir -I../../input -I../../mappers \
	-I../../palettes -I../../utils -I./interface/include -I$(PS2SDK)/ports/include

EE_CFLAGS = -DSOUND_ON $(FCEU_FLAGS)
EE_CXXFLAGS = -DSOUND_ON $(FCEU_FLAGS) -fno-rtti -fno-exceptions

EE_OBJS = $(ZLIB_OBJS) $(BOARDS_OBJS) $(MAPPERS_OBJS) $(INPUT_OBJS) \
		$(CORE_OBJS) $(INTERFACE_OBJS) $(DRIVER_OBJS)

EE_LDFLAGS = -L$(PS2SDK)/ports/lib

PS2SDK_GFX_LIBS = -ldma -lpacket -lgraph -lfont -ldraw
PS2SDK_FILE_LIBS = -lfileXio
PS2SDK_PORT_LIBS = -lconfig -limage -lpng -lz
PS2SDK_SYS_LIBS = -lSMSCDVD -laudsrv -linputx -lmc -lmtap -lpadx -lpoweroff -lpatches -lmf -lm

EE_LIBS = $(PS2SDK_FILE_LIBS) $(PS2SDK_GFX_LIBS) $(PS2SDK_PORT_LIBS) $(PS2SDK_SYS_LIBS)

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

#needs to be compiled with standard ee-gcc
../../zlib/unzip.o: ../../zlib/unzip.c
	$(EE_CC) -D_EE -O2 -G0 -Wall $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%_tgz.s:
	bin2s $*.tgz $*_tgz.s $*_tgz

include $(PS2SDK)/samples/Makefile.pref
include Makefile.eeglobal_cpp
