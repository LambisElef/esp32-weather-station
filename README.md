# ESP32 Weather Station combined with InfluxDB

## Description
TBA

## InfluxDB installation, DB and SSL certificate creation
TBA

## Grafana installation
TBA

## Notes
This is an ESP IDF v4 project. It will not work as is, as some code configurations are needed:
<pre>
- /main/influxdb.pem  Generate the SSL certificate and place it into this path to be included into the binary.
- /main/wifi.h        Configure the defined **WIFI_SSID** and **WIFI_PASS**.
- /main/http.h        Configure the defined **HTTP_POST_URL**.
</pre>
The project is divided into 3 main code modules:
- `bme` which handles the communication with the BME280 sensor.
- `http` which handles the data transmission from the ESP32 to the InfluxDB.
- `wifi` which handles connecting to a WiFi AP and maintains that connection.

## Special Thanks
