SET(SOURCES_PUBLIC 
	"allocator.h"
	"allocator_tlsf.h"
	"allocator_overrides.h"
	"allocator_proxy_thread_safe.h"
	"allocator_proxy_tracker.h"
	"allocator_virtual.h"
	"array.h"
	"array_view.h"
	"command_line.h"
	"concurrency.h"
	"debug.h"
	"dll.h"
	"enum.h"
	"external_allocator.h"
	"file.h"
	"file_impl.h"
	"float.h"
	"function.h"
	"handle.h"
	"half.h"
	"hash.h"
	"library.h"
	"linear_allocator.h"
	"map.h"
	"misc.h"
	"mpmc_bounded_queue.h"
	"os.h"
	"pair.h"
	"portability.h"
	"random.h"
	"set.h"
	"string.h"
	"timer.h"
	"type_conversion.h"
	"types.h"
	"uuid.h"
	"vector.h"
)

SET(SOURCES_ISPC
)

SET(SOURCES_PRIVATE 
	"private/allocator.cpp"
	"private/allocator_proxy_thread_safe.cpp"
	"private/allocator_proxy_tracker.cpp"
	"private/allocator_tlsf.cpp"
	"private/allocator_virtual.cpp"
	"private/command_line.cpp"
	"private/concurrency.cpp"
	"private/concurrency.inl"
	"private/debug.cpp"
	"private/dll.cpp"
	"private/enum.cpp"
	"private/external_allocator.cpp"
	"private/external_allocator.inl"
	"private/file.cpp"
	"private/float.cpp"
	"private/handle.cpp"
	"private/half.cpp"
	"private/hash.cpp"
	"private/library.cpp"
	"private/linear_allocator.cpp"
	"private/misc.cpp"
	"private/misc.inl"
	"private/random.cpp"
	"private/string.cpp"
	"private/type_conversion.cpp"
	"private/uuid.cpp"
	"private/timer.cpp"
)

SET(SOURCES_TESTS
	"tests/allocator_tests.cpp"
	"tests/array_tests.cpp"
	"tests/concurrency_tests.cpp"
	"tests/file_tests.cpp"
	"tests/function_tests.cpp"
	"tests/handle_tests.cpp"
	"tests/map_tests.cpp"
	"tests/string_tests.cpp"
	"tests/test_entry.cpp"
	"tests/type_conversion_tests.cpp"
	"tests/uuid_tests.cpp"
	"tests/vector_tests.cpp"
)

INCLUDE_DIRECTORIES(
	"${ENGINE_3RDPARTY_PATH}/etlsf"
	"${ENGINE_3RDPARTY_PATH}/tlsf"
)

ADD_ENGINE_LIBRARY(core ${SOURCES_PUBLIC} ${SOURCES_ISPC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(core Remotery etlsf tlsf dbghelp)


