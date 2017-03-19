SET(SOURCES_PUBLIC 
	"dll.h"
	"imgui.h"
)

SET(SOURCES_PRIVATE 
	"private/imgui.cpp"
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
ADD_LIBRARY(imgui SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_PRIVATE_IMGUI_MASTER})
SET_TARGET_PROPERTIES(imgui PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(imgui client gpu math)
TARGET_COMPILE_DEFINITIONS(imgui PRIVATE IMGUI_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})
SOURCE_GROUP("Private\\ImGui-master" FILES ${SOURCES_PRIVATE_IMGUI_MASTER})

# Add client_test.
ADD_EXECUTABLE(imgui_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(imgui_test imgui)

SET_TARGET_PROPERTIES(imgui_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

