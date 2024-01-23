cmake_minimum_required(VERSION 3.20)

#set project name
project(CodeGenerator)

#set c++ standard
set(CMAKE_CXX_STANDARD 17)

#add variables for source files
set(SOURCE_FILES
        src/XmlParser.cpp
        src/FileGenerator.cpp
        src/FileParser.cpp
        src/SimpleOptionParser.cpp
        main.cpp
)

#add variables for header files
set(HEADER_FILES
        include/XmlParser.hpp
        include/FileGenerator.hpp
        include/FileParser.hpp
        include/Objects.hpp
        include/SimpleOptionParser.hpp
)

#add variable for all files
set(ALL_FILES ${SOURCE_FILES} ${HEADER_FILES})

add_executable(CodeGenerator ${ALL_FILES})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/External/pugixml)

#set CMAKE_OBJECT_PATH_MAX to 350
set(CMAKE_OBJECT_PATH_MAX 350)

#enable clang as a project
set(LLVM_ENABLE_PROJECTS "clang" CACHE STRING "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/External/LLVM/llvm-project/llvm)
#add_subdirectory(${LLVM_EXTERNAL_CLANG_SOURCE_DIR})

#add llvm libraries to project
target_link_libraries(CodeGenerator PRIVATE clangTooling pugixml)

#add include folder
target_include_directories(CodeGenerator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(CodeGenerator PRIVATE
        ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/External/LLVM/llvm-project/llvm/tools/clang/include
        ${CMAKE_CURRENT_BINARY_DIR}/External/LLVM/llvm-project/llvm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/External/LLVM/llvm-project/llvm/include)

#add dll to output folder
#check if not using visual studio
#if (WIN32 AND NOT MSVC)
#    add_custom_command(TARGET CodeGenerator POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            "${bin_DIR}/libclang.dll"
#            $<TARGET_FILE_DIR:CodeGenerator>)
#    add_custom_command(TARGET CodeGenerator POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            "${bin_DIR}/libgcc_s_seh-1.dll"
#            $<TARGET_FILE_DIR:CodeGenerator>)
#    add_custom_command(TARGET CodeGenerator POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            "${bin_DIR}/libwinpthread-1.dll"
#            $<TARGET_FILE_DIR:CodeGenerator>)
#    add_custom_command(TARGET CodeGenerator POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            "${bin_DIR}/libstdc++-6.dll"
#            $<TARGET_FILE_DIR:CodeGenerator>)
#endif ()