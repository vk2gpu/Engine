SET(SOURCES_PUBLIC 
	"dll.h"
	"plugin.h"
	"manager.h"
)

SET(SOURCES_PRIVATE 
	"private/dll.cpp"
	"private/manager.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/plugin_tests.cpp"
)

ADD_ENGINE_LIBRARY(plugin ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(plugin core)

# Plugins
SET(SOURCES_TEST_BASIC
	"tests/plugin_test_basic.h"
	"tests/plugin_test_basic.cpp"
)

ADD_ENGINE_PLUGIN(plugin_test_basic ${SOURCES_TEST_BASIC})
TARGET_LINK_LIBRARIES(plugin_test_basic core plugin)

SET(SOURCES_TEST_ADVANCED
	"tests/plugin_test_advanced.h"
	"tests/plugin_test_advanced.cpp"
)

ADD_ENGINE_PLUGIN(plugin_test_advanced ${SOURCES_TEST_ADVANCED})
TARGET_LINK_LIBRARIES(plugin_test_advanced core plugin)
