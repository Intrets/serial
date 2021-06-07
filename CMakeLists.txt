include("${PROJECT_SOURCE_DIR}/cmake/MakeModule.cmake")

set(MODULE_NAME serial)

set(MODULE_FILES
	Serializer
)

make_module(${MODULE_NAME} "${MODULE_FILES}")

target_link_libraries(${MODULE_NAME} PRIVATE
	WGLM
	tepp
)
