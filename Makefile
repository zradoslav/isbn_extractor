SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:%.cpp=%.o)
DEPENDS = $(OBJECTS:%.o=%.d)

CXXFLAGS  = -Wall -Wextra -pedantic -O2 -s -fno-rtti -fno-exceptions
LDFLAGS = -ltesseract -ldjvulibre -l:libmupdf.a -l:libmupdf-third.a \
          -ljbig2dec -ljpeg -lopenjp2 -lfreetype -lharfbuzz -lm -lz

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

isbn_extract: $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJECTS) isbn_extract
