zephyr_syscall_header(${ZEPHYR_LIBOSTENTUS_MODULE_DIR}/include/libostentus.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RC522 rc522.c)
