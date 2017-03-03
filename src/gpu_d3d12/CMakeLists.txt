SET(SOURCES_PUBLIC 
	"d3d12backend.h"
	"d3d12commandlist.h"
	"d3d12types.h"
	"dll.h"
)

SET(SOURCES_PRIVATE 
	"private/d3d12backend.cpp"
	"private/d3d12commandlist.cpp"
	"private/d3d12types.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
)

# Add gpu_d3d12.
ADD_LIBRARY(gpu_d3d12 SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(gpu_d3d12 PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(gpu_d3d12 core gpu)
TARGET_COMPILE_DEFINITIONS(gpu_d3d12 PRIVATE GPU_D3D12_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})
