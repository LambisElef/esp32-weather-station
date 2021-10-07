# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c" "bme.c" "http.c" "wifi.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_EMBED_TXTFILES "influxdb.pem")

register_component()
