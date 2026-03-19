set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
idf_component_register(SRCS 
														"main.c" 
														"led_strip_encoder.c"
														"addr_led_driver.c"
                            "animation_manager.c"
                            "editable_value.c"
                            "animation_canvas.c"
                            "logger.c"
                            "visual_util.c"
                            "colorspace-conversions.c"
                            "colorspace_constructors.c"
                            "colorspace_interface.c"
                            "colorspace_body.c"
                       INCLUDE_DIRS ".")

