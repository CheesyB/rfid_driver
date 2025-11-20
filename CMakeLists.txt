
zephyr_syscall_header(${ZEPHYR_LIBOSTENTUS_MODULE_DIR}/include/rfid.h)

zephyr_library()
zephyr_include_directories(include)
zephyr_library_sources(rc522.c)



