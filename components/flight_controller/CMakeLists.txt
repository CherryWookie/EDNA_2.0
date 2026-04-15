idf_component_register(
    SRCS "pid.c" "motor_mix.c" "flight_controller.c"
    INCLUDE_DIRS "."
    REQUIRES esc esp_timer freertos main
)
