CPP = g++
CPPFLAGS=-g
INCLUDEDIR=../../../liblua/.lua50/include
LIBDIR=../../../liblua/.lua50/lib
LIBS=-llua -llualib
#INCLUDEDIR=../../../liblua/.lua52/install/include
#LIBDIR=../../../liblua/.lua52/install/lib
#LIBS=../../../liblua/.lua52/src/liblua.a
all: qc cc

qc: qc.cc qc.o crc32.o
	$(CPP) -g -o ./qc -I$(INCLUDEDIR) -L$(LIBDIR) qc.o crc32.o $(LIBS)

crc32.o: crc32.cc
	$(CPP) $(CPPFLAGS) -c $< -I$(INCLUDEDIR) -L$(LIBDIR)

qc.o: qc.cc
	$(CPP) $(CPPFLAGS) -c $< -I$(INCLUDEDIR) -L$(LIBDIR)

create_conversion.o: create_conversion.cc
	$(CPP) $(CPPFLAGS) -c $< -I$(INCLUDEDIR) -L$(LIBDIR)

cc: crc32.o create_conversion.o
	$(CPP) -g -o ./cc -I$(INCLUDEDIR) -L$(LIBDIR) create_conversion.o crc32.o $(LIBS)

clean:
	rm -f qc cc qc.o crc32.o cc.o create_conversion.o
