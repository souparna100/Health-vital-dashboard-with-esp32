#include <WiFi.h>
#include <WebServer.h>

#define PULSE_PIN 27

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

volatile unsigned long lastBeat = 0;
volatile int bpm = 0;

String webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Heart Rate Monitor</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
<h2>Heart Rate Monitor</h2>
<h3>BPM: <span id="bpm">0</span></h3>

<canvas id="chart"></canvas>

<script>
let data = [];
let labels = [];

const ctx = document.getElementById('chart').getContext('2d');
const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: labels,
    datasets: [{
      label: 'BPM',
      data: data,
      borderColor: 'red',
      fill: false
    }]
  }
});

setInterval(() => {
  fetch('/bpm')
    .then(res => res.text())
    .then(val => {
      document.getElementById('bpm').innerHTML = val;
      labels.push('');
      data.push(val);
      if (data.length > 30) {
        data.shift();
        labels.shift();
      }
      chart.update();
    });
}, 1000);
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleBPM() {
  server.send(200, "text/plain", String(bpm));
}

void IRAM_ATTR beatDetected() {
  unsigned long now = millis();
  if (now - lastBeat > 300) {  // debounce (max ~200 BPM)
    bpm = 60000 / (now - lastBeat);
    lastBeat = now;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PULSE_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), beatDetected, RISING);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/bpm", handleBPM);
  server.begin();
}

void loop() {
  server.handleClient();
}
