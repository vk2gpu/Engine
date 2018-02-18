SET(SOURCES_PUBLIC 
	"dll.h"
	"color.h"
	"image.h"
	"load.h"
	"process.h"
	"save.h"
	"types.h"
)

SET(SOURCES_PRIVATE 
	"private/dds.cpp"
	"private/dds.h"
	"private/dll.cpp"
	"private/color.cpp"
	"private/image.cpp"
	"private/load.cpp"
	"private/process.cpp"
	"private/save.cpp"
)

SET(SOURCES_ISPC
	"ispc/image_processing.ispc"
	"ispc/ispc_texcomp_kernel.ispc"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/color_tests.cpp"
	"tests/image_tests.cpp"
)

INCLUDE_DIRECTORIES(
	# 3rd party includes.
	"${ENGINE_3RDPARTY_PATH}/stb"
	"${ENGINE_3RDPARTY_PATH}/squish"
)

ADD_ENGINE_LIBRARY(image ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_ISPC} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(image core math gpu squish)
