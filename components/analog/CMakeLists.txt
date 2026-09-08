idf_component_register(
  SRCS "analog.c"
  INCLUDE_DIRS "include"
  REQUIRES esp_event 
  PRIV_REQUIRES esp_adc
)
