SET(SOURCES_PUBLIC 
	"dll.h"
	"serializer.h"
)

SET(SOURCES_PRIVATE 
	"private/serializer.cpp"
)

SET(SOURCES_TESTS
	"tests/serializer_tests.cpp"
	"tests/test_entry.cpp"
)

INCLUDE_DIRECTORIES(
	# 3rd party includes.
	"../../jsoncpp/include"
)

# Add core.
ADD_LIBRARY(serialization SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(serialization PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(serialization core jsoncpp_lib_static)
TARGET_COMPILE_DEFINITIONS(serialization PRIVATE SERIALIZATION_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add core_test.
ADD_EXECUTABLE(serialization_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(serialization_test serialization)

SET_TARGET_PROPERTIES(serialization_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

