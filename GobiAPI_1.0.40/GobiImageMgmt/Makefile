INCLUDES=-I ../Core -I ../Shared -lrt -L. -lCore
CXXFLAGS=-m32 -march=i386 -Wunused-variable
LDFLAGS=-melf_i386
DEBUGFLAGS=-g3 -D DEBUG
QMICOREFLAGS=-D IMG_SUPPORT
SOURCES=GobiImageMgmtExports.cpp \
        GobiImageMgmt.cpp \
        ../Shared/GobiQMICore.cpp \
        ../Shared/GobiQMICoreImg.cpp \
        ../Shared/GobiQDLCore.cpp \
        ../Shared/GobiMBNMgmt.cpp
OUTPUT=GobiImageMgmt.so

all: clean $(OUTPUT)

$(OUTPUT): $(SOURCES) libCore.a
	g++ $(CXXFLAGS) $(QMICOREFLAGS) -shared $(SOURCES) $(INCLUDES) -o $(OUTPUT) 

install: $(OUTPUT)
	mkdir -p /opt/Qualcomm/Gobi/
	cp -f $(OUTPUT) /opt/Qualcomm/Gobi/

clean:
	rm -f $(OUTPUT) \
         libCore.a \
         QMIDB.o \
         BitPacker.o \
         BitParser.o \
         Comm.o \
         CoreDatabase.o \
         CoreUtilities.o \
         CRC.o \
         DataPacker.o \
         DataParser.o \
         DB2NavTree.o \
         DB2TextFile.o \
         DB2Utilities.o \
         Event.o \
         HDLC.o \
         HDLCProtocolServer.o \
         MemoryMappedFile.o \
         ProtocolBuffer.o \
         ProtocolEntityNav.o \
         ProtocolLog.o \
         ProtocolNotification.o \
         ProtocolRequest.o \
         ProtocolServer.o \
         QDLBuffers.o \
         QDLProtocolServer.o \
         QMIBuffers.o \
         QMIProtocolServer.o \
         SharedBuffer.o

debug: 
	$(MAKE) $(MFLAGS) CXXFLAGS="$(CXXFLAGS) $(DEBUGFLAGS)" all

libCore.a: QMIDB.o \
           BitPacker.o \
           BitParser.o \
           Comm.o \
           CoreDatabase.o \
           CoreUtilities.o \
           CRC.o \
           DataPacker.o \
           DataParser.o \
           DB2NavTree.o \
           DB2TextFile.o \
           DB2Utilities.o \
           Event.o \
           HDLC.o \
           HDLCProtocolServer.o \
           MemoryMappedFile.o \
           ProtocolBuffer.o \
           ProtocolEntityNav.o \
           ProtocolLog.o \
           ProtocolNotification.o \
           ProtocolRequest.o \
           ProtocolServer.o \
           QDLBuffers.o \
           QDLProtocolServer.o \
           QMIBuffers.o \
           QMIProtocolServer.o \
           SharedBuffer.o
	ar rcs libCore.a \
          QMIDB.o \
          BitPacker.o \
          BitParser.o \
          Comm.o \
          CoreDatabase.o \
          CoreUtilities.o \
          CRC.o \
          DataPacker.o \
          DataParser.o \
          DB2NavTree.o \
          DB2TextFile.o \
          DB2Utilities.o \
          Event.o \
          HDLC.o \
          HDLCProtocolServer.o \
          MemoryMappedFile.o \
          ProtocolBuffer.o \
          ProtocolEntityNav.o \
          ProtocolLog.o \
          ProtocolNotification.o \
          ProtocolRequest.o \
          ProtocolServer.o \
          QDLBuffers.o \
          QDLProtocolServer.o \
          QMIBuffers.o \
          QMIProtocolServer.o \
          SharedBuffer.o

QMIDB.o: ../Database/QMI/Entity.txt \
         ../Database/QMI/EnumEntry.txt \
         ../Database/QMI/Enum.txt \
         ../Database/QMI/Field.txt \
         ../Database/QMI/Struct.txt
	cp ../Database/QMI ./ -rf
	ld $(LDFLAGS) -r -b binary -o QMIDB.o \
	              QMI/Entity.txt \
	              QMI/EnumEntry.txt \
	              QMI/Enum.txt \
	              QMI/Field.txt \
	              QMI/Struct.txt
	rm QMI -rf

