

SET(SOURCES_PUBLIC 
	"array.h"
	"debug.h"
	"hash.h"
	"map.h"
	"pair.h"
	"portability.h"
	"types.h"
	"vector.h"
)

SET(SOURCES_PRIVATE 
	"private/debug.cpp"
	"private/hash.cpp"
)

SET(SOURCES_TESTS
	"tests/array_tests.cpp"
	"tests/map_tests.cpp"
	"tests/test_entry.cpp"
	"tests/vector_tests.cpp"
)


# Add core.
ADD_LIBRARY(core STATIC ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(core PROPERTIES FOLDER Libraries)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add core_test.
ADD_EXECUTABLE(core_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(core_test core)

SET_TARGET_PROPERTIES(core_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

