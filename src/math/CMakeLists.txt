SET(SOURCES_PUBLIC 
	"aabb.h"
	"dll.h"
	"interpolation.h"
	"mat33.h"
	"mat44.h"
	"plane.h"
	"quat.h"
	"vec2.h"
	"vec3.h"
	"vec4.h"
)

SET(SOURCES_PRIVATE 
	"private/aabb.cpp"
	"private/mat33.cpp"
	"private/mat44.cpp"
	"private/plane.cpp"
	"private/quat.cpp"
	"private/vec2.cpp"
	"private/vec3.cpp"
	"private/vec4.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/vec2_tests.cpp"
)


# Add core.
ADD_LIBRARY(math SHARED ${SOURCES_PUBLIC} ${SOURCES_PRIVATE})
SET_TARGET_PROPERTIES(math PROPERTIES FOLDER Libraries)
TARGET_LINK_LIBRARIES(math core)
TARGET_COMPILE_DEFINITIONS(math PRIVATE MATH_EXPORT=1)

SOURCE_GROUP("Public" FILES ${SOURCES_PUBLIC})
SOURCE_GROUP("Private" FILES ${SOURCES_PRIVATE})

# Add core_test.
ADD_EXECUTABLE(math_test ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(math_test math)

SET_TARGET_PROPERTIES(math_test PROPERTIES FOLDER Tests)
SOURCE_GROUP("Tests" FILES ${SOURCES_TESTS})

