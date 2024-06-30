const char LED_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" charset="utf-8" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Sensors Control</title>
    <style>
        html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }
        body { margin-top: 50px; }
        h1 { color: #444444; margin: 50px auto 30px; }
        h3 { color: #444444; margin-bottom: 50px; }
        .button { display: block; width: 80px; background-color: #3498db; border: none; color: white; padding: 16px 32px; text-decoration: none; font-size: 100px; margin: 0px auto 35px; cursor: pointer; border-radius: 4px; }
        .button-on { background-color: #3498db; }
        .button-on:active { background-color: #2980b9; }
        .button-off { background-color: #34495e; }
        .button-off:active { background-color: #2c3e50; }
        p { font-size: 14px; color: #888; margin-bottom: 10px; }
    </style>
</head>
<body>
    <h1>ESP32 Web Server</h1>
    <h3>LED:</h3>
    <p><a href="/led?F%LED_F_STATE%=1"><button class="button-%LED_F_BUTTON%">%LED_F_LABEL%</button></a></p>
    <p><a href="/led?O%LED_O_STATE%=1"><button class="button-%LED_O_BUTTON%">%LED_O_LABEL%</button></a></p>
    <form action="/led">
        Hue (0-360): <input type="text" name="hue" value="%HUE%">
        Saturação (0-100): <input type="text" name="sat" value="%SAT%">
        Intensidade (0-100): <input type="text" name="int" value="%INT%">
        <input type="submit" value="Submit">
    </form>
    <p>date: %DATE%</p>
    <p>Potencia atual: %POTENCIA_ATUAL%</p>
    <p>Vezes atual: %VEZES_ATUAL%</p>
    <p>duração atual: %DURACAO_ATUAL%</p>
</body>
</html>
)rawliteral";