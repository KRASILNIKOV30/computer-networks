set(CMAKE_CXX_STANDARD 20)

add_executable(socket-programming
        src/lib/FileDesc.h
        src/lib/Acceptor.h
        src/lib/Connection.h
        src/lib/Socket.h
        src/Client.h
        src/Server.h
        main.cpp
        src/constants/Constants.h
)

set_target_properties(socket-programming PROPERTIES LINKER_LANGUAGE CXX)
