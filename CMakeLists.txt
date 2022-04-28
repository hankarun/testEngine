cmake_minimum_required(VERSION 3.14)

project(testWeb)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/external/bgfx/include
    ${CMAKE_SOURCE_DIR}/external/SDL2-2.0.20/include
    ${CMAKE_SOURCE_DIR}/external/glm-master
)

ADD_LIBRARY(bgfx STATIC IMPORTED)
SET_TARGET_PROPERTIES(bgfx PROPERTIES IMPORTED_LOCATION_RELEASE ${CMAKE_SOURCE_DIR}/external/bgfx/lib/bgfxRelease.lib)
SET_TARGET_PROPERTIES(bgfx PROPERTIES IMPORTED_LOCATION_DEBUG ${CMAKE_SOURCE_DIR}/external/bgfx/lib/bgfxDebug.lib)

ADD_LIBRARY(bx STATIC IMPORTED)
SET_TARGET_PROPERTIES(bx PROPERTIES IMPORTED_LOCATION_RELEASE ${CMAKE_SOURCE_DIR}/external/bgfx/lib/bxRelease.lib)
SET_TARGET_PROPERTIES(bx PROPERTIES IMPORTED_LOCATION_DEBUG ${CMAKE_SOURCE_DIR}/external/bgfx/lib/bxDebug.lib)

ADD_LIBRARY(bimg STATIC IMPORTED)
SET_TARGET_PROPERTIES(bimg PROPERTIES IMPORTED_LOCATION_RELEASE ${CMAKE_SOURCE_DIR}/external/bgfx/lib/bimgRelease.lib)
SET_TARGET_PROPERTIES(bimg PROPERTIES IMPORTED_LOCATION_DEBUG ${CMAKE_SOURCE_DIR}/external/bgfx/lib/bimgDebug.lib)

ADD_LIBRARY(sdl2 SHARED IMPORTED)
SET_TARGET_PROPERTIES(sdl2 PROPERTIES IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}/external/SDL2-2.0.20/lib/x64/SDL2.lib)

ADD_LIBRARY(sdl2main SHARED IMPORTED)
SET_TARGET_PROPERTIES(sdl2main PROPERTIES IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}/external/SDL2-2.0.20/lib/x64/SDL2main.lib)

target_link_libraries(${PROJECT_NAME} bgfx bx bimg sdl2 sdl2main)

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

set(SHADER_COMPILER ${CMAKE_SOURCE_DIR}/tools/shaderc.exe)

file(GLOB VS_SHADERS
  ${SHADER_SOURCE_DIR}/v_*.sc)

file(GLOB FS_SHADERS
  ${SHADER_SOURCE_DIR}/f_*.sc)

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
  OUTPUT ${SHADER_BINARY_DIR}
  COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(source IN LISTS VS_SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${SHADER_COMPILER}
      -f ${source}
      -o ${SHADER_BINARY_DIR}/${FILENAME}.bin
      ${source}
      --platform windows --type vertex --verbose -i ./ -p vs_5_0
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.bin
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND VS_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.bin)
endforeach()

foreach(source IN LISTS FS_SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${SHADER_COMPILER}
      -f ${source}
      -o ${SHADER_BINARY_DIR}/${FILENAME}.bin
      ${source}
      --platform windows --type fragment --verbose -i ./ -p ps_5_0
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.bin
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND FS_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.bin)
endforeach()

add_custom_target(shaders ALL DEPENDS ${VS_SHADERS} ${FS_SHADERS})