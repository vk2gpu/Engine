SET(SOURCES_PUBLIC 
	"allocator.h"
	"array.h"
	"concurrency.h"
	"debug.h"
	"dll.h"
	"enum.h"
	"file.h"
	"float.h"
	"handle.h"
	"hash.h"
	"library.h"
	"map.h"
	"misc.h"
	"mpmc_bounded_queue.h"
	"pair.h"
	"portability.h"
	"random.h"
	"set.h"
	"string.h"
	"timer.h"
	"types.h"
	"uuid.h"
	"vector.h"
)

SET(SOURCES_PRIVATE 
	"private/concurrency.cpp"
	"private/concurrency.inl"
	"private/debug.cpp"
	"private/enum.cpp"
	"private/file.cpp"
	"private/float.cpp"
	"private/handle.cpp"
	"private/hash.cpp"
	"private/library.cpp"
	"private/random.cpp"
	"private/string.cpp"
	"private/uuid.cpp"
	"private/timer.cpp"
)

SET(SOURCES_TESTS
	"tests/array_tests.cpp"
	"tests/concurrency_tests.cpp"
	"tests/file_tests.cpp"
	"tests/handle_tests.cpp"
	"tests/map_tests.cpp"
	"tests/string_tests.cpp"
	"tests/test_entry.cpp"
	"tests/uuid_tests.cpp"
	"tests/vector_tests.cpp"
)


ADD_ENGINE_LIBRARY(core ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
