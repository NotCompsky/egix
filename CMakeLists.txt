cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR) # CONTRIBUTIONS WELCOME: Tests of previous/future versions which work or not


set(EXIG_MAJOR_VERSION 0)
set(EXIG_MINOR_VERSION 4)
set(EXIG_PATCH_VERSION 0)
set(EXIG_VERSION ${EXIG_MAJOR_VERSION}.${EXIG_MINOR_VERSION}.${EXIG_PATCH_VERSION})


option(ENABLE_STATIC "Build static, rather than shared, library" OFF)
option(BUILD_PROGRAM "Build GUI program, rather than just the library" ON)


project(Egix CXX) # WARNING: Sets some important variables about the plarform. Don't call find_package before setting a project name.

find_package(Boost REQUIRED COMPONENTS regex)
find_package(Qt5   REQUIRED COMPONENTS Widgets)


set(CMAKE_AUTOMOC ON)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fsanitize=undefined -Wall -Wextra -Wshadow -Wstrict-aliasing -Wfloat-equal -Wpointer-arith -Wwrite-strings -Wformat=2 -Wstrict-overflow -Wunreachable-code")
	endif()
else()
	set(MY_DEFINITIONS "QT_NO_DEBUG" "QT_NO_DEBUG_OUTPUT")
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# libcompsky uses a lot of 'flag' parameters
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
endif()

include(GNUInstallDirs) # To define CMAKE_INSTALL_*DIR


set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(LIB_SRCS
	"${SRC_DIR}/editor.cpp"
	"${SRC_DIR}/highlighter.cpp"
	"${SRC_DIR}/name_dialog.cpp"
	"${SRC_DIR}/msgbox.cpp"
	"${SRC_DIR}/sql_name_dialog.cpp"
	"${SRC_DIR}/regopt.cpp"
	"${SRC_DIR}/3rdparty/codeeditor.cpp"
)

if(ENABLE_STATIC)
	add_library(egix STATIC ${LIB_SRCS})
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
else()
	add_library(egix SHARED ${LIB_SRCS})
endif()
set(TARGETS egix)

target_include_directories(egix PRIVATE ${Qt5Core_INCLUDE_DIRS})
target_include_directories(egix PUBLIC "${INC_DIR}")
target_link_libraries(egix Qt5::Widgets "${Boost_REGEX_LIBRARY}")
set_property(TARGET egix PROPERTY CXX_STANDARD 17)


#set_target_properties(egix PROPERTIES IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libegix.so")

if(BUILD_PROGRAM)
	add_executable(egixr "${SRC_DIR}/main.cpp")
	target_link_libraries(egixr egix Qt5::Widgets)
	list(APPEND TARGETS egixr)
	if(ENABLE_STATIC)
		target_link_libraries(egixr -static)
	endif()
endif()


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/ExigConfigVersion.cmake"
    VERSION "${EXIG_VERSION}"
    COMPATIBILITY SameMinorVersion
)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/ExigConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/ExigConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Exig"
)
install(FILES "${PROJECT_BINARY_DIR}/ExigConfigVersion.cmake"
              "${PROJECT_BINARY_DIR}/ExigConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Exig")


install(
	TARGETS ${TARGETS}
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
