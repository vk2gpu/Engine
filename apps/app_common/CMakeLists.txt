SET(SOURCES_PUBLIC 
	"dll.h"
	"app.h"
	"app.cpp"
	"camera.h"
	"camera.cpp"
	"common.h"
	"forward_pipeline.h"
	"forward_pipeline.cpp"
	"imgui_pipeline.h"
	"imgui_pipeline.cpp"
	"test_shared.h"
	"texture_compressor.h"
	"texture_compressor.cpp"
	"render_packets.h"
	"render_packets.cpp"
	"shadow_pipeline.h"
	"shadow_pipeline.cpp"
)

IF(ENABLE_SIMPLYGON)
	SET(SIMPLYGON_PATH "C:/Program Files/Simplygon/8/Tools")
	SET(SIMPLYGON_SDK_PATH "${SIMPLYGON_PATH}/SimplygonRedistUE4/Dependencies/Common/Engine/Source/ThirdParty/NotForLicensees")
	SET(SIMPLYGON_BINARIES_PATH "${SIMPLYGON_PATH}/SimplygonRedistUE4/Dependencies/Common/Engine/Binaries/ThirdParty/NotForLicensees/Simplygon")

	SET(SIMPLYGON_SPL_LIB "${SIMPLYGON_SDK_PATH}/SPL/lib/win64/vs2015/SPL.lib")
	SET(SIMPLYGON_SSF_LIB "${SIMPLYGON_SDK_PATH}/SSF/lib/win64/vs2015/SSF.lib")
ELSE()
	SET(SIMPLYGON_PATH)
	SET(SIMPLYGON_SDK_PATH)
	SET(SIMPLYGON_BINARIES_PATH)

	SET(SIMPLYGON_SPL_LIB "")
	SET(SIMPLYGON_SSF_LIB "")
ENDIF()


INCLUDE_DIRECTORIES(
	# 3rd party includes.
	"${ENGINE_3RDPARTY_PATH}/assimp/include"

	"${SIMPLYGON_SDK_PATH}/Simplygon/Simplygon-latest/Inc"
	"${SIMPLYGON_SDK_PATH}/SPL/Public/Include"
	"${SIMPLYGON_SDK_PATH}/SSF/Public"
)

ADD_ENGINE_LIBRARY(app_common ${SOURCES_PUBLIC})
SET_TARGET_PROPERTIES(app_common PROPERTIES FOLDER Apps)
TARGET_LINK_LIBRARIES(app_common core graphics image imgui plugin assimp ${SIMPLYGON_SPL_LIB} ${SIMPLYGON_SSF_LIB})

IF(ENABLE_SIMPLYGON)
	# Copy Simplygon runtime.
	ADD_CUSTOM_COMMAND(
        	TARGET app_common POST_BUILD
        	COMMAND ${CMAKE_COMMAND} -E copy
                	${SIMPLYGON_BINARIES_PATH}/SimplygonSDKRuntimeReleasex64.dll
                	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/SimplygonSDKRuntimeReleasex64.dll)
ENDIF()
