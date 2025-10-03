set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(CMAKE_C_COMPILER clang)
#set(CMAKE_C_FLAGS "-stdlib=libc -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS)

set(CMAKE_CXX_COMPILER clang++)
#set(CMAKE_CXX_FLAGS "-stdlib=libc++ -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "-stdlib=libc++")
