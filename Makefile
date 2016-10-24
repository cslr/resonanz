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

CFLAGS = -fPIC -std=c++1y -O3 -g -fopenmp `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf` `pkg-config --cflags SDL2_image` `pkg-config --cflags SDL2_gfx` `pkg-config --cflags SDL2_mixer` `pkg-config --cflags dinrhiw` -I. -Ioscpkt -I/usr/lib/jvm/java-7-openjdk-amd64/include/
CXXFLAGS = -fPIC -std=c++1y -O3 -g -fopenmp `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf` `pkg-config --cflags SDL2_image` `pkg-config --cflags SDL2_gfx` `pkg-config --cflags SDL2_mixer` `pkg-config --cflags dinrhiw` -I. -Ioscpkt -I/usr/lib/jvm/java-7-openjdk-amd64/include/

OBJECTS = ResonanzEngine.o MuseOSC.o NMCFile.o NoEEGDevice.o RandomEEG.o SDLTheora.o Log.o SDLSoundSynthesis.o FMSoundSynthesis.o hermitecurve.o

SOURCES = main.cpp ResonanzEngine.cpp MuseOSC.cpp NMCFile.cpp NoEEGDevice.cpp RandomEEG.cpp SDLTheora.cpp jni/fi_iki_nop_neuromancer_ResonanzEngine.cpp Log.cpp hermitecurve.cpp



# MuseOSCSampler.o

TARGET = resonanz

LIBS = `sdl2-config --libs` `pkg-config --libs SDL2_ttf` `pkg-config --libs SDL2_image` `pkg-config --libs SDL2_gfx` `pkg-config --libs SDL2_mixer` `pkg-config --libs dinrhiw` -fopenmp -ltheoraenc -ltheoradec -logg

RESONANZ_OBJECTS=$(OBJECTS) main.o

JNILIB_OBJECTS=$(OBJECTS) jni/fi_iki_nop_neuromancer_ResonanzEngine.o

SPECTRAL_TEST_OBJECTS=spectral_analysis.o tst/spectral_test.o
SPECTRAL_TEST_TARGET=spectral_test

MAXIMPACT_CFLAGS=-O -g `sdl2-config --cflags` `pkg-config SDL2_image --cflags` `pkg-config SDL2_gfx --cflags` `aalib-config --cflags` `pkg-config dinrhiw --cflags`

MAXIMPACT_CXXFLAGS=$(CFLAGS)

MAXIMPACT_LIBS=`sdl2-config --libs` `pkg-config SDL2_image --libs` `pkg-config SDL2_gfx --libs` `aalib-config --libs` `pkg-config dinrhiw --libs` -lncurses

MAXIMPACT_OBJECTS=maximpact.o
MAXIMPACT_TARGET=maximpact

SOUND_LIBS=`sdl2-config --libs`
SOUND_TEST_TARGET=fmsound
SOUND_TEST_OBJECTS=sound_test.o SDLSoundSynthesis.o FMSoundSynthesis.o


############################################################

all: $(OBJECTS) resonanz 

# jnilib spectral_test

resonanz: $(RESONANZ_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(RESONANZ_OBJECTS) $(LIBS)

jnilib: $(JNILIB_OBJECTS)
# TODO build shared library for JNI class to link to

sound: $(SOUND_TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(SOUND_TEST_TARGET) $(SOUND_TEST_OBJECTS) $(SOUND_LIBS)

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
