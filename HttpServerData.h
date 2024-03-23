const char index_html_top[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Growatt2mqtt Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
  </style>
</head>
<body>
)rawliteral";


const char index_html_bottom[] PROGMEM = R"rawliteral(
<script>
function getData (element,data) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById(element).innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "request?"+data+"=", true);
  xhttp.send();
}
 
 setInterval(function() 
{
  //getData('outputVoltage','voltage');
  //getData('outputCurrent','outputCurrent');
  //getData('frequency','frequency');
  //getData('outputPowerFactor','outputPowerfactor');
  //getData('outputPower','outputPower');
  //getData('outputPwm','outputPwm');
  //getData('solarOutputPower','solarOutputPower');
  //getData('solarPvPower','solarPvPower');
  //getData('solarBatteryPower','solarBatteryPower');
  //getData('solarBatterySoc','solarBatterySoc');
  //getData('systemMode','systemMode');
  getData('wifiRSSI','wifiRSSI');
}, 2000); 
</script>
</body>
</html>
)rawliteral"; 