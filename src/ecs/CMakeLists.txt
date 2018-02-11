SET(SOURCES_PUBLIC 
	"dll.h"
	"system.h"
)

SET(SOURCES_PRIVATE
	"private/system.cpp"
)

SET(SOURCES_ISPC

)

SET(SOURCES_TESTS
	"tests/ecs_tests.cpp"
	"tests/test_entry.cpp"
)

ADD_ENGINE_LIBRARY(ecs ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_ISPC} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(ecs core math)
