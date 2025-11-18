set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
idf_component_register(SRCS 
														"main.c"
                            "usr_commands.c"
														"led_strip_encoder.c"
														"addr_led_driver.c"
                            "animation_manager.c"
                            "editable_value.c"
                            "ble_manager.c"
                            "control_signals.c"
                            "logger.c"
                            "visual_util.c"
                            "colorspace-conversions.c"
                            "colorspace_constructors.c"
                            "colorspace_interface.c"
                            "colorspace_body.c"
                       INCLUDE_DIRS ".")

