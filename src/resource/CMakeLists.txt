SET(SOURCES_PUBLIC 
	"dll.h"
	"manager.h"
	"converter.h"
	"factory.h"
	"resource.h"
)

SET(SOURCES_PRIVATE 
	"private/database.h"
	"private/database.cpp"
	"private/manager.cpp"
)

SET(SOURCES_TESTS
	"tests/database_tests.cpp"
	"tests/manager_tests.cpp"
	"tests/test_entry.cpp"
	"tests/resource_tests.cpp"
)


# Add resource.
ADD_LIBRARY(resource SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(resource PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(resource core job plugin)
TARGET_COMPILE_DEFINITIONS(resource PRIVATE RESOURCE_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add resource_test.
ADD_EXECUTABLE(resource_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(resource_test resource plugin)

SET_TARGET_PROPERTIES(resource_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

# Plugins
SET(SOURCES_TEST_BASIC
	"tests/converter_test_basic.cpp"
)

ADD_LIBRARY(converter_test_basic SHARED ${SOURCES_TEST_BASIC})
SET_TARGET_PROPERTIES(converter_test_basic PROPERTIES FOLDER Tests/Plugins)
TARGET_LINK_LIBRARIES(converter_test_basic core resource)
SOURCE_GROUP("Public" FILES ${SOURCES_TEST_BASIC})
