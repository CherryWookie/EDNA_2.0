idf_component_register(
    SRCS "mpu6050.c" "kalman.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer freertos main
)
