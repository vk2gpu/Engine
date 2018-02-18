SET(SOURCES_PUBLIC 
	"dll.h"
	"manager.h"
)

SET(SOURCES_PRIVATE 
	"private/dll.cpp"
	"private/manager.cpp"
)

SET(SOURCES_PRIVATE_IMGUI_MASTER
	"private/imgui-master/imconfig.h"
	"private/imgui-master/imgui.cpp"
	"private/imgui-master/imgui.h"
	"private/imgui-master/imgui_demo.cpp"
	"private/imgui-master/imgui_draw.cpp"
	"private/imgui-master/imgui_internal.h"
	"private/imgui-master/stb_rect_pack.h"
	"private/imgui-master/stb_textedit.h"
	"private/imgui-master/stb_truetype.h"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/imgui_tests.cpp"
)


# Add client.
ADD_ENGINE_LIBRARY(imgui ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_PRIVATE_IMGUI_MASTER} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(imgui client gpu math)
TARGET_LINK_LIBRARIES(imgui_test client)

SOURCE_GROUP("Private\\ImGui-master" FILES ${SOURCES_PRIVATE_IMGUI_MASTER})
