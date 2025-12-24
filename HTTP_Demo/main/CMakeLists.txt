idf_component_register(SRCS "main.c" "oled/ssd1306.c" "oled/oled_integration.c"
                    INCLUDE_DIRS "." "oled"
                    PRIV_REQUIRES esp_https_server esp_wifi nvs_flash esp_eth esp_http_client json mbedtls esp_driver_i2c esp_driver_gpio
                    EMBED_TXTFILES "certs/servercert.pem"
                                   "certs/prvtkey.pem")
