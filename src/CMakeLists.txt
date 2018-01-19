cmake_minimum_required(VERSION 2.8.4)

project(pinboard)

message(STATUS "start running cmake...")

find_package(Boost COMPONENTS system thread log date_time filesystem iostreams locale program_options regex REQUIRED)
find_package (Threads)

message("system: ${CMAKE_SYSTEM_PREFIX_PATH}")

find_library(LIB_BITCOIN bitcoin)
message("bitcoin: ${LIB_BITCOIN}")

find_library(LIB_ALTCOIN_NETWORK altcoin-network)
message("altcoin: ${LIB_ALTCOIN_NETWORK}")

set(CMAKE_CXX_FLAGS "-Wall -std=gnu++11 -g -Og -DBOOST_ALL_DYN_LINK")

if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(pinboard main.cpp
                        get_my_ip.cpp
                        chain_listener.cpp
                        object.cpp
                        multihash.cpp
                        pow_certificate.cpp
                        miner.cpp
                        pinboard.cpp
                        lite_header.cpp
                        lite_node.cpp
                        session_lite_inbound.cpp
                        session_lite_outbound.cpp
                        session_lite_manual.cpp
                        protocol_lite_header_sync.cpp
                        protocol_pinboard_sync.cpp
                        protocol_address.cpp
                        message_subscriber_ex.cpp
                        message_broadcaster.cpp
                        libaltcoin_network_impl.cpp
              )

if(Boost_FOUND)
    target_link_libraries(pinboard ${Boost_LIBRARIES} ${LIB_BITCOIN} ${LIB_ALTCOIN_NETWORK} ${CMAKE_THREAD_LIBS_INIT})
endif()
