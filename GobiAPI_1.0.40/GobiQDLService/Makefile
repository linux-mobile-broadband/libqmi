INCLUDES=-I ../Shared -I ../Core -lrt -L. -lCore
CXXFLAGS=-m32 -march=i386 -Wunused-variable
DEBUGFLAGS=-g3 -D DEBUG
SOURCES=Main.cpp \
        ../Shared/GobiQDLCore.cpp \
        ../Shared/GobiMBNMgmt.cpp
OUTPUT=GobiQDLService

all: clean $(OUTPUT)

$(OUTPUT): $(SOURCES) libCore.a
	g++ $(CXXFLAGS) $(SOURCES) $(INCLUDES) -o $(OUTPUT)

debug: 
	$(MAKE) $(MFLAGS) CXXFLAGS="$(CXXFLAGS) $(DEBUGFLAGS)" all

install: $(OUTPUT)
	mkdir -p /opt/Qualcomm/Gobi/GobiQDLService/
	cp -f $(OUTPUT) /opt/Qualcomm/Gobi/GobiQDLService/
	cp -f 99-GobiQDLService.rules /etc/udev/rules.d/
	udevadm control reload_rules 2>/dev/null \
      || udevcontrol reload_rules 2>/dev/null \
      || udevadm control --reload-rules 2>/dev/null

clean:
	rm -f $(OUTPUT) \
         libCore.a \
         Comm.o \
         CoreUtilities.o \
         CRC.o \
         DB2TextFile.o \
         Event.o \
         HDLC.o \
         HDLCProtocolServer.o \
         MemoryMappedFile.o \
         ProtocolBuffer.o \
         ProtocolLog.o \
         ProtocolNotification.o \
         ProtocolRequest.o \
         ProtocolServer.o \
         QDLBuffers.o \
         QDLProtocolServer.o \
         SharedBuffer.o

libCore.a: Comm.o \
           CoreUtilities.o \
           CRC.o \
           Event.o \
           HDLC.o \
           HDLCProtocolServer.o \
           MemoryMappedFile.o \
           ProtocolBuffer.o \
           ProtocolLog.o \
           ProtocolNotification.o \
           ProtocolRequest.o \
           ProtocolServer.o \
           QDLBuffers.o \
           QDLProtocolServer.o \
           SharedBuffer.o
	ar rcs libCore.a \
          Comm.o \
          CoreUtilities.o \
          CRC.o \
          Event.o \
          HDLC.o \
          HDLCProtocolServer.o \
          MemoryMappedFile.o \
          ProtocolBuffer.o \
          ProtocolLog.o \
          ProtocolNotification.o \
          ProtocolRequest.o \
          ProtocolServer.o \
          QDLBuffers.o \
          QDLProtocolServer.o \
          SharedBuffer.o

Comm.o: ../Core/Comm.cpp
	g++ $(CXXFLAGS) -c ../Core/Comm.cpp

CoreUtilities.o: ../Core/CoreUtilities.cpp
	g++ $(CXXFLAGS) -c ../Core/CoreUtilities.cpp

CRC.o: ../Core/CRC.cpp
	g++ $(CXXFLAGS) -c ../Core/CRC.cpp

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

SharedBuffer.o: ../Core/SharedBuffer.cpp
	g++ $(CXXFLAGS) -c ../Core/SharedBuffer.cpp

