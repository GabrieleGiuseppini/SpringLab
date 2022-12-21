cmake_minimum_required (VERSION 3.12)

if(POLICY CMP0048)
	cmake_policy(SET CMP0048 NEW)
endif()

if(POLICY CMP0074)
	cmake_policy(SET CMP0074 NEW)
endif()

if(POLICY CMP0082)
	# Run install rules in specified order
	cmake_policy(SET CMP0082 NEW)
endif()

if(POLICY CMP0087)
	# Allow generator expressions in install
	cmake_policy(SET CMP0087 NEW)
endif()

###################################################

project (SpringLab)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(FS_USE_STATIC_LIBS "Force static linking" ON)
option(FS_BUILD_BENCHMARKS "Build benchmarks" ON)

# Set architecture on Windows, so we may build 64- and 32-bit with agility
if(WIN32)
	if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
		set(CMAKE_LIBRARY_ARCHITECTURE "Win32")
	else()
		set(CMAKE_LIBRARY_ARCHITECTURE "x64")
	endif()
endif()

####################################################
# Custom CMake modules
####################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

####################################################
# User settings file
####################################################

set(USER_SETTINGS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/UserSettings.cmake")
if(EXISTS "${USER_SETTINGS_FILE}")
	message(STATUS "Loading user specific settings from ${USER_SETTINGS_FILE}")
	include("${USER_SETTINGS_FILE}")
endif()

####################################################
#  External libraries configuration
####################################################

message (STATUS "FS_USE_STATIC_LIBS:" ${FS_USE_STATIC_LIBS})
message (STATUS "FS_BUILD_BENCHMARKS:" ${FS_BUILD_BENCHMARKS})

#
# PicoJSON
#

find_package(PicoJSON REQUIRED)

#
# Google Benchmark
#

if(FS_BUILD_BENCHMARKS)
	list(APPEND CMAKE_PREFIX_PATH "${BENCHMARK_ROOT_DIR}")

	find_package(benchmark) # Not required as benchmarks are optional and used only for testing
endif()

#
# zlib, jpeg, png
#

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)

#
# wxWidgets
#

message(STATUS "wxWidgets_ROOT:" ${wxWidgets_ROOT})

find_package(wxWidgets REQUIRED base core gl html propgrid ribbon)

#
# DevIL
#

message(STATUS "DevIL_ROOT_DIR:" ${DevIL_ROOT_DIR})

find_package(DevIL REQUIRED)

if(FS_USE_STATIC_LIBS)
	# If we use static linking, we expect to have built DevIL 
	# as static libs, and we expect to _use_ it as static libs,
	# so tell the DevIL include files that DevIL is used as static libs
	add_compile_definitions("IL_STATIC_LIB")
endif()

#
# OpenGL
#

find_package(OpenGL REQUIRED)

####################################################
# Flags
####################################################

