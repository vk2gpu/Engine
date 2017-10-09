SET(SOURCES_PUBLIC 
	"dll.h"
	"manager.h"
	"converter.h"
	"factory.h"
	"ref.h"
	"resource.h"
)

SET(SOURCES_PRIVATE 
	"private/converter_context.h"
	"private/converter_context.cpp"
	"private/database.h"
	"private/database.cpp"
	"private/factory_context.h"
	"private/factory_context.cpp"
	"private/jobs_fileio.h"
	"private/jobs_fileio.cpp"
	"private/manager.cpp"
	"private/path_resolver.h"
	"private/path_resolver.cpp"
	"private/ref.cpp"
)

SET(SOURCES_TESTS
	"tests/manager_tests.cpp"
	"tests/test_entry.cpp"
	"tests/resource_tests.cpp"
)

ADD_ENGINE_LIBRARY(resource ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(resource core job plugin serialization)

# Plugins
SET(SOURCES_TEST_BASIC
	"tests/converter_test_basic.cpp"
)

ADD_ENGINE_PLUGIN(converter_test_basic ${SOURCES_TEST_BASIC})
SET_TARGET_PROPERTIES(converter_test_basic PROPERTIES FOLDER Tests/Plugins)
TARGET_LINK_LIBRARIES(converter_test_basic core resource)
SOURCE_GROUP("Public" FILES ${SOURCES_TEST_BASIC})
