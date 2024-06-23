cmake_minimum_required(VERSION 3.20.0)
project(tiny_c2 
  LANGUAGES CXX C
  VERSION 0.1
  DESCRIPTION "A tiny c2 framework written in C, targetting Linux and Windows"
)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(server)


set(MYPROJECT_VERSION "unknown")
find_program(GIT git)
if (NOT GIT_FOUND)
	execute_process(COMMAND ${GIT} describe --always
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE GIT_DESCRIBE_RESULT
	)
	if (GIT_DESCRIBE_RESULT EQUAL "0")
		set(MYPROJECT_VERSION ${GIT_DESCRIBE_OUTPUT})
	else()
		message(WARNING "git describe returned ${GIT_DESCRIBE_RESULT}")
	endif()
else()
	message(WARNING "git not found")
endif()
target_compile_definitions(server INTERFACE
	MYPROJECT_VERSION="${MYPROJECT_VERSION}"
)
message("myproject version: ${MYPROJECT_VERSION}")

