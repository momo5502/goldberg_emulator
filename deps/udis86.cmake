FetchContent_Declare(
    udis86
    GIT_REPOSITORY https://github.com/vmt/udis86.git
    GIT_TAG 56ff6c87c11de0ffa725b14339004820556e343d
)

FetchContent_MakeAvailable(udis86)

message("blub ${udis86_SOURCE_DIR}")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${udis86_SOURCE_DIR}/libudis86/*.c
    ${CMAKE_CURRENT_LIST_DIR}/extra/udis86/libudis86/*.c
)

add_library(udis86 ${SRC_FILES})

target_include_directories(udis86 INTERFACE "${udis86_SOURCE_DIR}")
target_include_directories(udis86 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extra/udis86")

target_include_directories(udis86 PUBLIC "${udis86_SOURCE_DIR}/libudis86")
target_include_directories(udis86 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/extra/udis86/libudis86")
