# idf_component_register(
#     SRCS "controller.c"
#     INCLUDE_DIRS "."
#     REQUIRES bluepad32 freertos esp_log
# )
idf_component_register(
    SRCS "controller_input.c"
    INCLUDE_DIRS "."
    REQUIRES imu main
)