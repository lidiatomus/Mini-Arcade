#include <WiFiS3.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================== Wi-Fi credentials ==================
char ssid[] = "AndroidAPC010lidia";
char pass[] = "lidia2004";

WiFiServer server(80);

// ================== Pins ==================
#define JOY_X     A0
#define JOY_Y     A1
#define JOY_BTN   8

// Shield buttons
#define BTN_A     2   // Menu UP  + P2 UP
#define BTN_B     3   // Select / Back
#define BTN_C     4   // Menu DOWN + P2 DOWN
#define BTN_D     5
#define BTN_E     6
#define BTN_F     7

// Other hardware
#define BUZZER    10
#define LED_PIN   9

// OLED config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// LDR light sensor
#define LDR_PIN A2
bool isNightMode = true;


// =========================================================
// OLED Helpers
// =========================================================
void beep(int freq, int duration) {
  tone(BUZZER, freq, duration);
}

void setOledBrightness(bool night) {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(night ? 10 : 255);
}

// ================== OLED: Show Scores ==================
int lastScore1 = -1;
int lastScore2 = -1;

void updateOLED(int s1, int s2) {
  if (s1 == lastScore1 && s2 == lastScore2) return;

  lastScore1 = s1;
  lastScore2 = s2;

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("P1:");
  display.print(s1);

  display.setCursor(0, 32);
  display.print("P2:");
  display.print(s2);

  display.display();
}

// ================== OLED: Menu Item ==================
void showMenuOLED(String item) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("MENU");

  display.setTextSize(2);
  display.setCursor(0,20);
  display.print("> ");
  display.print(item);

  display.display();
}

// ================== OLED: Game Title ==================
void showGameOLED(String title) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println(title);

  display.setTextSize(1);
  display.setCursor(0,40);
  display.println("Running...");

  display.display();
}


// =========================================================
// SETUP
// =========================================================
void setup() {
  Serial.begin(9600);

  // Wi-Fi connection
  WiFi.begin(ssid, pass);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());
  server.begin();

  // PinModes
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);
  pinMode(JOY_BTN, INPUT_PULLUP);

  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_C, INPUT_PULLUP);
  pinMode(BTN_D, INPUT_PULLUP);
  pinMode(BTN_E, INPUT_PULLUP);
  pinMode(BTN_F, INPUT_PULLUP);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed!");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("Mini Arcade");
  display.display();
  delay(1500);

  display.clearDisplay();
  display.display();

  setOledBrightness(true);
}


