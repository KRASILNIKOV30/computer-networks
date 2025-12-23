set(CMAKE_CXX_STANDARD 20)

add_executable(socket-programming
        ../lib/FileDesc.h
        ../lib/Acceptor.h
        ../lib/Connection.h
        ../lib/Socket.h
        src/Client.h
        src/Server.h
        main.cpp
        src/constants/Constants.h
)

set_target_properties(socket-programming PROPERTIES LINKER_LANGUAGE CXX)
