SET(SOURCES_PUBLIC 
	"d3d12_backend.h"
	"d3d12_command_list.h"
	"d3d12_descriptor_heap_allocator.h"
	"d3d12_device.h"
	"d3d12_linear_heap_allocator.h"
	"d3d12_resources.h"
	"d3d12_types.h"
	"dll.h"
)

SET(SOURCES_PRIVATE 
	"private/d3d12_backend.cpp"
	"private/d3d12_descriptor_heap_allocator.cpp"
	"private/d3d12_device.cpp"
	"private/d3d12_command_list.cpp"
	"private/d3d12_linear_heap_allocator.cpp"
	"private/d3d12_types.cpp"
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
