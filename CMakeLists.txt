# CMakeLists for zephyr

target_sources_ifdef(CONFIG_ZEPHYR_HDL app PRIVATE hdl.c)