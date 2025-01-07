const char PROMETHEUS_HTML[] = R"rawliteral(
# temperatura_quarto temperatura medida pelo aht
# TYPE temperatura_quarto gauge
temperatura_quarto %TEMP_QUARTO%
# temperatura_caixa temperatura medida pelo bmp
# TYPE temperatura_caixa gauge
temperatura_caixa %TEMP_CAIXA%
# temperatura_esp32 temperatura medida pelo esp
# TYPE temperatura_esp32 gauge
temperatura_caixa %TEMP_ESP%
# pressao_atm pressao atmosferica em Pascal
# TYPE pressao_atm gauge
pressao_atm %PRESSAO%
# luz_sup luz medida em cima da caixa
# TYPE luz_sup gauge
luz_sup %LUZ1%
# luz_lat luz medida do lado da caixa
# TYPE luz_lat gauge
luz_lat %LUZ2%
# co2 co2 do quarto
# TYPE co2 gauge
co2 %CO2%
# humidade humidade do quarto
# TYPE humidade gauge
humidade %HUMIDADE%
)rawliteral";