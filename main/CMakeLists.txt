add_executable(main
        hc05.c
        main.c
)

target_link_libraries(main pico_stdlib hardware_adc freertos hardware_pwm)
pico_add_extra_outputs(main)
