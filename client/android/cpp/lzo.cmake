# Lzo's own cmake is rather throublesome

set(lzo_srcs
  lzo1.c
  lzo1_99.c
  lzo1a.c
  lzo1a_99.c
  lzo1b_1.c
  lzo1b_2.c
  lzo1b_3.c
  lzo1b_4.c
  lzo1b_5.c
  lzo1b_6.c
  lzo1b_7.c
  lzo1b_8.c
  lzo1b_9.c
  lzo1b_99.c
  lzo1b_9x.c
  lzo1b_cc.c
  lzo1b_d1.c
  lzo1b_d2.c
  lzo1b_rr.c
  lzo1b_xx.c
  lzo1c_1.c
  lzo1c_2.c
  lzo1c_3.c
  lzo1c_4.c
  lzo1c_5.c
  lzo1c_6.c
  lzo1c_7.c
  lzo1c_8.c
  lzo1c_9.c
  lzo1c_99.c
  lzo1c_9x.c
  lzo1c_cc.c
  lzo1c_d1.c
  lzo1c_d2.c
  lzo1c_rr.c
  lzo1c_xx.c
  lzo1f_1.c
  lzo1f_9x.c
  lzo1f_d1.c
  lzo1f_d2.c
  lzo1x_1.c
  lzo1x_1k.c
  lzo1x_1l.c
  lzo1x_1o.c
  lzo1x_9x.c
  lzo1x_d1.c
  lzo1x_d2.c
  lzo1x_d3.c
  lzo1x_o.c
  lzo1y_1.c
  lzo1y_9x.c
  lzo1y_d1.c
  lzo1y_d2.c
  lzo1y_d3.c
  lzo1y_o.c
  lzo1z_9x.c
  lzo1z_d1.c
  lzo1z_d2.c
  lzo1z_d3.c
  lzo2a_9x.c
  lzo2a_d1.c
  lzo2a_d2.c
  lzo_crc.c
  lzo_init.c
  lzo_ptr.c
  lzo_str.c
  lzo_util.c
  )

PREPEND(lzo_src_with_path "lzo/src" ${lzo_srcs})
add_library(lzo ${lzo_src_with_path})
target_include_directories(lzo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lzo/include")
if (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    target_compile_options(lzo PRIVATE -O0)
endif()