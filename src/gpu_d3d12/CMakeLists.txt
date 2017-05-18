INCLUDE_DIRECTORIES(
	"${ENGINE_3RDPARTY_PATH}/dxsdk/include"
)

SET(SOURCES_PUBLIC 
	"d3d12_backend.h"
	"d3d12_command_list.h"
	"d3d12_compile_context.h"
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
	"private/d3d12_compile_context.cpp"
	"private/d3d12_linear_heap_allocator.cpp"
	"private/d3d12_types.cpp"
)

ADD_ENGINE_PLUGIN(gpu_d3d12 ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
TARGET_LINK_LIBRARIES(gpu_d3d12 core gpu)
