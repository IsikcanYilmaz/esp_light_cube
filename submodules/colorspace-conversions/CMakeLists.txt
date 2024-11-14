set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
aux_source_directory(. colorspace)
add_library(colorspace ${colorspace})
target_include_directories(colorspace PRIVATE)
