SET(SOURCES_PUBLIC 
	"dll.h"
	"factory.h"
	"texture.h"
)

SET(SOURCES_PRIVATE 
	"private/factory.cpp"
	"private/texture.cpp"
)

SET(SOURCES_TESTS
	"tests/converter_tests.cpp"
	"tests/test_entry.cpp"
)

# Add graphics.
ADD_LIBRARY(graphics SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(graphics PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(graphics core gpu job math resource)
TARGET_COMPILE_DEFINITIONS(graphics PRIVATE GRAPHICS_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add graphics_test.
ADD_EXECUTABLE(graphics_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(graphics_test client graphics)

SET_TARGET_PROPERTIES(graphics_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})


# Plugins
SET(SOURCES_TEXTURE_CONVERTER
	"converters/converter_texture.cpp"
	"converters/dds.h"
	"converters/dds.cpp"
	"converters/image.h"
	"converters/image.cpp"
)

INCLUDE_DIRECTORIES(
	# 3rd party includes.
	"../../3rdparty/stb"
	"../../3rdparty/squish"
)

ADD_LIBRARY(converter_graphics_texture SHARED ${SOURCES_TEXTURE_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_texture PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_texture core graphics resource squish)
SOURCE_GROUP("Public" FILES ${SOURCES_TEXTURE_CONVERTER})

