SET(SOURCES 
	"etlsf.c"
	"etlsf.h"
)

INCLUDE_DIRECTORIES(".")

# Add etlsf.
ADD_LIBRARY(etlsf STATIC ${SOURCES})
