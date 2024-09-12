cmake_minimum_required(VERSION 3.29)
project(intro-openmp)

option(INSOURCEBUILD OFF) # Prevent in-source style builds
if((${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR}) AND NOT
                                                              ${INSOURCEBUILD})
  message(
    FATAL_ERROR
      "In-source building disabled. Provide -DINSOURCEBUILD option if you are sure about it."
  )
endif()

option(SANITIZE OFF)
if(${SANITIZE})
  add_compile_options(-fsanitize=undefined,address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=undefined,address -fno-omit-frame-pointer)
endif()

option(THREAD_SANITIZE OFF)
if(${THREAD_SANITIZE})
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread -fno-omit-frame-pointer)
endif()

if(THREAD_SANITIZE AND SANITIZE)
  message(FATAL_ERROR "Thread and address sanitizer can't be used together")
endif()

include(../../cmake/functions.cmake)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(OpenMP REQUIRED COMPONENTS CXX)

function(easy_add_executable target_name)
  add_executable(${target_name} src/${target_name}.cc)
  target_enable_linter(${target_name})
  target_compile_features(${target_name} PUBLIC cxx_std_23)
  enable_warnings(${target_name})
  target_link_libraries(${target_name} PRIVATE OpenMP::OpenMP_CXX
                                               Boost::program_options)
endfunction()

easy_add_executable(hello-openmp)

execute_process(
  COMMAND
    ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json)
