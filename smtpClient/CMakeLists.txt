set(CMAKE_CXX_STANDARD 20)

add_executable(smtp-client
        src/main.cpp
        src/SmtpClient.h
        ../lib/FileDesc.h
        ../lib/Connection.h
)

set_target_properties(smtp-client PROPERTIES LINKER_LANGUAGE CXX)