// =========================================================
// LOOP
// =========================================================
void loop() {

  // ----------------- LDR Day/Night Detection -----------------
  int lightValue = analogRead(LDR_PIN);
  static bool nightState = true;

  if (lightValue > 850 && nightState == true) {
    nightState = false;
    isNightMode = false;
    setOledBrightness(false);
    Serial.println("Mode changed → DAY");
  }

  if (lightValue < 300 && nightState == false) {
    nightState = true;
    isNightMode = true;
    setOledBrightness(true);
    Serial.println("Mode changed → NIGHT");
  }


  // ----------------- HTTP Server -----------------
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.flush();
  Serial.println(req);


  // ================== /data endpoint ==================
  if (req.indexOf("GET /data") >= 0) {

    int x = analogRead(JOY_X);
    int y = analogRead(JOY_Y);

    bool joy = !digitalRead(JOY_BTN);
    bool a = !digitalRead(BTN_A);
    bool b = !digitalRead(BTN_B);
    bool c = !digitalRead(BTN_C);
    bool d = !digitalRead(BTN_D);
    bool e = !digitalRead(BTN_E);
    bool f = !digitalRead(BTN_F);

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println();
    client.print("{\"x\":"); client.print(x);
    client.print(",\"y\":"); client.print(y);
    client.print(",\"joyBtn\":"); client.print(joy);
    client.print(",\"btnA\":"); client.print(a);
    client.print(",\"btnB\":"); client.print(b);
    client.print(",\"btnC\":"); client.print(c);
    client.print(",\"btnD\":"); client.print(d);
    client.print(",\"btnE\":"); client.print(e);
    client.print(",\"btnF\":"); client.print(f);
    client.print("}");
  }

  // ================== /mode endpoint ==================
  else if (req.indexOf("GET /mode") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println();
    client.print("{\"night\":");
    client.print(isNightMode ? "true" : "false");
    client.print("}");
  }

  // ================== /score endpoint ==================
  else if (req.indexOf("GET /score") >= 0) {
    int idx1 = req.indexOf("s1=");
    int idx2 = req.indexOf("s2=");

    int s1 = (idx1 > 0) ? req.substring(idx1+3).toInt() : 0;
    int s2 = (idx2 > 0) ? req.substring(idx2+3).toInt() : 0;

    updateOLED(s1, s2);

    client.println("HTTP/1.1 204 No Content");
    client.println();
  }

  // ================== /oled endpoint ==================
  else if (req.indexOf("GET /oled") >= 0) {
    int idx = req.indexOf("title=");
    if (idx > 0) {
      String t = req.substring(idx + 6);
      t.trim();
      showGameOLED(t);
    }
    client.println("HTTP/1.1 204 No Content");
    client.println();
  }

  // ================== /sound endpoint ==================
  else if (req.indexOf("GET /sound") >= 0) {
    if (req.indexOf("menu") > 0) beep(700, 80);
    if (req.indexOf("hit") > 0)  beep(300, 50);
    if (req.indexOf("score") > 0) beep(1000, 150);

    client.println("HTTP/1.1 204 No Content");
    client.println();
  }

  // ================== /screen HTML ==================
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
body { background:#000; margin:0; overflow:hidden; }
canvas { display:block; margin:0 auto; background:#111; }
h2 { color:#0f0; text-align:center; font-family:sans-serif; margin:8px; }
</style>
</head>
<body>
<h2>Mini Arcade</h2>
<canvas id="game" width="320" height="240"></canvas>

<script>
// ================== GLOBAL VARS ==================
let night = false;

// Poll day/night mode
async function checkMode(){
  try {
    let r = await fetch('/mode');
    let j = await r.json();
    night = j.night;
  } catch(e){}
}
setInterval(checkMode, 1000);

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');

let currentScreen = "menu";
let menuIndex = 0;

let lastBtnA = 0, lastBtnB = 0, lastBtnC = 0;


// ================== PING PONG STATE ==================
let pp_ball = { x:160, y:120, vx:2, vy:2 };
let pp_p1 = { y:120 };
let pp_p2 = { y:120 };
let pp_score1 = 0, pp_score2 = 0;

function pingpongReset(dir=1){
  pp_ball.x = 160;
  pp_ball.y = 120;
  pp_ball.vx = 2 * dir;
  pp_ball.vy = (Math.random()*2 - 1) * 2;
}

// ================== DRAW PING PONG ==================
function drawPingPong(){
  ctx.fillStyle = night ? "#000" : "#111";
  ctx.fillRect(0,0,320,240);

  ctx.strokeStyle="#333";
  ctx.setLineDash([5,5]);
  ctx.beginPath();
  ctx.moveTo(160,0);
  ctx.lineTo(160,240);
  ctx.stroke();
  ctx.setLineDash([]);

  ctx.fillStyle = night ? "#0a0" : "#0f0";
  ctx.font="16px monospace";
  ctx.fillText(pp_score1, 120, 30);
  ctx.fillText(pp_score2, 200, 30);

  ctx.fillRect(20, pp_p1.y - 20, 6, 40);
  ctx.fillRect(294, pp_p2.y - 20, 6, 40);

  ctx.beginPath();
  ctx.arc(pp_ball.x, pp_ball.y, 6, 0, Math.PI*2);
  ctx.fill();
}

// ================== UPDATE PING PONG ==================
function updatePingPong(data){
  pp_p1.y += (data.y - 512) / 40;
  pp_p1.y = Math.max(20, Math.min(220, pp_p1.y));

  if (data.btnA) pp_p2.y -= 4;
  if (data.btnC) pp_p2.y += 4;
  pp_p2.y = Math.max(20, Math.min(220, pp_p2.y));

  pp_ball.x += pp_ball.vx;
  pp_ball.y += pp_ball.vy;

  if (pp_ball.y <= 6 || pp_ball.y >= 234) pp_ball.vy *= -1;

  // Left paddle hit
  if (pp_ball.x <= 26 &&
      pp_ball.y >= pp_p1.y - 20 &&
      pp_ball.y <= pp_p1.y + 20){
    pp_ball.vx *= -1;
    pp_ball.x = 27;
    fetch('/sound?type=hit');
  }

  // Right paddle hit
  if (pp_ball.x >= 294 &&
      pp_ball.y >= pp_p2.y - 20 &&
      pp_ball.y <= pp_p2.y + 20){
    pp_ball.vx *= -1;
    pp_ball.x = 293;
    fetch('/sound?type=hit');
  }

  // Scoring
  if (pp_ball.x < 0){
    pp_score2++;
    pingpongReset(1);
    fetch('/sound?type=score');
  }

  if (pp_ball.x > 320){
    pp_score1++;
    pingpongReset(-1);
    fetch('/sound?type=score');
  }

  fetch(`/score?s1=${pp_score1}&s2=${pp_score2}`);
  drawPingPong();

  if (data.btnB && !lastBtnB) currentScreen="menu";
}

// ================== MENU ==================
function drawMenu(){
  ctx.fillStyle = night ? "#000" : "#111";
  ctx.fillRect(0,0,320,240);

  ctx.fillStyle = night ? "#0a0" : "#0f0";
  ctx.font = "16px monospace";
  ctx.textAlign="center";
  ctx.fillText("Select Game", 160, 40);

  let items = ["Ping Pong", "Snake", "Flappy Bird"];
  ctx.font = "14px monospace";

  for (let i = 0; i < items.length; i++){
    ctx.fillStyle = (i===menuIndex ? "#0f0" : "#666");
    ctx.fillText((i===menuIndex?"> ":"") + items[i], 160, 100 + i * 30);
  }
}

function drawPlaceholder(title){
  ctx.fillStyle = night ? "#000" : "#111";
  ctx.fillRect(0,0,320,240);

  ctx.fillStyle = night ? "#0a0" : "#0f0";
  ctx.font="18px monospace";
  ctx.fillText(title,160,60);

  ctx.font="12px monospace";
  ctx.fillText("Game coming soon...",160,110);

  ctx.font="10px monospace";
  ctx.fillText("Press B to return",160,200);
}

// ================== MAIN UPDATE LOOP ==================
async function update(){
  let data;
  try{
    let r = await fetch('/data',{cache:'no-store'});
    data = await r.json();
  } catch(e){
    data = {btnA:0,btnB:0,btnC:0,y:512};
  }

  if (currentScreen==="menu"){
    if (data.btnA && !lastBtnA) menuIndex = (menuIndex+2) % 3;
    if (data.btnC && !lastBtnC) menuIndex = (menuIndex+1) % 3;

    if (data.btnB && !lastBtnB){
      let game = ["PING PONG", "SNAKE", "FLAPPY"][menuIndex];
      fetch(`/oled?title=${game}`);
      fetch(`/sound?type=menu`);
      currentScreen = ["pingpong","snake","flappy"][menuIndex];
    }

    drawMenu();
  }

  else if (currentScreen === "pingpong"){
    updatePingPong(data);
  }

  else if (currentScreen === "snake"){
    drawPlaceholder("SNAKE");
    if (data.btnB && !lastBtnB) currentScreen="menu";
  }

  else if (currentScreen === "flappy"){
    drawPlaceholder("FLAPPY");
    if (data.btnB && !lastBtnB) currentScreen="menu";
  }

  lastBtnA = data.btnA;
  lastBtnB = data.btnB;
  lastBtnC = data.btnC;
}

setInterval(update, 100);
</script>
</body>
</html>
)html");
  }

  // ================== ROOT PAGE ==================
  else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<html><body style='background:#111;color:#0f0;text-align:center'>");
    client.println("<h1>Mini Arcade</h1>");
    client.println("<p><a style='color:#0f0' href='/screen'>Open Game Screen</a></p>");
    client.println("</body></html>");
  }

  client.stop();
}
