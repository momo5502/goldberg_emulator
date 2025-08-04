file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
  *.cpp
  *.hpp
)

add_library(hooking ${SRC_FILES})

if (MSVC)
  target_link_libraries(hooking PUBLIC
    minhook
    asmjit
	  udis86
  )
endif()

target_include_directories(hooking INTERFACE "${CMAKE_CURRENT_LIST_DIR}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(hooking PUBLIC Threads::Threads)
