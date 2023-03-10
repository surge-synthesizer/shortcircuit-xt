# This was modified from the 4.3.0 to build libgig on all our platforms - baconpaul
cmake_minimum_required(VERSION 3.0)
project(libgig)

#get version from configure.ac
#have to make a slight detour since I cannot get the cmake regex subgroup to work with file
file(STRINGS configure.ac VERSION_STRINGS REGEX "^m4_define\\(libgig_release.*\\)$")

string(REGEX MATCH "libgig_release_major, ([^)]*)\\)" TEMP ${VERSION_STRINGS})
set(VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "libgig_release_minor, ([^)]*)\\)" TEMP ${VERSION_STRINGS})
set(VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "libgig_release_build, ([^)]*)\\)" TEMP ${VERSION_STRINGS})
set(VERSION_BUILD ${CMAKE_MATCH_1})

set(PACKAGE_NAME "libgig")

set(LIBGIG_HEADERS
	src/DLS.h
	src/gig.h
	src/RIFF.h
	src/SF.h
	src/Korg.h
	src/Serialization.h)

#libgig core library
add_library(libgig STATIC
	src/DLS.cpp
	src/gig.cpp
	src/helper.cpp
	src/Korg.cpp
	src/RIFF.cpp
	src/Serialization.cpp
	src/SF.cpp
	src/typeinfo.cpp)

set_target_properties(libgig PROPERTIES PUBLIC_HEADER "${LIBGIG_HEADERS}")
target_include_directories(libgig PUBLIC src)
target_compile_definitions(libgig PRIVATE PACKAGE="${PACKAGE_NAME}" VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}")
target_include_directories(libgig PUBLIC $<INSTALL_INTERFACE:include>)

#libakai library
add_library(libakai src/Akai.cpp)
set_target_properties(libakai PROPERTIES PUBLIC_HEADER src/Akai.h)
target_include_directories(libakai PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories(libakai PUBLIC src)
target_compile_definitions(libakai PRIVATE PACKAGE="${PACKAGE_NAME}" VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}")


if (WIN32)
	target_compile_definitions(libgig PRIVATE NOMINMAX POSIX=0)
	target_compile_definitions(libakai PRIVATE NOMINMAX POSIX=0)
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		target_compile_definitions(libgig PRIVATE WINDOWS_CLANG_SCXT)
		target_compile_definitions(libakai PRIVATE WINDOWS_CLANG_SCXT)
	else()
		target_compile_definitions(libgig PRIVATE WINDOWS_MSVC_SCXT)
		target_compile_definitions(libakai PRIVATE WINDOWS_MSVC_SCXT)
	endif()
endif()