cmake_minimum_required(VERSION 3.13)
project(greenhouse_project)

message(STATUS "PICO_SDK_PATH: $ENV{PICO_SDK_PATH}")
message(STATUS "SDK import file exists: ${PICO_SDK_PATH}")

add_executable(greenhouse_project
        main.cpp
        critical_section.cpp
        Fmutex.cpp
        Fmutex.h

                ipstack/IPStack.cpp
                ipstack/IPStack.h
        ipstack/lwipopts.h
        ipstack/tls_common.c
        ipstack/picow_tls_client.c
        ipstack/lwipopts_tls.h

        i2c/PicoI2C.cpp
        i2c/PicoI2C.h

        uart/PicoOsUart.cpp
        uart/PicoOsUart.h

        modbus/nanomodbus.h
        modbus/nanomodbus.c
        modbus/ModbusRegister.cpp
        modbus/ModbusRegister.h
        modbus/ModbusClient.cpp
        modbus/ModbusClient.h

        display/framebuf.cpp
        display/framebuf.h
        display/mono_vlsb.cpp
        display/mono_vlsb.h
        #        display/ssd1306.cpp
        #        display/ssd1306.h
        display/ssd1306os.h
        display/ssd1306os.cpp
        main.cpp
        SensorTask.cpp
        SensorTask.h
        UITask.h
        UITask.cpp
        pressure_sensor/PressureSensor.cpp
        pressure_sensor/PressureSensor.h
        inputs/InputHandler.cpp
        inputs/InputHandler.h
        network/remote.cpp
        network/remote.h
        structs.h
        eeprom/eeprom.cpp
        eeprom/eeprom.h
        control/ControlTask.cpp
        control/ControlTask.h
)

target_include_directories(greenhouse_project PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        modbus
        uart
        i2c
        display
        ipstack
)

target_compile_definitions(greenhouse_project PRIVATE
        WIFI_SSID="$ENV{WIFI_SSID}"
        WIFI_PASSWORD="$ENV{WIFI_PASSWORD}"
        NO_SYS=0
        PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE=CYW43_COUNTRY_FINLAND
)

target_link_libraries(greenhouse_project
        pico_stdlib
        hardware_i2c
        FreeRTOS-Kernel-Heap4
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_mbedtls
        pico_mbedtls
)

add_compile_definitions(PICO_CYW43_LOGGING_ENABLED=0)

# Ignore warnings from lwip code
set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
)

pico_add_extra_outputs(greenhouse_project)

# Disable usb output, enable uart output
pico_enable_stdio_usb(greenhouse_project 0)
pico_enable_stdio_uart(greenhouse_project 1)
