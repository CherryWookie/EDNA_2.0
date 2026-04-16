idf_component_register(
    SRCS "wifi_server.c" "telemetry.c"
    INCLUDE_DIRS "."
    REQUIRES
        flight_controller
        esp_wifi
        esp_event
        esp_netif
        lwip
        freertos
        main
)
