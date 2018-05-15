SET(SOURCES_PUBLIC 
	"dll.h"
	"material.h"
	"model.h"
	"pipeline.h"
	"render_graph.h"
	"render_pass.h"
	"render_resources.h"
	"shader.h"
	"texture.h"
	"virtual_texture.h"
)

SET(SOURCES_ISPC 
)

SET(SOURCES_PRIVATE 
	"private/dll.cpp"
	"private/model.cpp"
	"private/model_impl.h"
	"private/material.cpp"
	"private/material_impl.h"
	"private/pipeline.cpp"
	"private/render_graph.cpp"
	"private/render_pass.cpp"
	"private/render_pass_impl.h"
	"private/render_resources.cpp"
	"private/shader.cpp"
	"private/shader_impl.h"
	"private/texture.cpp"
	"private/texture_impl.h"
	"private/virtual_texture.cpp"
	"private/virtual_texture_impl.h"
)

SET(SOURCES_TESTS
	"tests/converter_tests.cpp"
	"tests/model_tests.cpp"
	"tests/render_graph_tests.cpp"
	"tests/shader_parser_tests.cpp"
	"tests/shader_tests.cpp"
	"tests/test_entry.cpp"
	"tests/test_shared.h"
	"tests/virtual_texture_tests.cpp"

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

ADD_ENGINE_LIBRARY(graphics ${SOURCES_PUBLIC} ${SOURCES_ISPC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(graphics gpu job math resource image)
TARGET_LINK_LIBRARIES(graphics_test client imgui fcpp)

# Plugins
SET(SOURCES_MATERIAL_CONVERTER
	"converters/converter_material.cpp"
	"converters/import_material.h"
)

SET(SOURCES_MODEL_CONVERTER
	"converters/converter_model.cpp"
	"converters/import_model.h"
)

SET(SOURCES_SHADER_CONVERTER
	"converters/converter_shader.cpp"
	"converters/import_shader.h"
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

SET(SOURCES_TEXTURE_CONVERTER
	"converters/converter_texture.cpp"
	"converters/import_texture.h"
)

SET(SOURCES_TEST_PIPELINE
	"pipelines/pipeline_test.cpp"
)

INCLUDE_DIRECTORIES(
	# 3rd party includes.
	"${ENGINE_3RDPARTY_PATH}/fcpp"
	"${ENGINE_3RDPARTY_PATH}/stb"
	"${ENGINE_3RDPARTY_PATH}/squish"
	"${ENGINE_3RDPARTY_PATH}/assimp/include"
)

# Converter plugins.
ADD_ENGINE_PLUGIN(converter_graphics_material ${SOURCES_MATERIAL_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_material PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_material core graphics resource)

ADD_ENGINE_PLUGIN(converter_graphics_model ${SOURCES_MODEL_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_model PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_model core graphics resource assimp)

ADD_ENGINE_PLUGIN(converter_graphics_shader ${SOURCES_SHADER_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_shader PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_shader core graphics resource fcpp)

ADD_ENGINE_PLUGIN(converter_graphics_texture ${SOURCES_TEXTURE_CONVERTER})
SET_TARGET_PROPERTIES(converter_graphics_texture PROPERTIES FOLDER Libraries/Plugins)
TARGET_LINK_LIBRARIES(converter_graphics_texture core graphics resource image)

# Pipeline plugins.
ADD_ENGINE_PLUGIN(graphics_test_pipeline ${SOURCES_TEST_PIPELINE})
SET_TARGET_PROPERTIES(graphics_test_pipeline PROPERTIES FOLDER Tests/Plugins)
TARGET_LINK_LIBRARIES(graphics_test_pipeline core graphics)
