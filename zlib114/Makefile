#--------------------------------------------------------------------
# NOTE: This is the Win32 Makefile for the ZLIB compression library,
# with various hacks to make it able to build both debug and release
# versions from the same source.  Comments below were from the
# original version and some may no longer apply to this hacked
# version.
#
# Use "NMAKE -fMakefile" to build debug version.
# Use "NMAKE -fMakefile SHIP=1" to build release version.
#
# Add WIN64=1 to command line to build 64-bit version instead of
# 32-bit version.
#--------------------------------------------------------------------

# Makefile for zlib
# Microsoft 32-bit Visual C++ 8.0 or later (may work on earlier versions)

# To use, do "nmake /f makefile.w32"

# If you wish to reduce the memory requirements (default 256K for big
# objects plus a few K), you can add to CFLAGS below: 
#   -DMAX_MEM_LEVEL=7 -DMAX_WBITS=14
# See zconf.h for details about the memory requirements.

!ifdef WIN64
DIRSUF=64
!message Building 64-bit ZLIB
!else
DIRSUF=
!message Building 32-bit ZLIB
!endif

# ------------- Microsoft Visual C++ 8.0 and later -------------
!ifdef SHIP
!message Building release ZLIB
#CFLAGSCOMMON = -c -nologo -DSTRICT -DWIN32 -D_WIN32 -D_CRT_SECURE_NO_DEPRECATE -Gs -W3 -WX -EHsc -GR- -Ox -MD -Zi
CFLAGSCOMMON = -c -nologo -DSTRICT -DWIN32 -D_WIN32 -D_CRT_SECURE_NO_DEPRECATE -Gs -W3 -WX -EHsc -GR- -Ox -MT -Zi
LDFLAGS=/NOLOGO
OBJDIR=obj$(DIRSUF)
EXEDIR=release$(DIRSUF)
LIBNAME=zlib
!else
!message Building debug ZLIB
#CFLAGSCOMMON = -c -nologo -DSTRICT -DWIN32 -D_WIN32 -D_CRT_SECURE_NO_DEPRECATE -Gs -W3 -WX -EHsc -GR- -Od -MDd -Zi -RTC1 -Fdzlib.pdb
CFLAGSCOMMON = -c -nologo -DSTRICT -DWIN32 -D_WIN32 -D_CRT_SECURE_NO_DEPRECATE -Gs -W3 -WX -EHsc -GR- -Od -MTd -Zi -RTC1 -Fdzlib.pdb
LDFLAGS=/NOLOGO /DEBUG
OBJDIR=objd$(DIRSUF)
EXEDIR=debug$(DIRSUF)
LIBNAME=zlibd
!endif
CFLAGS= $(CFLAGSCOMMON) -I.
CPP=cl
LD=link

# variables
OBJS1 = &adler32.obj &compress.obj &crc32.obj &gzio.obj \
        &uncompr.obj &deflate.obj &trees.obj
OBJS2 = &zutil.obj &inflate.obj &infblock.obj &inftrees.obj \
        &infcodes.obj &infutil.obj &inffast.obj

!ifdef SHIP
!ifdef WIN64
OBJ1 = $(OBJS1:&=obj64\)
OBJ2 = $(OBJS2:&=obj64\)
!else
OBJ1 = $(OBJS1:&=obj\)
OBJ2 = $(OBJS2:&=obj\)
!endif
!else
!ifdef WIN64
OBJ1 = $(OBJS1:&=objd64\)
OBJ2 = $(OBJS2:&=objd64\)
!else
OBJ1 = $(OBJS1:&=objd\)
OBJ2 = $(OBJS2:&=objd\)
!endif
!endif

.SUFFIXES:
.SUFFIXES: .c

{.}.c{$(OBJDIR)}.obj:
   $(CPP) $(CFLAGS) -Fo$@ $(<F)

all:  $(OBJDIR) $(EXEDIR) \
   $(EXEDIR)\$(LIBNAME).lib \
   $(EXEDIR)\example.exe \
   $(EXEDIR)\minigzip.exe

$(OBJDIR):
   if not exist $(OBJDIR)/$(NULL) mkdir $(OBJDIR)

