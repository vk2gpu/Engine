SET(SOURCES_PUBLIC 
	"dll.h"
	"client.h"
	"window.h"
)

SET(SOURCES_PRIVATE 
	"private/client.cpp"
	"private/window.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/window_tests.cpp"
)


# Add client.
ADD_LIBRARY(client SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(client PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(client SDL2 core)
TARGET_COMPILE_DEFINITIONS(client PRIVATE CLIENT_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add client_test.
ADD_EXECUTABLE(client_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(client_test client)

SET_TARGET_PROPERTIES(client_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

