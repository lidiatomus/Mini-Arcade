
---

## 🕹️ Mini Arcade – Arduino UNO R4 WiFi

### 🎯 Overview

**Mini Arcade** is a Wi-Fi–based game console built with the **Arduino UNO R4 WiFi**.
It combines **hardware control** (joystick, buttons, buzzer, LEDs) with a **web-based display**, where one or two players can play classic games directly through their phones.

* 🟢 **Player 1** – controlled via the Arduino joystick
* 🔵 **Player 2** – connects from another phone as a controller
* 🎮 Games served directly from the Arduino as web pages
* 💡 LEDs and buzzer provide real-world effects
* 🔘 Physical mode button toggles *Single Player* / *Multiplayer*

---

### 🧩 Planned Game Modes

| Game               | Description                                     | Mode                 |
| ------------------ | ----------------------------------------------- | -------------------- |
| 🏓 **Ping Pong**   | Two paddles and a ball, 1 vs 1 or vs AI         | Multiplayer / Single |
| 🐍 **Snake**       | Classic snake collecting points                 | Single Player        |
| 🐦 **Flappy Bird** | Navigate through pipes using joystick or button | Single Player        |

Each game will be selectable from the OLED or phone menu, and scores will be saved locally to the SD card.

---

### ⚙️ Hardware Components

* Arduino **UNO R4 WiFi**
* Joystick module (X/Y + button)
* Tactile **mode button** (4 legs) – on pin **D7**
* **Buzzer** (pin D8) for sound effects
* **LED** (pin D9) for visual feedback
* Jumper wires & breadboard
* *(Optional)*

  * OLED display (menu & score display)
  * SD card module (save scores & settings)
  * LDR (light sensor for auto LED brightness)

---

### 🌐 How It Works

1. Arduino acts as a **local web server**.
2. Phones connect via Wi-Fi:

   * `/screen` → Game display (canvas)
   * `/controller` → Player 2 control interface
3. Joystick and phone send input through HTTP requests.
4. The canvas shows both players (green & blue balls).
5. The **mode button** switches between single and multiplayer, triggering LED and buzzer feedback.

---

### 💡 Features Implemented

* Real-time joystick data endpoint (`/data`)
* Dual-player mode via Wi-Fi
* Smooth rendering with caching and sync fixes
* LED & buzzer hardware feedback
* Auto-updating menu through `/mode` JSON endpoint
* Responsive web interface for phones

---

### 🚧 In Development

| Feature               | Description                                      |
| --------------------- | ------------------------------------------------ |
| 🏓 **Ping Pong Game** | Two paddles, real physics, score tracking        |
| 🐍 **Snake Game**     | Collect dots, increase speed and size            |
| 🐦 **Flappy Bird**    | Vertical joystick/button control with gravity    |
| 💾 **Score Saving**   | Player name, high scores stored on SD card       |
| 🖥️ **OLED Menu**     | Select game, add/delete player, view scores      |
| 🎧 **Music System**   | Background music + buzzer effects                |
| 💡 **Smart Lighting** | LDR adjusts LED brightness / phone flashlight    |
| 📡 **AP Mode**        | Arduino hosts its own hotspot (no router needed) |
| 🔢 **QR Code Join**   | Display a QR to connect phones instantly         |

---

### 🧰 How to Run

1. Upload the main sketch to your **Arduino UNO R4 WiFi**.
2. Open Serial Monitor → note the IP address.
3. Connect your phone(s) to the same network.

   * `http://<IP>` → main menu
   * `/screen` → game display
   * `/controller` → second player controller
4. Press the **mode button (D7)** to switch between single and multiplayer.
5. Move the joystick or press controller buttons to control the game.

---

### 🧑‍💻 Project Goals

* Combine **embedded systems** and **web technologies** into a mini console.
* Explore multiplayer synchronization and real-time drawing using the Arduino as a web server.
* Build multiple retro-inspired games (Snake, Flappy Bird, Ping Pong) with stored scores and local feedback.

---

