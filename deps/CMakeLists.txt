if (MSVC)

set(ASMJIT_STATIC ON)
set(ASMJIT_NO_AARCH64 ON)
set(ASMJIT_NO_FOREIGN ON)
set(ASMJIT_NO_CUSTOM_FLAGS ON)

FetchContent_Declare(
    asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG a3199e8857792cd10b7589ff5d58343d2c9008ea
)

FetchContent_MakeAvailable(asmjit)

##################################

FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG 565968b28583221751cc2810e09ea621745fc3a3
)

FetchContent_MakeAvailable(minhook)

##################################

include(udis86.cmake)

endif()
