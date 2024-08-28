#---------------------------------------------------------------------
# NMAKE build script for the draw2pdf library.
#
# On NMAKE command line, use RELEASE=1 to select release build instead
# of debug build, and use WIN32=1 to select 32-bit build instead
# of 64-bit build.
#---------------------------------------------------------------------
# Applications that use the draw2pdf library will need to:
#  * Include "draw2pdf.h" in any of the app's cpp modules that want
#    to use draw2pdf's classes and functions.
#  * Link the app's executable to draw2pdf.lib.
#  * Link the app's executable to the ZLIB open source compression
#    library.
#---------------------------------------------------------------------

COMMONHDR= draw2pdf.h ascii85.h

!ifndef RELEASE
DIR_SUFFIX=
CPPFLAGS2=   -MTd -Od -Zi
!else
DIR_SUFFIX=r
CPPFLAGS2=   -MT -Ox
!endif

CPPFLAGS=   -nologo -c $(CPPFLAGS2) -Gs -EHsc -W4 -WX -DWIN32 -D_UNICODE -DUNICODE -I./zlib114
!ifdef WIN32
OBJDIR=     obj$(DIR_SUFFIX)
EXEDIR=     bin$(DIR_SUFFIX)
!else
OBJDIR=     obj64$(DIR_SUFFIX)
EXEDIR=     bin64$(DIR_SUFFIX)
!endif

!ifndef RELEASE
!ifndef WIN32
ZLIB=./zlib114/debug64/zlibd.lib
!else
ZLIB=./zlib114/debug/zlibd.lib
!endif
!else
!ifndef WIN32
ZLIB=./zlib114/release64/zlib.lib
!else
ZLIB=./zlib114/release/zlib.lib
!endif
!endif

#---------------------------------------------------------------------

.SUFFIXES:
.SUFFIXES:   .cpp

{.}.cpp{$(OBJDIR)}.obj:
   cl $(CPPFLAGS) -Fo$*.obj -Fd$(OBJDIR)\dlist.pdb $<

#---------------------------------------------------------------------

all:  $(OBJDIR) $(EXEDIR) $(EXEDIR)\draw2pdf.lib \
      $(EXEDIR)\pdftest.exe

$(OBJDIR):
   if not exist $(OBJDIR)/$(NULL) mkdir $(OBJDIR)

$(EXEDIR):
   if not exist $(EXEDIR)/$(NULL) mkdir $(EXEDIR)

$(EXEDIR)\draw2pdf.lib:   $(OBJDIR)\draw2pdf.obj $(ZLIB)
   lib /NOLOGO /OUT:$@ $**

$(EXEDIR)\pdftest.exe:  $(OBJDIR)\pdftest.obj $(EXEDIR)\draw2pdf.lib
   if exist link.tmp del link.tmp
   @echo /OUT:$@                                >> link.tmp
   @echo /DEBUG                                 >> link.tmp
   @echo /SUBSYSTEM:CONSOLE                     >> link.tmp
   @echo /IGNORE:4099                           >> link.tmp
   @echo $(OBJDIR)\pdftest.obj                  >> link.tmp
   @echo $(EXEDIR)\draw2pdf.lib                 >> link.tmp
   @echo user32.lib gdi32.lib comdlg32.lib      >> link.tmp
   @echo shell32.lib advapi32.lib winmm.lib     >> link.tmp
   @echo comctl32.lib kernel32.lib wininet.lib  >> link.tmp
   @echo opengl32.lib glu32.lib                 >> link.tmp
   link /NOLOGO @link.tmp
   if exist link.tmp del link.tmp

#---------------------------------------------------------------------

$(OBJDIR)\draw2pdf.obj:  draw2pdf.cpp $(COMMONHDR)
$(OBJDIR)\pdftest.obj:   pdftest.cpp $(COMMONHDR)

#---------------------------------------------------------------------
clean:
   echo Cleaning.
   if exist lib.tmp del lib.tmp
   if exist link.tmp del link.tmp
   if exist *.obj del *.obj
   if exist *.lst del *.lst
   if exist *.bak del *.bak
   if exist *.aps del *.aps
   if exist *.res del *.res
   if exist *.map del *.map
   if exist *.exp del *.exp
   if exist *.pdb del *.pdb
   if exist *.ilk del *.ilk
   if exist *.lib del *.lib
   if exist *.exp del *.exp
   if exist *.exe del *.exe
   if exist *.dll del *.dll
   if exist *.vc* del *.vc*
   if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
   if exist $(OBJDIR)\*.res del $(OBJDIR)\*.res
   if exist $(OBJDIR)\*.lib del $(OBJDIR)\*.lib
   if exist $(OBJDIR)\*.pdb del $(OBJDIR)\*.pdb
   if exist $(OBJDIR)\$(NULL) rmdir $(OBJDIR)
   if exist $(EXEDIR)\*.lib del $(EXEDIR)\*.lib
   if exist $(EXEDIR)\*.exe del $(EXEDIR)\*.exe
   if exist $(EXEDIR)\*.mac del $(EXEDIR)\*.mac
   if exist $(EXEDIR)\*.pdb del $(EXEDIR)\*.pdb
   if exist $(EXEDIR)\*.ini del $(EXEDIR)\*.ini
   if exist $(EXEDIR)\*.ilk del $(EXEDIR)\*.ilk
   if exist $(EXEDIR)\*.res del $(EXEDIR)\*.res
   if exist $(EXEDIR)\.vs\$(NULL) rmdir /s /q $(EXEDIR)\.vs
   if exist $(EXEDIR)\$(NULL) rmdir $(EXEDIR)
   if exist err del err
   if exist test.pdf del test.pdf
   echo Cleaned.

