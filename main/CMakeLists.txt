idf_component_register(
    SRCS "main.cpp"
         "src/module_gpio.cpp"
         "src/module_mqtt.cpp"
         "src/module_timer.cpp"
         "src/module_wifi_provisioning.cpp"
    INCLUDE_DIRS "." "include"
)
