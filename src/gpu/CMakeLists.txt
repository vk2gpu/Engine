INCLUDE_DIRECTORIES(
	"${ENGINE_3RDPARTY_PATH}/renderdoc"
)

SET(SOURCES_PUBLIC 
	"backend.h"
	"commands.h"
	"command_list.h"
	"dll.h"
	"enum.h"
	"fwd_decls.h"
	"manager.h"
	"resources.h"
	"types.h"
	"utils.h"
)

SET(SOURCES_PRIVATE 
	"private/command_list.cpp"
	"private/command_list.inl"
	"private/dll.cpp"
	"private/enum.cpp"
	"private/manager.cpp"
	"private/resources.cpp"
	"private/resources.inl"
	"private/utils.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/command_list_tests.cpp"
	"tests/gpu_tests.cpp"
	"tests/transfer_tests.cpp"
)

ADD_ENGINE_LIBRARY(gpu ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(gpu core plugin)
TARGET_LINK_LIBRARIES(gpu_test client)
