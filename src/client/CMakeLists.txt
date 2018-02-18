INCLUDE_DIRECTORIES(
	"${ENGINE_3RDPARTY_PATH}/SDL-mirror/include"
)

SET(SOURCES_PUBLIC 
	"dll.h"
	"input_provider.h"
	"key_input.h"
	"manager.h"
	"window.h"
)

SET(SOURCES_PRIVATE 
	"private/dll.cpp"
	"private/manager_impl.h"
	"private/manager.cpp"
	"private/window.cpp"
	"private/window_impl.h"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/window_tests.cpp"
)

ADD_ENGINE_LIBRARY(client ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(client core math SDL2)
