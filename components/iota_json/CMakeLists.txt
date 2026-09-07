idf_component_register(
  SRCS "iota_json.c"
  INCLUDE_DIRS "include"
  EMBED_TXTFILES "certs/rootCA.pem"
  PRIV_REQUIRES esp_wifi esp_netif esp_event
)
