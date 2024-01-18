##############Basic Project Setup##########################
cmake_minimum_required(VERSION 3.23.0)
set(CMAKE_C_STANDARD 99)
set(PROJECT_NAME zenzizenzizenzic_ncurses)
set(PROJECT_VENDOR Kestrel Gregorich-Trevor)
set(PROJECT_DESCRIPTION "A roguelike about fighting games.")
set(ZZZZZZ_DESKTOP_DIR /usr/share/applications)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
SET(VERSION_PATCH 0)
SET(RELEASE_STATE "alpha")

# Generate files
configure_file (
    "${CMAKE_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_SOURCE_DIR}/include/version.h"
)
configure_file (
    "${CMAKE_SOURCE_DIR}/share/zenzizenzizenzic.desktop.in"
    "${CMAKE_SOURCE_DIR}/share/zenzizenzizenzic.desktop"
)

# Vcpkg setup
set(VCPKG_BOOTSTRAP_OPTIONS -disableMetrics)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

# Project definition
project(${PROJECT_NAME} VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} LANGUAGES C) 
##########################Compile##########################
# Find required packages
set(CURSES_NEED_WIDE TRUE)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
find_package(cJSON REQUIRED)

# Source files
set (SOURCES
    src/action.c
    src/actor.c
    src/ai.c
    src/combat.c
    src/fov.c
    src/gameover.c
    src/invent.c
    src/main.c
    src/map.c
    src/mapgen.c
    src/message.c
    src/parser.c
    src/pqueue.c
    src/random.c
    src/register.c
    src/render.c
    src/save.c
    src/spawn.c
    src/tile.c
    windows/curses/windows.c
    windows/curses/menu.c)
set (HEADERS
    include/action.h
    include/actor.h
    include/ai.h
    include/color.h
    include/combat.h
    include/fov.h
    include/gameover.h
    include/invent.h
    include/map.h
    include/mapgen.h
    include/menu.h
    include/message.h
    include/parser.h
    include/pqueue.h
    include/random.h
    include/register.h
    include/render.h
    include/save.h
    include/spawn.h
    include/tactics.h
    include/tile.h
    include/version.h
    include/wfc.h
    include/windows.h)
set (INCLUDE_DIR include)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR} ${CJSON_INCLUDE_DIR} ${CURSES_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CJSON_LIBRARIES} -lpanelw ${CURSES_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Copy data files
# See https://stackoverflow.com/questions/13429656/how-to-copy-contents-of-a-directory-into-build-directory-after-make-with-cmake
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
                    COMMENT "Copying data folder to output dir.")

message(STATUS "Configuring CMake for ${PROJECT_NAME} v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${RELEASE_STATE}")
message(STATUS "Release Type: ${CMAKE_BUILD_TYPE}")
##########################Install##########################
install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/ DESTINATION data)
install(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
install(FILES share/zenzizenzizenzic.desktop DESTINATION ${ZZZZZZ_DESKTOP_DIR})
############################CPack##########################
#CPack
include(InstallRequiredSystemLibraries)
set(CPACK_VERBATIM_VARIABLES true)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_VENDOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_EXECUTABLES "zenzizenzizenzic_ncurses;ZZZZZZ")
include(CPack)
