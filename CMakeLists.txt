cmake_minimum_required(VERSION 3.20)

include("./cmake/MakeModule.cmake")

set(MODULE_NAME serial)

project(${MODULE_NAME} LANGUAGES CXX)

make_module(
	MODULE_NAME ${MODULE_NAME}
	MODULE_FILES
		Serializer
		ByteSwap
	CXX_STANDARD 20
	REQUIRED_LIBS
		WGLM
		tepp
		mem
)
