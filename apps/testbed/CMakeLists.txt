SET(SOURCES_PUBLIC 
	"test_entry.cpp"
	"test_shared.h"
)

ADD_ENGINE_EXECUTABLE(testbed ${SOURCES_PUBLIC})
SET_TARGET_PROPERTIES(testbed PROPERTIES FOLDER Apps)
TARGET_LINK_LIBRARIES(testbed core graphics imgui plugin)
