pushd %~dp0
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto

GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=C_ --send=S_
GenPackets.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=S_ --send=C_

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.pb.h "../../../Server"
XCOPY /Y Enum.pb.cc "../../../Server"
XCOPY /Y Struct.pb.h "../../../Server"
XCOPY /Y Struct.pb.cc "../../../Server"
XCOPY /Y Protocol.pb.h "../../../Server"
XCOPY /Y Protocol.pb.cc "../../../Server"
XCOPY /Y ServerPacketHandler.h "../../../Server"

XCOPY /Y Enum.pb.h "../../../DummyClient"
XCOPY /Y Enum.pb.cc "../../../DummyClient"
XCOPY /Y Struct.pb.h "../../../DummyClient"
XCOPY /Y Struct.pb.cc "../../../DummyClient"
XCOPY /Y Protocol.pb.h "../../../DummyClient"
XCOPY /Y Protocol.pb.cc "../../../DummyClient"
XCOPY /Y ClientPacketHandler.h "../../../DummyClient"

XCOPY /Y Enum.pb.h "../../../../S1/Source/S1/Network/Protocol"
XCOPY /Y Enum.pb.cc "../../../../S1/Source/S1/Network/Protocol"
XCOPY /Y Struct.pb.h "../../../../S1/Source/S1/Network/Protocol"
XCOPY /Y Struct.pb.cc "../../../../S1/Source/S1/Network/Protocol"
XCOPY /Y Protocol.pb.h "../../../../S1/Source/S1/Network/Protocol"
XCOPY /Y Protocol.pb.cc "../../../../S1/Source/S1/Network/Protocol"
XCOPY /Y ClientPacketHandler.h "../../../../S1/Source/S1/Network"

DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

PAUSE