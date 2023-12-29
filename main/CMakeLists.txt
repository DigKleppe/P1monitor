set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")
#set(COMPONENT_PRIV_REQUIRES  "spiffs nvs_flash driver wifiConnect klp  http udp" )
register_component()
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)


