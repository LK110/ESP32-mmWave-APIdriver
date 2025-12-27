
idf_component_register(SRCS "src/mr24hpc.c" "src/mr24hpc_parser.c"
                       REQUIRES "mr24hpc_uart"
                       PRIV_REQUIRES "esp_timer"
                       INCLUDE_DIRS "include")
