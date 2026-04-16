idf_component_register(
    SRCS "controller.c"
    INCLUDE_DIRS "."
    REQUIRES
        bluepad32_component
        imu
)