# GNU Make Makefile for resonanz 
# (C) Copyright Tomas Ukkonen
############################################################

CC = gcc
CXX = g++
MAKE = make
MKDIR = mkdir
AR = ar
CD = cd
RM = rm -f
MV = mv
CP = cp

CFLAGS = -std=c++11 -O2 -g `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf` `pkg-config --cflags SDL2_image` `pkg-config --cflags SDL2_gfx` `pkg-config --cflags SDL2_mixer` `pkg-config --cflags dinrhiw` -I.
CXXFLAGS = -std=c++11 -O2 -g `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf` `pkg-config --cflags SDL2_image` `pkg-config --cflags SDL2_gfx` `pkg-config --cflags SDL2_mixer` `pkg-config --cflags dinrhiw` -I.

OBJECTS = FMSoundSynthesis.o SDLSoundSynthesis.o ResonanzShow.o EmotivInsightStub.o spectral_analysis.o main.o
SOURCES = FMSoundSynthesis.cpp SDLSoundSynthesis.cpp ResonanzShow.cpp EmotivInsightStub.cpp spectral_analysis.cpp main.cpp tst/spectral_test.cpp

# MuseOSCSampler.o

TARGET = resonanz

LIBS = `sdl2-config --libs` `pkg-config --libs SDL2_ttf` `pkg-config --libs SDL2_image` `pkg-config --libs SDL2_gfx` `pkg-config --libs SDL2_mixer` `pkg-config --libs dinrhiw` -lfftw3 -lm

SPECTRAL_TEST_OBJECTS=spectral_analysis.o tst/spectral_test.o
SPECTRAL_TEST_TARGET=spectral_test

MAXIMPACT_CFLAGS=-O -g `sdl2-config --cflags` `pkg-config SDL2_image --cflags` `pkg-config SDL2_gfx --cflags` `aalib-config --cflags` `pkg-config dinrhiw --cflags`

MAXIMPACT_CXXFLAGS=$(CFLAGS)

MAXIMPACT_LIBS=`sdl2-config --libs` `pkg-config SDL2_image --libs` `pkg-config SDL2_gfx --libs` `aalib-config --libs` `pkg-config dinrhiw --libs` -lncurses

MAXIMPACT_OBJECTS=maximpact.o
MAXIMPACT_TARGET=maximpact

############################################################

all: $(OBJECTS) spectral_test
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

spectral_test: $(SPECTRAL_TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(SPECTRAL_TEST_TARGET) $(SPECTRAL_TEST_OBJECTS) $(LIBS)

maximpact: $(MAXIMPACT_OBJECTS)
	$(CXX) $(MAXIMPACT_CXXFLAGS) -o $(MAXIMPACT_TARGET) $(MAXIMPACT_OBJECTS) $(MAXIMPACT_LIBS)

clean:
	$(RM) $(OBJECTS)
	$(RM) $(TARGET)
	$(RM) *~

depend:
	$(CXX) $(CXXFLAGS) -MM $(SOURCES) > Makefile.depend

############################################################

include Makefile.depend