$(EXEDIR):
   if not exist $(EXEDIR)/$(NULL) mkdir $(EXEDIR)

$(OBJDIR)\adler32.obj: adler32.c zlib.h zconf.h
$(OBJDIR)\compress.obj: compress.c zlib.h zconf.h
$(OBJDIR)\crc32.obj: crc32.c zlib.h zconf.h
$(OBJDIR)\deflate.obj: deflate.c deflate.h zutil.h zlib.h zconf.h
$(OBJDIR)\gzio.obj: gzio.c zutil.h zlib.h zconf.h
$(OBJDIR)\infblock.obj: infblock.c zutil.h zlib.h zconf.h infblock.h inftrees.h infcodes.h infutil.h
$(OBJDIR)\infcodes.obj: infcodes.c zutil.h zlib.h zconf.h inftrees.h infutil.h infcodes.h inffast.h
$(OBJDIR)\inflate.obj: inflate.c zutil.h zlib.h zconf.h infblock.h
$(OBJDIR)\inftrees.obj: inftrees.c zutil.h zlib.h zconf.h inftrees.h
$(OBJDIR)\infutil.obj: infutil.c zutil.h zlib.h zconf.h inftrees.h infutil.h
$(OBJDIR)\inffast.obj: inffast.c zutil.h zlib.h zconf.h inftrees.h infutil.h inffast.h
$(OBJDIR)\trees.obj: trees.c deflate.h zutil.h zlib.h zconf.h
$(OBJDIR)\uncompr.obj: uncompr.c zlib.h zconf.h
$(OBJDIR)\zutil.obj: zutil.c zutil.h zlib.h zconf.h
$(OBJDIR)\example.obj: example.c zlib.h zconf.h
$(OBJDIR)\minigzip.obj: minigzip.c zlib.h zconf.h

$(EXEDIR)\$(LIBNAME).lib: $(OBJ1) $(OBJ2)
  if exist $@ del $@
  lib /OUT:$@ $(OBJ1) $(OBJ2)

$(EXEDIR)\example.exe: $(OBJDIR)\example.obj $(EXEDIR)\$(LIBNAME).lib
  $(LD) /PDB:$*.pdb $(LDFLAGS) $** /OUT:$@ /SUBSYSTEM:CONSOLE

$(EXEDIR)\minigzip.exe: $(OBJDIR)\minigzip.obj $(EXEDIR)\$(LIBNAME).lib
  $(LD) /PDB:$*.pdb $(LDFLAGS) $** /OUT:$@ /SUBSYSTEM:CONSOLE

test: $(EXEDIR)\example.exe $(EXEDIR)\minigzip.exe
  $(EXEDIR)\example
  echo hello world | $(EXEDIR)\minigzip | $(EXEDIR)\minigzip -d 

clean2:
   if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
   if exist $(OBJDIR)\*.res del $(OBJDIR)\*.res
   if exist $(OBJDIR)\*.pdb del $(OBJDIR)\*.pdb
   if exist $(OBJDIR)\*.idb del $(OBJDIR)\*.idb
   if exist $(OBJDIR)\BuildLog.htm del $(OBJDIR)\BuildLog.htm
   if exist $(EXEDIR)\*.exe del $(EXEDIR)\*.exe
   if exist $(EXEDIR)\*.exe.manifest del $(EXEDIR)\*.exe.manifest
   if exist $(EXEDIR)\*.lib del $(EXEDIR)\*.lib
   if exist $(EXEDIR)\*.pdb del $(EXEDIR)\*.pdb
   if exist $(EXEDIR)\*.ilk del $(EXEDIR)\*.ilk
   if exist $(EXEDIR)\*.ini del $(EXEDIR)\*.ini
   if exist *.obj del *.obj
   if exist *.exe del *.exe
   if exist *.lib del *.lib
   if exist *.pdb del *.pdb
   if exist *.bak del *.bak
   if exist foo.gz del foo.gz
   if exist $(OBJDIR)/$(NULL) rmdir $(OBJDIR)
   if exist $(EXEDIR)/$(NULL) rmdir $(EXEDIR)

clean:
   nmake clean2
   nmake clean2 "SHIP=1"

