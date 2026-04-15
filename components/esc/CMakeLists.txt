idf_component_register(
    SRCS "esc.c" "esc_calibration.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos main
)
