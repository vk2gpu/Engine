SET(SOURCES_PUBLIC 
	"dll.h"
	"plugin.h"
	"manager.h"
)

SET(SOURCES_PRIVATE 
	"private/manager.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/plugin_tests.cpp"
)


# Add plugin.
ADD_LIBRARY(plugin SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(plugin PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(plugin core)
TARGET_COMPILE_DEFINITIONS(plugin PRIVATE PLUGIN_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add plugin_test.
ADD_EXECUTABLE(plugin_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(plugin_test plugin)

SET_TARGET_PROPERTIES(plugin_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

# Plugins
SET(SOURCES_TEST_BASIC
	"tests/plugin_test_basic.cpp"
)

ADD_LIBRARY(plugin_test_basic SHARED ${SOURCES_TEST_BASIC})
SET_TARGET_PROPERTIES(plugin_test_basic PROPERTIES FOLDER Tests/Plugins)
TARGET_LINK_LIBRARIES(plugin_test_basic core plugin)
SOURCE_GROUP("Private" FILES ${SOURCES_TEST_BASIC})

