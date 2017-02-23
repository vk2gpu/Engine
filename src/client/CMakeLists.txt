

SET(SOURCES_PUBLIC 
	"window.h"
)

SET(SOURCES_PRIVATE 
	"private/window.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/window_tests.cpp"
)


# Add client.
ADD_LIBRARY(client STATIC ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(client PROPERTIES FOLDER Libraries)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add client_test.
ADD_EXECUTABLE(client_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(client_test SDL2-static core client)

SET_TARGET_PROPERTIES(client_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

