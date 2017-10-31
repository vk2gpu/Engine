SET(SOURCES 
	"tlsf.c"
	"tlsf.h"
)

INCLUDE_DIRECTORIES(".")

# Add tlsf.
ADD_LIBRARY(tlsf STATIC ${SOURCES})