set(CMAKE_CXX_STANDARD 17)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")

	# Make sure the CRT is statically linked, and that we use W4 instead of W3
	foreach(flag_var
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
		if(FS_USE_STATIC_LIBS)
			if(${flag_var} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
			endif(${flag_var} MATCHES "/MD")
		endif()
		if(${flag_var} MATCHES "/W3")
			string(REGEX REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/W3")
		set(${flag_var} "${${flag_var}} /W4 /w44062")
	endforeach(flag_var)

	# Warnings are errors
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

	# Make RelWithDebInfo use aggressive inlining
	#if(${CMAKE_CXX_FLAGS_RELWITHDEBINFO} MATCHES "/Ob1")
	#	string(REGEX REPLACE "/Ob1" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	#endif(${CMAKE_CXX_FLAGS_RELWITHDEBINFO} MATCHES "/Ob1")
	#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2")
	
	# Disable incremental linking
	foreach(flag_var
		CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO CMAKE_EXE_LINKER_FLAGS_RELEASE)
		if(${flag_var} MATCHES "/INCREMENTAL")
			string(REGEX REPLACE "/INCREMENTAL:NO" "" ${flag_var} "${${flag_var}}")
			string(REGEX REPLACE "/INCREMENTAL" "" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/INCREMENTAL")
		set(${flag_var} "${${flag_var}} /INCREMENTAL:NO")
	endforeach(flag_var)

	# Enable LTCG:incremental in RelWithDebInfo
	foreach(flag_var
		CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO)
		if(${flag_var} MATCHES "/LTCG")
			string(REGEX REPLACE "/LTCG:incremental" "" ${flag_var} "${${flag_var}}")
			string(REGEX REPLACE "/LTCG" "" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/LTCG")
		set(${flag_var} "${${flag_var}} /LTCG:incremental")
	endforeach(flag_var)

	# Enable LTCG in release
	foreach(flag_var
		CMAKE_EXE_LINKER_FLAGS_RELEASE)
		if(${flag_var} MATCHES "/LTCG")
			string(REGEX REPLACE "/LTCG:incremental" "" ${flag_var} "${${flag_var}}")
			string(REGEX REPLACE "/LTCG" "" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/LTCG")
		set(${flag_var} "${${flag_var}} /LTCG")
	endforeach(flag_var)

	# Fine-tune optimizations
	string(REGEX REPLACE "/O2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast /Ox /GS-")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /fp:fast /Ox /GS-")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:strict /DFLOATING_POINT_CHECKS")
	
	# Enable multi-processor builds
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP")

	# Misc
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT /NODEFAULTLIB:MSVCRTD")
	add_compile_options(/permissive-)

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -O3 -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

endif()

add_definitions(-DPICOJSON_USE_INT64)

####################################################
# Misc
####################################################

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(ADDITIONAL_LIBRARIES comctl32 rpcrt4 advapi32)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(ADDITIONAL_LIBRARIES ${CMAKE_DL_LIBS} pthread stdc++fs atomic png jpeg)
	if (UNIX)
	    list(APPEND ADDITIONAL_LIBRARIES X11)
	endif (UNIX)
endif()

####################################################
# Sub-projects
####################################################

add_subdirectory(SLabCoreLib)
add_subdirectory(SpringLab)

if(FS_BUILD_BENCHMARKS)
	add_subdirectory(Benchmarks)
endif()

####################################################
# Visual Studio specifics
####################################################

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "SpringLab")
endif(MSVC)

####################################################
# Install
####################################################

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/Install" CACHE PATH "..." FORCE)

####################################################
# Diagnostics
####################################################

message (STATUS "-----------------------------------------------------")
message (STATUS "CMake CXX compiler ID:" ${CMAKE_CXX_COMPILER_ID})
message (STATUS "cxx Flags:" ${CMAKE_CXX_FLAGS})
message (STATUS "cxx Flags Release:" ${CMAKE_CXX_FLAGS_RELEASE})
message (STATUS "cxx Flags RelWithDebInfo:" ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message (STATUS "cxx Flags Debug:" ${CMAKE_CXX_FLAGS_DEBUG})
message (STATUS "c Flags:" ${CMAKE_C_FLAGS})
message (STATUS "c Flags Release:" ${CMAKE_C_FLAGS_RELEASE})
message (STATUS "c Flags RelWithDebInfo:" ${CMAKE_C_FLAGS_RELWITHDEBINFO})
message (STATUS "c Flags Debug:" ${CMAKE_C_FLAGS_DEBUG})
message (STATUS "exe Linker Flags Release:" ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
message (STATUS "exe Linker Flags RelWithDebInfo:" ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
message (STATUS "exe Linker Flags Debug:" ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
message (STATUS "wxWidgets libraries:" "${wxWidgets_LIBRARIES}")
message (STATUS "Additional libraries:" "${ADDITIONAL_LIBRARIES}")
message (STATUS "Install directory:" ${CMAKE_INSTALL_PREFIX})
message (STATUS "-----------------------------------------------------")
