SRCS=WinSystemX11.cpp \
     WinSystemX11GLContext.cpp \
     WinSystemX11GLESContext.cpp \
     GLContextEGL.cpp \
     GLContextGLX.cpp \
     XRandR.cpp \
     Xsel.cpp
     
LIB=windowing_X11.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
