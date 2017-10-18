SET(SOURCES 
	"alpha.cpp"
	"alpha.h"
	"clusterfit.cpp"
	"clusterfit.h"
	"colourblock.cpp"
	"colourblock.h"
	"colourfit.cpp"
	"colourfit.h"
	"colourset.cpp"
	"colourset.h"
	"config.h"
	"maths.cpp"
	"maths.h"
	"rangefit.cpp"
	"rangefit.h"
	"simd.h"
	"simd_float.h"
	"simd_sse.h"
	"simd_ve.h"
	"singlecolourfit.cpp"
	"singlecolourfit.h"
	"singlecolourlookup.inl"
	"squish.cpp"
	"squish.h"
)

INCLUDE_DIRECTORIES(".")

# Add squish.
ADD_LIBRARY(squish STATIC ${SOURCES})
TARGET_COMPILE_DEFINITIONS(squish PRIVATE SQUISH_USE_SSE=2)

IF(MSVC)
	STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	STRING (REGEX REPLACE "/O.*" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	TARGET_COMPILE_OPTIONS(squish PRIVATE /O2)
ELSE()
	TARGET_COMPILE_OPTIONS(squish PRIVATE -O3)
ENDIF()
