SET(SOURCES_PUBLIC 
	"dll.h"
	"basic_job.h"
	"concurrency.h"
	"function_job.h"
	"manager.h"
	"types.h"
)

SET(SOURCES_PRIVATE 
	"private/basic_job.cpp"
	"private/concurrency.cpp"
	"private/dll.cpp"
	"private/function_job.cpp"
	"private/manager.cpp"
)

SET(SOURCES_TESTS
	"tests/test_entry.cpp"
	"tests/job_tests.cpp"
)

ADD_ENGINE_LIBRARY(job ${SOURCES_PUBLIC} ${SOURCES_PRIVATE} ${SOURCES_TESTS})
TARGET_LINK_LIBRARIES(job core Remotery)
