idf_component_register(
    SRCS "main.c" "fc_sound.c"
         "fortune/fortune_calendar.c" "fortune/fortune_misc.c" "fortune/fortune_qimen.c" "fortune/fortune_tables.c"
         "fortune/fortune_ziwei.c" "fortune/fortune_text.c" "fortune/fortune_names.c"
         "fortune/gfx/fc_gfx.c" "fortune/gfx/fc_res.c"
         "fortune/ui/fc_ui.c" "fortune/ui/scenes.c" "fortune/ui/scene_boot_menu.c" "fortune/ui/scene_birth.c" "fortune/ui/scene_clock.c"
         "fortune/ui/scene_lot.c" "fortune/ui/scene_shake.c" "fortune/ui/scene_tarot_daily.c" "fortune/ui/scene_bone_star.c"
         "fortune/ui/scene_ziwei_qimen.c"
    INCLUDE_DIRS "." "fortune" "fortune/gfx"
    REQUIRES bsp esp_lcd esp_timer nvs_flash esp_partition spi_flash esp_driver_usb_serial_jtag
)
set_source_files_properties(fortune/gfx/fc_gfx.c fortune/gfx/fc_res.c fc_sound.c PROPERTIES COMPILE_OPTIONS "-O2")
