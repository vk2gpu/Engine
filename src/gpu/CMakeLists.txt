ADD_DEFINITIONS("-DGPU_EXPORT=1")

SET(SOURCES_PUBLIC 
	"commands.h"
	"command_list.h"
	"dll.h"
	"manager.h"
	"resources.h"
	"types.h"
	"utils.h"
)

SET(SOURCES_PRIVATE 
	"private/command_list.cpp"
	"private/command_list.inl"
	"private/interface.h"
	"private/manager.cpp"
	"private/utils.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/command_list_tests.cpp"
	"tests/gpu_tests.cpp"
)


# Add gpu.
ADD_LIBRARY(gpu SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(gpu PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(gpu core)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add gpu_test.
ADD_EXECUTABLE(gpu_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(gpu_test core client gpu)

SET_TARGET_PROPERTIES(gpu_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

