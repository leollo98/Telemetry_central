const char BASE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" http-equiv="refresh" content="5" charset="utf-8" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Sensors Control</title>
    <style>
        html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }
        body { margin-top: 50px; }
        h1 { color: #444444; margin: 50px auto 30px; }
        h3 { color: #444444; margin-bottom: 50px; }
        .button { display: block; width: 80px; background-color: #3498db; border: none; color: white; padding: 13px 30px; text-decoration: none; font-size: 25px; margin: 0px auto 35px; cursor: pointer; border-radius: 4px; }
        .button-on { background-color: #3498db; }
        .button-on:active { background-color: #2980b9; }
        .button-off { background-color: #34495e; }
        .button-off:active { background-color: #2c3e50; }
        p { font-size: 14px; color: #888; margin-bottom: 10px; }
    </style>
</head>
<body>
    <h1>ESP32 Web Server</h1>
    <h3>Sensores:</h3>
    <h4>Temperatura quarto: </h4><h3>%TEMP_QUARTO%</h3>
    <h4>Temperatura caixa: </h4><h3>%TEMP_CAIXA%</h3>
    <h4>Temperatura ESP32: </h4><h3>%TEMP_ESP%</h3>
    <h4>Pressão: </h4><h3>%PRESSAO% Pa</h3>
    <h4>Luz1: </h4><h3>%LUZ1% Lux</h3>
    <h4>Luz2: </h4><h3>%LUZ2% Lux</h3>
    <h4>CO2: </h4><h3>%CO2% ppm</h3>
    <h4>Humidade: </h4><h3>%HUMIDADE%</h3>
</body>
</html>
)rawliteral";