BitPacker.o: ../Core/BitPacker.cpp
	g++ $(CXXFLAGS) -c ../Core/BitPacker.cpp

BitParser.o: ../Core/BitParser.cpp
	g++ $(CXXFLAGS) -c ../Core/BitParser.cpp

Comm.o: ../Core/Comm.cpp
	g++ $(CXXFLAGS) -c ../Core/Comm.cpp

CoreDatabase.o: ../Core/CoreDatabase.cpp
	g++ $(CXXFLAGS) -c ../Core/CoreDatabase.cpp

CoreUtilities.o: ../Core/CoreUtilities.cpp
	g++ $(CXXFLAGS) -c ../Core/CoreUtilities.cpp

CRC.o: ../Core/CRC.cpp
	g++ $(CXXFLAGS) -c ../Core/CRC.cpp

DataPacker.o: ../Core/DataPacker.cpp
	g++ $(CXXFLAGS) -c ../Core/DataPacker.cpp

DataParser.o: ../Core/DataParser.cpp
	g++ $(CXXFLAGS) -c ../Core/DataParser.cpp

DB2NavTree.o: ../Core/DB2NavTree.cpp
	g++ $(CXXFLAGS) -c ../Core/DB2NavTree.cpp

DB2TextFile.o: ../Core/DB2TextFile.cpp
	g++ $(CXXFLAGS) -c ../Core/DB2TextFile.cpp

DB2Utilities.o: ../Core/DB2Utilities.cpp
	g++ $(CXXFLAGS) -c ../Core/DB2Utilities.cpp

Event.o: ../Core/Event.cpp
	g++ $(CXXFLAGS) -c ../Core/Event.cpp

HDLC.o: ../Core/HDLC.cpp
	g++ $(CXXFLAGS) -c ../Core/HDLC.cpp

HDLCProtocolServer.o: ../Core/HDLCProtocolServer.cpp
	g++ $(CXXFLAGS) -c ../Core/HDLCProtocolServer.cpp

MemoryMappedFile.o: ../Core/MemoryMappedFile.cpp
	g++ $(CXXFLAGS) -c ../Core/MemoryMappedFile.cpp

ProtocolBuffer.o: ../Core/ProtocolBuffer.cpp
	g++ $(CXXFLAGS) -c ../Core/ProtocolBuffer.cpp

ProtocolEntityNav.o: ../Core/ProtocolEntityNav.cpp
	g++ $(CXXFLAGS) -c ../Core/ProtocolEntityNav.cpp

ProtocolLog.o: ../Core/ProtocolLog.cpp
	g++ $(CXXFLAGS) -c ../Core/ProtocolLog.cpp

ProtocolNotification.o: ../Core/ProtocolNotification.cpp
	g++ $(CXXFLAGS) -c ../Core/ProtocolNotification.cpp

ProtocolRequest.o: ../Core/ProtocolRequest.cpp
	g++ $(CXXFLAGS) -c ../Core/ProtocolRequest.cpp

ProtocolServer.o: ../Core/ProtocolServer.cpp
	g++ $(CXXFLAGS) -c ../Core/ProtocolServer.cpp

QDLBuffers.o: ../Core/QDLBuffers.cpp
	g++ $(CXXFLAGS) -c ../Core/QDLBuffers.cpp

QDLProtocolServer.o: ../Core/QDLProtocolServer.cpp
	g++ $(CXXFLAGS) -c ../Core/QDLProtocolServer.cpp

QMIBuffers.o: ../Core/QMIBuffers.cpp
	g++ $(CXXFLAGS) -c ../Core/QMIBuffers.cpp

QMIProtocolServer.o: ../Core/QMIProtocolServer.cpp
	g++ $(CXXFLAGS) -c ../Core/QMIProtocolServer.cpp

SharedBuffer.o: ../Core/SharedBuffer.cpp
	g++ $(CXXFLAGS) -c ../Core/SharedBuffer.cpp
	
