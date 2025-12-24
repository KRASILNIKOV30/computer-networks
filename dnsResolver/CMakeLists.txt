set(CMAKE_CXX_STANDARD 20)

add_executable(dns-resolver
        src/main.cpp
        src/DnsResolver.h
)

set_target_properties(dns-resolver PROPERTIES LINKER_LANGUAGE CXX)