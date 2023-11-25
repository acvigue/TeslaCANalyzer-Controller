idf_component_register(
    SRCS "radar/radar_task.cpp" "interface/interface_task.cpp" "ble/helper_ble.cpp" "main.cpp" 
    INCLUDE_DIRS "." "ble" "interface" "radar"
    REQUIRES bt esp_wifi nvs_flash nimble_central_utils
)

idf_build_set_property(COMPILE_OPTIONS "-Wno-error=parentheses" APPEND)