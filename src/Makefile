include ../Make.include

#CFLAGS += -DDEBUG

HEADERS=$(wildcard *.h)
HEADERS_GCH=$(HEADERS:.h=.h.gch)

all:	$(PROGRAM)

$(PROGRAM):	$(OBJECTS)
	$(CXX) $(LDFLAGS) -o $(PROGRAM) $(OBJECTS) $(LIBS)
#	strip $(PROGRAM)

$(OBJECTS): $(HEADERS)

.SUFFIXES:
.SUFFIXES: .cc .rc .o

#$(HEADERS_GCH): %.h.gch: %.h
#	$(CXX) -c -x c++-header $(CFLAGS) -o $@ $<

.rc.o:
	windres -o $@ $<

clean:
	rm -f *.o $(PROGRAM)
