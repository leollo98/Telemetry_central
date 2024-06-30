const char ALARM_HTML[] = R"rawliteral(
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
    <h3>Alarmes:</h3>
    <form action="/alarme">
        <div>hora (0-23): <input type="text" name="hora" value="%HORA%"></div>
        <div>minuto (0-59): <input type="text" name="minuto" value="%MINUTO%"></div>
        <div>tempo fade in (0-59): <input type="text" name="fade" value="%FADE%"></div>
        <div>luzes no maximo (0-59): <input type="text" name="max" value="%MAX%"></div>
        <input type="submit" value="Submit">
    </form>
    <p>date: %DATA%</p>
    <p>Potencia atual: %POTENCIA%</p>
    <p>Vezes atual: %VEZES%</p>
    <p>duração atual: %DURACAO%</p>
</body>
</html>
)rawliteral";