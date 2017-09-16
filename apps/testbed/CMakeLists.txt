SET(SOURCES_PUBLIC 
	"common.h"
	"forward_pipeline.h"
	"forward_pipeline.cpp"
	"imgui_pipeline.h"
	"imgui_pipeline.cpp"
	"test_entry.cpp"
	"test_shared.h"
	"texture_compressor.h"
	"texture_compressor.cpp"
	"render_packets.h"
	"render_packets.cpp"
)

ADD_ENGINE_EXECUTABLE(testbed ${SOURCES_PUBLIC})
SET_TARGET_PROPERTIES(testbed PROPERTIES FOLDER Apps)
TARGET_LINK_LIBRARIES(testbed core graphics imgui plugin)
