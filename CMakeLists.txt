CMAKE_MINIMUM_REQUIRED(VERSION 3.2.3)

# Engine project is C++
PROJECT(Engine CXX)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

SET(CMAKE_MODULE_PATH "cmake")

# Version.
SET(ENGINE_MAJOR_VERSION 0)
SET(ENGINE_MINOR_VERSION 0)
SET(ENGINE_MICRO_VERSION 0)
SET(ENGINE_VERSION "${ENGINE_MAJOR_VERSION}.${ENGINE_MINOR_VERSION}.${ENGINE_MICRO_VERSION}")

# Setup warning level.
IF(MSVC)
    ADD_DEFINITIONS(/W4)		# level 4
    ADD_DEFINITIONS(/WX)		# warnings as errors
    ADD_DEFINITIONS(/wd4100)	# disable unreferenced formal parameter
ENDIF()


# Include directories.
INCLUDE_DIRECTORIES(
	# 3rd party includes.
	"3rdparty/Catch/include"

	# Source.
	"src"
)

# Add source.
ADD_SUBDIRECTORY("src/core")
ADD_SUBDIRECTORY("src/math")


