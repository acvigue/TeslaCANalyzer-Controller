file(GLOB_RECURSE NESTED_SRC
       **.cpp
)

idf_component_register(
    SRCS ${NESTED_SRC}
    INCLUDE_DIRS "." "ble" "interface" "radar" "wifi" "provisioning"
    REQUIRES bt esp_wifi nvs_flash nimble_central_utils wifi_provisioning
)

idf_build_set_property(COMPILE_OPTIONS "-Wno-error=parentheses" APPEND)