CXXFLAGS = -g -O2 -Wall -DDEBUG -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lpng16 -lz
OBJECTS = Image.o PNGImage.o Resampler.o resample.o

resample: ${OBJECTS} 
	  g++ ${CXXFLAGS} -o resample ${OBJECTS} ${LDFLAGS} ${LIBS}

clean:	resample.o
	rm resample.exe ${OBJECTS}
