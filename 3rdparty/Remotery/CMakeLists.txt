SET(SOURCES 
	"lib/Remotery.c"
	"lib/Remotery.h"
)

INCLUDE_DIRECTORIES("./lib")

# Add Remotery.
ADD_LIBRARY(Remotery SHARED ${SOURCES})
TARGET_COMPILE_DEFINITIONS(Remotery PRIVATE RMT_DLL=1)

