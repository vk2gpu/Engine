SET(SOURCES_PUBLIC 
	"dll.h"
	"model.h"
	"render_graph.h"
	"render_pass.h"
	"render_resources.h"
	"shader.h"
	"texture.h"
)

SET(SOURCES_PRIVATE 
	"private/model.cpp"
	"private/model_impl.h"
	"private/render_graph.cpp"
	"private/render_pass.cpp"
	"private/shader.cpp"
	"private/shader_impl.h"
	"private/texture.cpp"
	"private/texture_impl.h"
)

SET(SOURCES_TESTS
	"tests/converter_tests.cpp"
	"tests/model_tests.cpp"
	"tests/render_graph_tests.cpp"
	"tests/shader_parser_tests.cpp"
	"tests/shader_tests.cpp"
	"tests/test_entry.cpp"
	"tests/test_shared.h"

	# Pull in shader parser for test usage.
	"converters/shader_ast.h"
	"converters/shader_ast.cpp"
	"converters/shader_backend_hlsl.h"
	"converters/shader_backend_hlsl.cpp"
	"converters/shader_backend_metadata.h"
	"converters/shader_backend_metadata.cpp"
	"converters/shader_compiler_hlsl.h"
	"converters/shader_compiler_hlsl.cpp"
	"converters/shader_parser.h"
	"converters/shader_parser.cpp"
	"converters/shader_preprocessor.h"
	"converters/shader_preprocessor.cpp"
)

ADD_ENGINE_LIBRARY(graphics ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(graphics gpu job math resource)
TARGET_LINK_LIBRARIES(graphics_test client fcpp)

# Plugins
SET(SOURCES_MODEL_CONVERTER
	"converters/converter_model.cpp"
)

SET(SOURCES_TEXTURE_CONVERTER
	"converters/converter_texture.cpp"
	"converters/dds.h"
	"converters/dds.cpp"
	"converters/image.h"
	"converters/image.cpp"
)

SET(SOURCES_SHADER_CONVERTER
	"converters/converter_shader.cpp"
	"converters/shader_ast.h"
	"converters/shader_ast.cpp"
	"converters/shader_backend_hlsl.h"
	"converters/shader_backend_hlsl.cpp"
	"converters/shader_backend_metadata.h"
	"converters/shader_backend_metadata.cpp"
	"converters/shader_compiler_hlsl.h"
	"converters/shader_compiler_hlsl.cpp"
	"converters/shader_parser.h"
	"converters/shader_parser.cpp"
	"converters/shader_preprocessor.h"
	"converters/shader_preprocessor.cpp"
)

INCLUDE_DIRECTORIES(
	# 3rd party includes.
	"${ENGINE_3RDPARTY_PATH}/fcpp"
	"${ENGINE_3RDPARTY_PATH}/stb"
	"${ENGINE_3RDPARTY_PATH}/squish"
	"${ENGINE_3RDPARTY_PATH}/assimp/include"
)

ADD_ENGINE_PLUGIN(converter_graphics_model SHARED ${SOURCES_MODEL_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_model PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_model core graphics resource assimp)

ADD_ENGINE_PLUGIN(converter_graphics_shader SHARED ${SOURCES_SHADER_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_shader PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_shader core graphics resource fcpp)

ADD_ENGINE_PLUGIN(converter_graphics_texture SHARED ${SOURCES_TEXTURE_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_texture PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_texture core graphics resource squish)
