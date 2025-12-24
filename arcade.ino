#include <WiFiS3.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//wi fi
char ap_ssid[] = "MiniArcade";
char ap_pass[] = "12345678";

WiFiServer server(80);

// pins
//joystick
#define JOY_X A0
#define JOY_Y A1
#define JOY_BTN 8

// joystick buttons
#define BTN_A 2  // menu up and ping pong up for second player
#define BTN_B 3  // select and back
#define BTN_C 4  // menu down and ping pong down for second player
#define BTN_D 5
#define BTN_E 6
#define BTN_F 7

#define BUZZER 10
#define LED_PIN 9


// oled config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// LDR light sensor
#define LDR_PIN A2
bool isNightMode = true;

enum OledScreen {
  OLED_SPLASH,
  OLED_MENU,
  OLED_GAME
};

OledScreen oledScreen = OLED_SPLASH;
unsigned long oledTimer = 0;
String currentGame = "";

//function for buzzer
void beep(int freq, int duration) {
  tone(BUZZER, freq, duration);
}

//set oled brightness
void setOledBrightness(bool night) {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(night ? 10 : 255);
}

//scores for oled -> not functional cause the http request are with a delay
int lastScore1 = -1;
int lastScore2 = -1;
int score1 = 0;
int score2 = 0;

//old function for oled, for showing the score during the games
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

// menu function for showing the menu
void showMenuOLED(String item) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MENU");

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("> ");
  display.print(item);

  display.display();
}

//this was for testing
void showGameOLED(String title) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(title);

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("Running...");

  display.display();
}

// title function for oled
void drawSplash() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println("Mini");
  display.println("Arcade");
  display.display();
}

// menu function for showing the menu of games on the oled
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Select Game");
  display.println();
  display.println("Ping Pong");
  display.println("Snake");
  display.println("Flappy");
  display.display();
}

// title function for oled
void drawGameTitle(String title) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(title);
  display.display();
}


void setup() {
  Serial.begin(9600);

  //wi fi setup
  Serial.println("Starting Access Point...");
  WiFi.beginAP(ap_ssid, ap_pass);

  delay(2000);  //a delay such that the wi fi starts

  Serial.println("AP started!");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.localIP());  

  server.begin();


  // pin setup
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
  digitalWrite(LED_PIN, LOW);  // start with day mode (led off)

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed!");
    while (1)
      ;
  }

  display.setTextColor(SSD1306_WHITE);

  drawSplash();
  oledScreen = OLED_SPLASH;
  oledTimer = millis();


  setOledBrightness(true);
}


