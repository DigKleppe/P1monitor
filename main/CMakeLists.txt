set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")
register_component()
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)


