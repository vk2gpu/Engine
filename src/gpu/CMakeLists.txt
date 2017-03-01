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
	"private/backend.h"
	"private/manager.cpp"
	"private/utils.cpp"
)

SET(SOURCES_D3D12_PUBLIC 
	"d3d12/d3d12backend.h"
	"d3d12/d3d12types.h"
	"d3d12/dll.h"
)

SET(SOURCES_D3D12_PRIVATE 
	"d3d12/private/d3d12backend.cpp"
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
TARGET_COMPILE_DEFINITIONS(gpu PRIVATE GPU_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add gpu_d3d12
ADD_LIBRARY(gpu_d3d12 SHARED ${SOURCES_D3D12_PUBLIC} ${SOURCES_D3D12_PRIVATE})
SET_TARGET_PROPERTIES(gpu_d3d12 PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(gpu_d3d12 core gpu)
TARGET_COMPILE_DEFINITIONS(gpu PRIVATE GPU_D3D12_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_D3D12_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_D3D12_PRIVATE})

# Add gpu_test.
ADD_EXECUTABLE(gpu_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(gpu_test core client gpu)

SET_TARGET_PROPERTIES(gpu_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})
