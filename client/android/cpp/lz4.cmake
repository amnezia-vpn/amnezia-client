set(lz4_srcs
  lz4.c
  )

PREPEND(lz4_src_with_path "lz4/lib/" ${lz4_srcs})
add_library(lz4 ${lz4_src_with_path})
target_include_directories(lz4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lz4/lib")
