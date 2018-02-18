SET(SOURCES_PUBLIC 
	"aabb.h"
	"dll.h"
	"interpolation.h"
	"mat33.h"
	"mat44.h"
	"plane.h"
	"quat.h"
	"utils.h"
	"vec2.h"
	"vec3.h"
	"vec4.h"
)

SET(SOURCES_PRIVATE 
	"private/aabb.cpp"
	"private/dll.cpp"
	"private/mat33.cpp"
	"private/mat44.cpp"
	"private/plane.cpp"
	"private/quat.cpp"
	"private/utils.inl"
	"private/utils.cpp"
	"private/vec2.cpp"
	"private/vec3.cpp"
	"private/vec4.cpp"
)

SET(SOURCES_ISPC
	"ispc/mat44.ispc"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/mat44_tests.cpp"
	"tests/utils_tests.cpp"
	"tests/vec2_tests.cpp"

)

ADD_ENGINE_LIBRARY(math ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_ISPC} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(math core)
