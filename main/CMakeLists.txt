idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        imu
        esc
        flight_controller
        controller
        telemetry
        nvs_flash
        esp_wifi
        freertos
        esp_timer
        bluepad32_component
        btstack
)