void loop() {
  // oled logic
  if (oledScreen == OLED_SPLASH) {
    if (millis() - oledTimer > 10000) {  
      oledScreen = OLED_MENU;
      drawMenu();
    }
  }

  // LDR day/night detection 
  int lightValue = analogRead(LDR_PIN);
  bool newNightMode = isNightMode;  // variable to hold the potential new state

  if (lightValue > 900) {  // DAY
    newNightMode = false;
  } else if (lightValue < 300) {  // NIGHT
    newNightMode = true;
  }

  // only update if the mode has actually changed
  if (newNightMode != isNightMode) {
    isNightMode = newNightMode;
    setOledBrightness(isNightMode);
  }

  digitalWrite(LED_PIN, isNightMode ? HIGH : LOW);


  //  http server
  WiFiClient client = server.available();
  if (!client) {
    delay(2);
    return;
  }


  String req = client.readStringUntil('\r');
  client.flush();
  Serial.println(req);


  // /data endpoint 
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
    client.print("{\"x\":");
    client.print(x);
    client.print(",\"y\":");
    client.print(y);
    client.print(",\"joyBtn\":");
    client.print(joy);
    client.print(",\"btnA\":");
    client.print(a);
    client.print(",\"btnB\":");
    client.print(b);
    client.print(",\"btnC\":");
    client.print(c);
    client.print(",\"btnD\":");
    client.print(d);
    client.print(",\"btnE\":");
    client.print(e);
    client.print(",\"btnF\":");
    client.print(f);
    client.print("}");
  }

  // /mode endpoint
  else if (req.indexOf("GET /mode") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println();
    client.print("{\"night\":");
    client.print(isNightMode ? "true" : "false");
    client.print("}");
  }

  // /score endpoint->not functional
  else if (req.indexOf("GET /score") >= 0) {
    int idx1 = req.indexOf("s1=");
    int idx2 = req.indexOf("s2=");


    int amp_idx = req.indexOf("&s2=");

    if (idx1 > 0 && idx2 > 0 && amp_idx > 0) {
      int s1 = req.substring(idx1 + 3, amp_idx).toInt();


      int end_idx = req.indexOf(' ', idx2);
      if (end_idx < 0) end_idx = req.length();

      int s2 = req.substring(idx2 + 3, end_idx).toInt();


      if (s1 != lastScore1 || s2 != lastScore2) {
        updateOLED(s1, s2);
      }
    }

    client.println("HTTP/1.1 204 No Content");
    client.println();
  }


  // /oled endpoint 
  else if (req.indexOf("GET /oled") >= 0) {
    int idx = req.indexOf("title=");
    if (idx > 0) {
      currentGame = req.substring(idx + 6);
      currentGame.trim();
      oledScreen = OLED_GAME;
      drawGameTitle(currentGame);
    }

    client.println("HTTP/1.1 204 No Content");
    client.println();
  }


  // /sound endpoint
  else if (req.indexOf("GET /sound") >= 0) {
    if (req.indexOf("menu") > 0) beep(700, 80);
    if (req.indexOf("hit") > 0) beep(300, 50);
    if (req.indexOf("score") > 0) beep(1000, 150);

    client.println("HTTP/1.1 204 No Content");
    client.println();
  } else if (req.indexOf("GET /oledmenu") >= 0) {
    oledScreen = OLED_MENU;
    drawMenu();

    client.println("HTTP/1.1 204 No Content");
    client.println();
  }


  // /screen HTML 
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
            // variables
            let night = false;
            let lastNight = false;

            let lastSentS1 = 0;
            let lastSentS2 = 0;

            //  day/night mode
            async function checkMode(){
              try {
                let r = await fetch('/mode');
                let j = await r.json();

                night = j.night;

                if (night !== lastNight && currentScreen === "menu") {
                  drawMenu();
                }

                lastNight = night;

              } catch(e){}
            }
            setInterval(checkMode, 1000);

            const canvas = document.getElementById('game');
            const ctx = canvas.getContext('2d');

            let currentScreen = "menu";
            let menuIndex = 0;

            let lastBtnA = 0, lastBtnB = 0, lastBtnC = 0;


            // ping pong
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

            // draw ping pong
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

            // update ping pong
            function updatePingPong(data){
              pp_p1.y += (data.y - 512) / 40;
              pp_p1.y = Math.max(20, Math.min(220, pp_p1.y));

              if (data.btnA) pp_p2.y -= 4;
              if (data.btnC) pp_p2.y += 4;
              pp_p2.y = Math.max(20, Math.min(220, pp_p2.y));

              pp_ball.x += pp_ball.vx;
              pp_ball.y += pp_ball.vy;

              if (pp_ball.y <= 6 || pp_ball.y >= 234) pp_ball.vy *= -1;

              // left paddle hit
              if (pp_ball.x <= 26 &&
                  pp_ball.y >= pp_p1.y - 20 &&
                  pp_ball.y <= pp_p1.y + 20){
                pp_ball.vx *= -1;
                pp_ball.x = 27;
                fetch('/sound?type=hit');
              }

              // right paddle hit
              if (pp_ball.x >= 294 &&
                  pp_ball.y >= pp_p2.y - 20 &&
                  pp_ball.y <= pp_p2.y + 20){
                pp_ball.vx *= -1;
                pp_ball.x = 293;
                fetch('/sound?type=hit');
              }

            // Scoring
            let scored = false;

            if (pp_ball.x < 0){
              pp_score2++;
              pingpongReset(1);
              fetch('/sound?type=score');
              scored = true;
            }

            if (pp_ball.x > 320){
              pp_score1++;
              pingpongReset(-1);
              fetch('/sound?type=score');
              scored = true;
            }

            // send score if it changed
            if (scored || pp_score1 !== lastSentS1 || pp_score2 !== lastSentS2) {
              fetch(`/score?s1=${pp_score1}&s2=${pp_score2}`);
              lastSentS1 = pp_score1;
              lastSentS2 = pp_score2;
            }


              drawPingPong();

              if (data.btnB && !lastBtnB) {currentScreen="menu";  fetch('/oledmenu');
              }
            }

            // snake
            let snake = [];
            let snakeDir = {x:1, y:0};
            let snakeFood = {x:10, y:10};
            let snakeTick = 0;

            function resetSnake(){
              snake = [
                {x:8, y:8},
                {x:7, y:8},
                {x:6, y:8}
              ];
              snakeDir = {x:1, y:0};
              snakeFood = {
                x: Math.floor(Math.random()*20),
                y: Math.floor(Math.random()*15)
              };
            }
            resetSnake();
            function drawSnake(){
              ctx.fillStyle = night ? "#000" : "#111";
              ctx.fillRect(0,0,320,240);

              // food
              ctx.fillStyle = "red";
              ctx.fillRect(snakeFood.x*16, snakeFood.y*16, 16, 16);

              // snake
              ctx.fillStyle = "#0f0";
              for (let s of snake){
                ctx.fillRect(s.x*16, s.y*16, 16, 16);
              }
            }

            //update snake
            function updateSnake(data){
              // back to menu
            if (data.btnB && !lastBtnB) {
              currentScreen = "menu";
                fetch('/oledmenu');

              return;
            }

            snakeTick++;
            if (snakeTick < 6) {
              drawSnake();
              return;
            }
            snakeTick = 0;


            // controls using joystick (4 directions)

            // UP
            if (data.y < 400 && snakeDir.y !== 1) {
              snakeDir = { x: 0, y: -1 };
            }
            // DOWN
            else if (data.y > 600 && snakeDir.y !== -1) {
              snakeDir = { x: 0, y: 1 };
            }
            // LEFT
            else if (data.x < 400 && snakeDir.x !== 1) {
              snakeDir = { x: -1, y: 0 };
            }
            // RIGHT
            else if (data.x > 600 && snakeDir.x !== -1) {
              snakeDir = { x: 1, y: 0 };
            }



              let head = {
                x: snake[0].x + snakeDir.x,
                y: snake[0].y + snakeDir.y
              };

              // wall collision
              if (head.x < 0 || head.y < 0 || head.x >= 20 || head.y >= 15){
                resetSnake();
                return;
              }

              // self collision
              for (let s of snake){
                if (s.x === head.x && s.y === head.y){
                  resetSnake();
                  return;
                }
              }

              snake.unshift(head);

              // food
              if (head.x === snakeFood.x && head.y === snakeFood.y){
                snakeFood = {
                  x: Math.floor(Math.random()*20),
                  y: Math.floor(Math.random()*15)
                };
              } else {
                snake.pop();
              }

              drawSnake();

              if (data.btnB && !lastBtnB) currentScreen = "menu";
            }
            
            // flappy bird
            let fb_birdY = 120;
            let fb_birdVY = 0;
            const fb_birdX = 70;

            let fb_pipes = [];
            let fb_score = 0;
            let fb_best = 0;
            let fb_gameOver = false;

            let fb_spawnTick = 0;
            let fb_lastFlap = 0;

            function resetFlappy(){
              fb_birdY = 120;
              fb_birdVY = 0;
              fb_pipes = [];
              fb_score = 0;
              fb_gameOver = false;
              fb_spawnTick = 0;
            }

            function spawnPipe(){
              const gap = 80;         
              const pipeW = 45;
              const minTop = 30;
              const maxTop = 240 - 30 - gap;

              const topH = Math.floor(minTop + Math.random() * (maxTop - minTop));
              fb_pipes.push({
                x: 320,
                w: pipeW,
                top: topH,
                gap: gap,
                passed: false
              });
            }

            function drawFlappy(){
              ctx.fillStyle = night ? "#000" : "#111";
              ctx.fillRect(0,0,320,240);

              // Pipes
              ctx.fillStyle = night ? "#0a0" : "#0f0";
              for (let p of fb_pipes){
                // top pipe
                ctx.fillRect(p.x, 0, p.w, p.top);
                // bottom pipe
                ctx.fillRect(p.x, p.top + p.gap, p.w, 240 - (p.top + p.gap));
              }

              // bird
              ctx.fillStyle = "yellow";
              ctx.fillRect(fb_birdX - 8, fb_birdY - 8, 16, 16);

              // score
              ctx.fillStyle = night ? "#0a0" : "#0f0";
              ctx.font = "16px monospace";
              ctx.textAlign = "left";
              ctx.fillText("Score: " + fb_score, 10, 20);
              ctx.fillText("Best: " + fb_best, 10, 40);

              if (fb_gameOver){
                ctx.textAlign = "center";
                ctx.font = "18px monospace";
                ctx.fillText("GAME OVER", 160, 110);
                ctx.font = "12px monospace";
                ctx.fillText("Press JOY or A to restart", 160, 140);
                ctx.fillText("Press B to menu", 160, 160);
              }
            }

            function rectHit(ax, ay, aw, ah, bx, by, bw, bh){
              return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
            }

            function updateFlappy(data){
              // Back to menu
              if (data.btnB && !lastBtnB) {
                currentScreen = "menu";
                  fetch('/oledmenu');

                return;
              }

              // flap input: joystick button OR BTN_A
              const flapPressed = (data.joyBtn || data.btnA);

              // if game over: restart on flap
              if (fb_gameOver){
                if (flapPressed && !fb_lastFlap){
                  resetFlappy();
                  fetch('/sound?type=menu');
                }
                fb_lastFlap = flapPressed ? 1 : 0;
                drawFlappy();
                return;
              }

              // flap (edge-triggered)
              if (flapPressed && !fb_lastFlap){
                fb_birdVY = -7;
                fetch('/sound?type=hit');
              }
              fb_lastFlap = flapPressed ? 1 : 0;

              // Physics
              fb_birdVY += 0.8;       // gravity
              fb_birdY += fb_birdVY;

              // ground/ceiling
              if (fb_birdY < 8) fb_birdY = 8;
              if (fb_birdY > 232){
                fb_gameOver = true;
                fb_best = Math.max(fb_best, fb_score);
                fetch('/sound?type=score');
              }

              // spawn pipes
              fb_spawnTick++;
              if (fb_spawnTick >= 18){   // ~1.8s at 10fps
                fb_spawnTick = 0;
                spawnPipe();
              }

              // move pipes and collisions
              for (let p of fb_pipes){
                p.x -= 4;

                // score when passing pipe
                if (!p.passed && p.x + p.w < fb_birdX){
                  p.passed = true;
                  fb_score++;
                  fetch('/sound?type=score');
                }

                // collision (bird rect vs pipes)
                const birdRect = {x: fb_birdX - 8, y: fb_birdY - 8, w: 16, h: 16};

                // top pipe rect
                if (rectHit(birdRect.x, birdRect.y, birdRect.w, birdRect.h, p.x, 0, p.w, p.top)){
                  fb_gameOver = true;
                  fb_best = Math.max(fb_best, fb_score);
                  fetch('/sound?type=score');
                }

                // bottom pipe rect
                const botY = p.top + p.gap;
                if (rectHit(birdRect.x, birdRect.y, birdRect.w, birdRect.h, p.x, botY, p.w, 240 - botY)){
                  fb_gameOver = true;
                  fb_best = Math.max(fb_best, fb_score);
                  fetch('/sound?type=score');
                }
              }

              // Remove off-screen pipes
              fb_pipes = fb_pipes.filter(p => p.x + p.w > -5);

              drawFlappy();
            }



            // menu
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
              ctx.font = "12px monospace";
              ctx.fillStyle = "#888";
              ctx.fillText(
                "Night Mode: " + (night ? "ON" : "OFF"),
                160,
                225
              );
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

            //main update loop
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
                  if (currentScreen === "snake") resetSnake();
                  if (currentScreen === "flappy") resetFlappy();

                }

                drawMenu();
              }

              else if (currentScreen === "pingpong"){
                updatePingPong(data);
              }

              else if (currentScreen === "snake"){
              updateSnake(data);
            }


            else if (currentScreen === "flappy"){
              updateFlappy(data);
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

  // root page
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