set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
idf_component_register(SRCS 
                            "animation_canvas.c"
                            "animation_walker.c"
                            "animation_scroller.c"
                            "animation_sparkles.c"
                            "animation_lines.c"
                            "animation_oscillator.c"
                            "animation_snakes.c"
                            "animation_game_of_life.c"
                       INCLUDE_DIRS "."
                       REQUIRES "main")
