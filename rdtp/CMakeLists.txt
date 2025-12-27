set(CMAKE_CXX_STANDARD 20)

add_library(rdt-common INTERFACE)
target_include_directories(rdt-common INTERFACE src/common)

add_executable(rdt_sender
        src/sender/main.cpp
        src/sender/GbnSender.h
        src/common/Packet.h
        src/common/RdtSocket.h
        ../lib/FileDesc.h
)
target_link_libraries(rdt_sender rdt-common)

add_executable(rdt_receiver
        src/receiver/main.cpp
        src/receiver/GbnReceiver.h
        src/common/Packet.h
        src/common/RdtSocket.h
        ../lib/FileDesc.h
)
target_link_libraries(rdt_receiver rdt-common)