#include <WiFiS3.h>

// Wi-Fi credentials
char ssid[] = "TP-Link_74B0";
char pass[] = "68705985";

WiFiServer server(80);

// Pin definitions
#define JOY_X A0
#define JOY_Y A1
#define JOY_BTN 2
#define MODE_BTN 7
#define BUZZER 8
#define LED_PIN 9

bool multiplayer = false;

// Player positions
float p1x = 160, p1y = 120;
float p2x = 100, p2y = 120;

// Player 2 velocity
int p2vx = 0, p2vy = 0;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, pass);

  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Connected!");
  Serial.print("Arduino IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
  Serial.println("🌐 Web server started.");

  // Pins
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);
  pinMode(JOY_BTN, INPUT_PULLUP);
  pinMode(MODE_BTN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Toggle mode button
  static unsigned long lastToggle = 0;
  if (digitalRead(MODE_BTN) == LOW && millis() - lastToggle > 400) {
    multiplayer = !multiplayer;
    Serial.print("Mode changed to: ");
    Serial.println(multiplayer ? "Multiplayer" : "Single Player");

    tone(BUZZER, multiplayer ? 900 : 600, 100);
    digitalWrite(LED_PIN, HIGH);
    delay(80);
    digitalWrite(LED_PIN, LOW);

    lastToggle = millis();
  }

  // Move player 2 if active
  if (multiplayer) {
    p2x += p2vx * 2;
    p2y += p2vy * 2;
    p2x = constrain(p2x, 10, 310);
    p2y = constrain(p2y, 10, 230);
  }

  // Handle HTTP
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.flush();
  Serial.println(req);

  // --- Player 1 joystick data ---
  if (req.indexOf("GET /data") >= 0) {
    int x = analogRead(JOY_X);
    int y = analogRead(JOY_Y);
    int btn = digitalRead(JOY_BTN);

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.print("{\"x\":"); client.print(x);
    client.print(",\"y\":"); client.print(y);
    client.print(",\"btn\":"); client.print(btn == LOW ? 1 : 0);
    client.print("}");
  }

  // --- Player 2 input handler ---
  else if (req.indexOf("GET /input") >= 0) {
    if (req.indexOf("UP") > 0)    { p2vy = -1; p2vx = 0; }
    if (req.indexOf("DOWN") > 0)  { p2vy = 1;  p2vx = 0; }
    if (req.indexOf("LEFT") > 0)  { p2vx = -1; p2vy = 0; }
    if (req.indexOf("RIGHT") > 0) { p2vx = 1;  p2vy = 0; }
    tone(BUZZER, 700, 50);

    client.println("HTTP/1.1 204 No Content");
    client.println("Connection: close");
    client.println();
  }

  // --- Player 2 position (for /screen) ---
  else if (req.indexOf("GET /p2") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println();
    client.print("{\"x\":");
    client.print(p2x);
    client.print(",\"y\":");
    client.print(p2y);
    client.print("}");
  }

  // --- Mode JSON endpoint ---
  else if (req.indexOf("GET /mode") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println();
    client.print("{\"multi\":");
    client.print(multiplayer ? "true" : "false");
    client.print("}");
  }

  // --- Game screen (phones) ---
  else if (req.indexOf("GET /screen") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println(R"html(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<style>
body{background:#000;margin:0;overflow:hidden;}
canvas{display:block;margin:0 auto;background:#111;}
h2{color:#0f0;text-align:center;font-family:sans-serif;}
</style>
</head>
<body>
<h2>🎮 Game Screen</h2>
<canvas id="game" width="320" height="240"></canvas>
<script>
let c = document.getElementById('game');
let ctx = c.getContext('2d');

// player positions
let p1 = {x:160, y:120};
let p2 = {x:100, y:120};

// last known data
let lastP2 = {x:100, y:120};

// fetch joystick + player2 together
async function update() {
  // Get joystick
  let j;
  try {
    let r = await fetch('/data',{cache:'no-store'});
    j = await r.json();
  } catch(e) {
    j = {x:512, y:512, btn:0};
  }

  // Update player 1
  p1.x += (j.x - 512) / 200;
  p1.y += (j.y - 512) / 200;
  p1.x = Math.max(10, Math.min(310, p1.x));
  p1.y = Math.max(10, Math.min(230, p1.y));

  // Get player 2 position every frame (keeps in sync)
  try {
    let r2 = await fetch('/p2',{cache:'no-store'});
    let j2 = await r2.json();
    lastP2.x = j2.x;
    lastP2.y = j2.y;
  } catch(e) {}

  // Draw everything cleanly each frame
  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, 320, 240);

  // Player 1 (green)
  ctx.fillStyle = '#0f0';
  ctx.beginPath();
  ctx.arc(p1.x, p1.y, 10, 0, Math.PI*2);
  ctx.fill();

  // Player 2 (blue)
  ctx.fillStyle = '#00f';
  ctx.beginPath();
  ctx.arc(lastP2.x, lastP2.y, 10, 0, Math.PI*2);
  ctx.fill();

  ctx.fillStyle = '#0f0';
  ctx.font = '12px monospace';
  ctx.fillText(`BTN:${j.btn?'Pressed':'Released'}`, 10, 20);
}

// Steady animation loop
setInterval(update, 100);
</script>

</body>
</html>
)html");
  }

  // --- Controller page ---
  else if (req.indexOf("GET /controller") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println(R"html(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<style>
body {background:#000;color:#0f0;text-align:center;font-family:sans-serif;}
button {
  font-size:32px;margin:10px;padding:20px 40px;
  background:#0f0;color:#000;border:none;border-radius:10px;
}
</style>
</head>
<body>
<h2>Player 2 Controller</h2>
<div>
  <button ontouchstart="send('UP')">UP</button><br>
  <button ontouchstart="send('LEFT')">LEFT</button>
  <button ontouchstart="send('RIGHT')">RIGHT</button><br>
  <button ontouchstart="send('DOWN')">DOWN</button>
</div>
<script>
function send(cmd){
  fetch('/input?player=2&cmd=' + cmd);
}
</script>
</body>
</html>
)html");
  }

  // --- Menu page ---
  else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println(R"html(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<style>
body{background:#111;color:#0f0;text-align:center;font-family:sans-serif;}
</style>
</head>
<body>
<h2>🕹️ Mini Arcade</h2>
<p id='mode'>Loading...</p>
<p><a href="/screen" style="color:#0f0;">Open Game Screen</a></p>
<p><a href="/controller" style="color:#0f0;">Open Controller</a></p>
<p>(Press mode button on Arduino to toggle modes)</p>
<script>
async function updateMode(){
  let r=await fetch('/mode');
  let j=await r.json();
  document.getElementById('mode').innerText =
    "Current Mode: " + (j.multi ? "Multiplayer 🧑‍🤝‍🧑" : "Single Player 🎮");
}
setInterval(updateMode,1000);
updateMode();
</script>
</body>
</html>
)html");
  }

  delay(1);
  client.stop();
}
