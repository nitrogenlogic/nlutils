set(NLUTILS_VERSION 0.12.3)
set(NLUTILS_SO_VERSION 12)

cmake_minimum_required(VERSION 2.6)

# Prevent project() from clearing VERSION variables
if(POLICY CMP0048)
	cmake_policy(SET CMP0048 OLD)
endif()

# Allow setting COMPILE_DEFINITIONS_DEBUG
if(POLICY CMP0043)
	cmake_policy(SET CMP0043 OLD)
endif()

project(NLUTILS)

# Prevent absolute paths from being compiled in
set(CMAKE_USE_RELATIVE_PATHS true)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99 -D_XOPEN_SOURCE=700 -D_ISOC99_SOURCE -D_GNU_SOURCE -fPIC -pthread -Wall -Wextra -Werror -pipe -DNLUTILS_VERSION=\"\\\"${NLUTILS_VERSION}\\\"\" -D_FILE_OFFSET_BITS=64 -DLARGEFILE_SOURCE=1")

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG DEBUG=1)

include_directories(BEFORE include/)

add_subdirectory(src)
add_subdirectory(programs)
add_subdirectory(tests)

if(NL_EMBEDDED)
	add_subdirectory(embedded)
else(NL_EMBEDDED)
	add_subdirectory(tools)
endif(NL_EMBEDDED)

if(NL_INSTALL_HEADERS OR (NOT NL_EMBEDDED AND NOT NL_PACKAGE))
	install(DIRECTORY include/ DESTINATION include/nlutils
		FILES_MATCHING PATTERN "*.h")
endif(NL_INSTALL_HEADERS OR (NOT NL_EMBEDDED AND NOT NL_PACKAGE))
