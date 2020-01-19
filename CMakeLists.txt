cmake_minimum_required(VERSION 3.6)

project(IPC LANGUAGES C CXX)

set(IPC_VERSION_MAJOR 1)
set(IPC_VERSION_MINOR 2)
set(IPC_VERSION "${IPC_VERSION_MAJOR}.${IPC_detection_VERSION_MINOR}")



set(CMAKE_CXX_FLAGS "${CMALE_CXX_FLAGS} -std=c++11 -Wall -O2")

#file(GLOB SRCIPC "src/*.c*")

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_BINARY_DIR}")

#IPC to Client Server Socket:
set(Total_src src/Socket.cpp src/SharedMemory.cpp src/Routing_table.cpp)
set(SRCIPC_serv src/Server.cpp ${Total_src})
set(SRCIPC_client src/Client.cpp ${Total_src})

add_executable(${PROJECT_NAME}_Server ${SRCIPC_serv})
add_executable(${PROJECT_NAME}_Client ${SRCIPC_client})

#add_executable(${PROJECT_NAME} ${SRCIPC})
#lib rt with Shared Memory
if (UNIX)
    target_link_libraries(${PROJECT_NAME}_Server rt)
    target_link_libraries(${PROJECT_NAME}_Client rt)
endif()