SET(SOURCES 
	"cpp.h"
	"cpp1.c"
	"cpp2.c"
	"cpp3.c"
	"cpp4.c"
	"cpp5.c"
	"cpp6.c"
	"cppadd.h"
	"cppdef.h"
	"fpp.h"
	"fpp_pragmas.h"
	"FPP_protos.h"
	"FPPBase.h"
)

INCLUDE_DIRECTORIES(".")

# Add fcpp.
ADD_LIBRARY(fcpp STATIC ${SOURCES})
TARGET_COMPILE_DEFINITIONS(fcpp PRIVATE)
