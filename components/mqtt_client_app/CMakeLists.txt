
idf_component_register(SRCS "src/mqtt_app.c" "src/mqtt_event.c" "src/mqtt_publish.c" "src/mqtt_watchdog.c" "src/mqtt_json.c" "src/mqtt_utils.c"
                       REQUIRES "mqtt" "mr24hpc" "json"
                       INCLUDE_DIRS "include" "private_include")
