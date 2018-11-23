SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
DEPENDS = $(OBJECTS:.o=.d)

CFLAGS  = -Wall -Wextra -pedantic -O2 -s
LDFLAGS = -lpcre2-posix -ltesseract -ldjvulibre -l:libmupdf.a -l:libmupdf-third.a \
          -ljbig2dec -ljpeg -lopenjp2 -lfreetype -lharfbuzz -lm -lz

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

isbn_extract: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJECTS) isbn_extract
