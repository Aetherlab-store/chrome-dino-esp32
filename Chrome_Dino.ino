#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP_I2S.h>
#include <math.h>

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================================
// BUTTON
// =====================================================

#define BUTTON_PIN 32

// =====================================================
// MAX98357 I2S
// =====================================================

#define I2S_BCLK 27
#define I2S_LRC  25
#define I2S_DIN  26

I2SClass I2S;

#define SAMPLE_RATE 16000
#define AUDIO_VOLUME 10000

// Small audio buffer.
// Smaller buffer = less time spent inside I2S.write()
#define AUDIO_BUFFER_SAMPLES 64

int16_t audioBuffer[AUDIO_BUFFER_SAMPLES * 2];

// =====================================================
// GAME AREA
// =====================================================

#define GROUND_Y 53

// =====================================================
// DINO
// =====================================================

#define DINO_X 18

float dinoY;
float dinoVelocity;

#define DINO_WIDTH 16
#define DINO_HEIGHT 16

#define GRAVITY 0.32
#define JUMP_FORCE -5.2

bool dinoOnGround = true;

// =====================================================
// CACTUS
// =====================================================

#define CACTUS_COUNT 2

float cactusX[CACTUS_COUNT];

int cactusWidth[CACTUS_COUNT];
int cactusHeight[CACTUS_COUNT];

#define CACTUS_MIN_GAP 48
#define CACTUS_MAX_GAP 72

// =====================================================
// GAME SPEED
// =====================================================

float gameSpeed = 1.7;

#define INITIAL_SPEED 1.7
#define MAX_SPEED 4.0

// =====================================================
// SCORE
// =====================================================

int score = 0;

// =====================================================
// LIVES
// =====================================================

#define MAX_LIVES 3

int lives = MAX_LIVES;

// =====================================================
// GAME STATES
// =====================================================

enum GameState {
  MENU,
  PLAYING,
  GAME_OVER
};

GameState gameState = MENU;

// =====================================================
// FRAME TIMING
// =====================================================

#define FRAME_TIME 30

unsigned long lastFrameTime = 0;

// =====================================================
// BUTTON DEBOUNCE
// =====================================================

bool lastButtonState = HIGH;

unsigned long lastButtonTime = 0;

#define DEBOUNCE_TIME 120

// =====================================================
// GROUND SCROLL
// =====================================================

int groundOffset = 0;

// =====================================================
// RANDOM CACTUS SEED
// =====================================================

int lastCactusHeight = 10;

// =====================================================
// NON-BLOCKING AUDIO ENGINE
// =====================================================

bool audioPlaying = false;

float audioFrequency = 0;

float audioPhase = 0.0;

unsigned long audioStartTime = 0;
unsigned long audioDuration = 0;

// =====================================================
// SOUND SEQUENCE
// =====================================================

#define MAX_SOUND_NOTES 5

struct SoundNote {
  int frequency;
  int duration;
};

SoundNote soundSequence[MAX_SOUND_NOTES];

int soundNoteCount = 0;
int currentSoundNote = 0;

bool sequencePlaying = false;

// =====================================================
// START TONE
// =====================================================

void startTone(int frequency, int duration) {

  if (frequency <= 0 || duration <= 0) {
    return;
  }

  audioFrequency = frequency;
  audioDuration = duration;
  audioStartTime = millis();

  audioPlaying = true;

  audioPhase = 0.0;

  Serial.print("AUDIO: ");
  Serial.print(frequency);
  Serial.print(" Hz / ");
  Serial.print(duration);
  Serial.println(" ms");
}

// =====================================================
// START SOUND SEQUENCE
// =====================================================

void startSoundSequence(
  SoundNote *notes,
  int count
) {

  if (count <= 0) {
    return;
  }

  if (count > MAX_SOUND_NOTES) {
    count = MAX_SOUND_NOTES;
  }

  for (int i = 0; i < count; i++) {
    soundSequence[i] = notes[i];
  }

  soundNoteCount = count;
  currentSoundNote = 0;

  sequencePlaying = true;

  startTone(
    soundSequence[0].frequency,
    soundSequence[0].duration
  );
}

// =====================================================
// UPDATE AUDIO
//
// IMPORTANT:
// This does NOT use delay().
// The game continues running while audio plays.
// =====================================================

void updateAudio() {

  // ---------------------------------------------------
  // Check current sound
  // ---------------------------------------------------

  if (!audioPlaying) {
    return;
  }

  unsigned long elapsed =
    millis() - audioStartTime;

  // ---------------------------------------------------
  // Current note finished
  // ---------------------------------------------------

  if (elapsed >= audioDuration) {

    audioPlaying = false;

    // Move to next note
    if (sequencePlaying) {

      currentSoundNote++;

      if (currentSoundNote < soundNoteCount) {

        startTone(
          soundSequence[currentSoundNote].frequency,
          soundSequence[currentSoundNote].duration
        );

      } else {

        sequencePlaying = false;
        currentSoundNote = 0;

        Serial.println(
          "AUDIO: Sequence finished"
        );
      }
    }

    return;
  }

  // ---------------------------------------------------
  // Generate small audio chunk
  // ---------------------------------------------------

  for (
    int i = 0;
    i < AUDIO_BUFFER_SAMPLES;
    i++
  ) {

    float sample =
      sin(audioPhase) *
      AUDIO_VOLUME;

    int16_t value =
      (int16_t)sample;

    // Left
    audioBuffer[i * 2] =
      value;

    // Right
    audioBuffer[i * 2 + 1] =
      value;

    audioPhase +=
      2.0 * PI *
      audioFrequency /
      SAMPLE_RATE;

    if (audioPhase >= 2.0 * PI) {
      audioPhase -= 2.0 * PI;
    }
  }

  I2S.write(
    (uint8_t *)audioBuffer,
    sizeof(audioBuffer)
  );
}

// =====================================================
// DINO JUMP SOUND
// =====================================================

void playJumpSound() {

  sequencePlaying = false;

  startTone(
    850,
    45
  );
}

// =====================================================
// SCORE SOUND
// =====================================================

void playScoreSound() {

  SoundNote notes[2] = {
    {1000, 40},
    {1400, 60}
  };

  startSoundSequence(
    notes,
    2
  );
}

// =====================================================
// HIT SOUND
// =====================================================

void playHitSound() {

  SoundNote notes[3] = {
    {450, 80},
    {300, 100},
    {180, 130}
  };

  startSoundSequence(
    notes,
    3
  );
}

// =====================================================
// START SOUND
// =====================================================

void playStartSound() {

  SoundNote notes[2] = {
    {600, 70},
    {900, 100}
  };

  startSoundSequence(
    notes,
    2
  );
}

// =====================================================
// GAME OVER SOUND
// =====================================================

void playGameOverSound() {

  SoundNote notes[4] = {
    {600, 100},
    {500, 100},
    {400, 120},
    {250, 250}
  };

  startSoundSequence(
    notes,
    4
  );
}

// =====================================================
// RESTART SOUND
// =====================================================

void playRestartSound() {

  SoundNote notes[2] = {
    {700, 60},
    {1000, 90}
  };

  startSoundSequence(
    notes,
    2
  );
}

// =====================================================
// AUDIO SETUP
// =====================================================

void setupAudio() {

  Serial.println();
  Serial.println(
    "AUDIO: Initializing MAX98357..."
  );

  I2S.setPins(
    I2S_BCLK,
    I2S_LRC,
    I2S_DIN,
    -1
  );

  if (
    !I2S.begin(
      I2S_MODE_STD,
      SAMPLE_RATE,
      I2S_DATA_BIT_WIDTH_16BIT,
      I2S_SLOT_MODE_STEREO
    )
  ) {

    Serial.println(
      "AUDIO ERROR: I2S initialization failed!"
    );

    return;
  }

  Serial.println(
    "AUDIO: I2S initialized successfully"
  );

  Serial.println(
    "AUDIO: Non-blocking audio enabled"
  );
}

// =====================================================
// RESET DINO
// =====================================================

void resetDino() {

  dinoY =
    GROUND_Y - DINO_HEIGHT;

  dinoVelocity = 0;

  dinoOnGround = true;

  Serial.println(
    "DINO: Reset"
  );
}

// =====================================================
// CREATE CACTUS
// =====================================================

void createCactus(int index, float xPosition) {

  cactusX[index] =
    xPosition;

  // Two basic cactus sizes
  if (random(0, 2) == 0) {

    cactusWidth[index] = 7;
    cactusHeight[index] = 11;

  } else {

    cactusWidth[index] = 10;
    cactusHeight[index] = 15;
  }

  Serial.print("CACTUS ");
  Serial.print(index);
  Serial.print(": X=");
  Serial.print(cactusX[index]);
  Serial.print(" H=");
  Serial.println(cactusHeight[index]);
}

// =====================================================
// RESET CACTI
// =====================================================

void resetCacti() {

  Serial.println(
    "GAME: Resetting cacti"
  );

  createCactus(
    0,
    150
  );

  createCactus(
    1,
    150 + CACTUS_MIN_GAP + random(10, 30)
  );
}

// =====================================================
// RESET GAME
// =====================================================

void resetGame() {

  Serial.println();
  Serial.println(
    "GAME: New game"
  );

  score = 0;

  lives =
    MAX_LIVES;

  gameSpeed =
    INITIAL_SPEED;

  groundOffset = 0;

  resetDino();

  resetCacti();
}

// =====================================================
// JUMP
// =====================================================

void jumpDino() {

  if (!dinoOnGround) {

    Serial.println(
      "DINO: Jump ignored - airborne"
    );

    return;
  }

  dinoVelocity =
    JUMP_FORCE;

  dinoOnGround =
    false;

  Serial.println(
    "DINO: JUMP"
  );

  playJumpSound();
}

// =====================================================
// UPDATE DINO PHYSICS
// =====================================================

void updateDino() {

  // Gravity
  dinoVelocity +=
    GRAVITY;

  // Position
  dinoY +=
    dinoVelocity;

  // ---------------------------------------------------
  // Ground
  // ---------------------------------------------------

  if (
    dinoY >=
    GROUND_Y - DINO_HEIGHT
  ) {

    dinoY =
      GROUND_Y - DINO_HEIGHT;

    dinoVelocity = 0;

    dinoOnGround =
      true;
  }
}

// =====================================================
// UPDATE CACTI
// =====================================================

void updateCacti() {

  for (
    int i = 0;
    i < CACTUS_COUNT;
    i++
  ) {

    // Move left
    cactusX[i] -=
      gameSpeed;

    // -------------------------------------------------
    // Cactus passed Dino
    // -------------------------------------------------

    if (
      cactusX[i] +
      cactusWidth[i] <
      DINO_X
    ) {

      score++;

      Serial.print(
        "SCORE: "
      );

      Serial.println(score);

      playScoreSound();

      // Increase speed gradually
      gameSpeed =
        INITIAL_SPEED +
        score * 0.08;

      if (
        gameSpeed >
        MAX_SPEED
      ) {

        gameSpeed =
          MAX_SPEED;
      }

      Serial.print(
        "GAME SPEED: "
      );

      Serial.println(
        gameSpeed
      );
    }

    // -------------------------------------------------
    // Recycle cactus
    // -------------------------------------------------

    if (
      cactusX[i] +
      cactusWidth[i] <
      0
    ) {

      float farthestX =
        cactusX[0];

      for (
        int j = 1;
        j < CACTUS_COUNT;
        j++
      ) {

        if (
          cactusX[j] >
          farthestX
        ) {

          farthestX =
            cactusX[j];
        }
      }

      cactusX[i] =
        farthestX +
        CACTUS_MIN_GAP +
        random(
          0,
          CACTUS_MAX_GAP -
          CACTUS_MIN_GAP
        );

      // New cactus size
      if (random(0, 2) == 0) {

        cactusWidth[i] = 7;
        cactusHeight[i] = 11;

      } else {

        cactusWidth[i] = 10;
        cactusHeight[i] = 15;
      }

      Serial.print(
        "CACTUS "
      );

      Serial.print(i);

      Serial.println(
        ": Recycled"
      );
    }
  }
}

// =====================================================
// COLLISION DETECTION
// =====================================================

bool checkCactusCollision() {

  // Slightly smaller hitbox than graphic.
  // Makes gameplay more forgiving.

  int dinoLeft =
    DINO_X + 2;

  int dinoRight =
    DINO_X +
    DINO_WIDTH -
    2;

  int dinoTop =
    (int)dinoY + 2;

  int dinoBottom =
    (int)dinoY +
    DINO_HEIGHT -
    1;

  for (
    int i = 0;
    i < CACTUS_COUNT;
    i++
  ) {

    int cactusLeft =
      (int)cactusX[i] + 1;

    int cactusRight =
      (int)cactusX[i] +
      cactusWidth[i] -
      1;

    int cactusBottom =
      GROUND_Y;

    int cactusTop =
      GROUND_Y -
      cactusHeight[i];

    // Rectangle collision
    if (
      dinoRight > cactusLeft &&
      dinoLeft < cactusRight &&
      dinoBottom > cactusTop &&
      dinoTop < cactusBottom
    ) {

      Serial.print(
        "COLLISION: Cactus "
      );

      Serial.println(i);

      return true;
    }
  }

  return false;
}

// =====================================================
// LOSE LIFE
// =====================================================

void loseLife() {

  lives--;

  Serial.print(
    "GAME: LIFE LOST - Remaining: "
  );

  Serial.println(lives);

  playHitSound();

  if (lives > 0) {

    Serial.println(
      "GAME: Continuing..."
    );

    resetDino();

    resetCacti();

    gameSpeed =
      INITIAL_SPEED +
      score * 0.08;

    if (
      gameSpeed >
      MAX_SPEED
    ) {

      gameSpeed =
        MAX_SPEED;
    }
  }

  else {

    Serial.println(
      "GAME: ALL LIVES LOST"
    );

    Serial.println(
      "GAME: GAME OVER"
    );

    gameState =
      GAME_OVER;

    playGameOverSound();
  }
}

// =====================================================
// DRAW DINO
//
// Simple pixel-style Chrome Dino inspired character
// =====================================================

void drawDino() {

  int x =
    DINO_X;

  int y =
    (int)dinoY;

  // ---------------------------------------------------
  // Head
  // ---------------------------------------------------

  display.fillRect(
    x + 5,
    y,
    9,
    7,
    SSD1306_WHITE
  );

  // Snout
  display.fillRect(
    x + 12,
    y + 3,
    4,
    4,
    SSD1306_WHITE
  );

  // ---------------------------------------------------
  // Body
  // ---------------------------------------------------

  display.fillRect(
    x + 2,
    y + 6,
    10,
    7,
    SSD1306_WHITE
  );

  // ---------------------------------------------------
  // Tail
  // ---------------------------------------------------

  display.fillRect(
    x,
    y + 8,
    5,
    3,
    SSD1306_WHITE
  );

  display.fillRect(
    x - 2,
    y + 10,
    4,
    2,
    SSD1306_WHITE
  );

  // ---------------------------------------------------
  // Eye
  // ---------------------------------------------------

  display.drawPixel(
    x + 11,
    y + 2,
    SSD1306_BLACK
  );

  // ---------------------------------------------------
  // Leg
  // ---------------------------------------------------

  if (
    dinoOnGround &&
    ((millis() / 120) % 2 == 0)
  ) {

    display.fillRect(
      x + 3,
      y + 12,
      3,
      4,
      SSD1306_WHITE
    );

    display.fillRect(
      x + 10,
      y + 12,
      3,
      4,
      SSD1306_WHITE
    );

  } else {

    display.fillRect(
      x + 5,
      y + 12,
      3,
      4,
      SSD1306_WHITE
    );

    display.fillRect(
      x + 11,
      y + 12,
      3,
      4,
      SSD1306_WHITE
    );
  }
}

// =====================================================
// DRAW CACTUS
// =====================================================

void drawCactus(int index) {

  int x =
    (int)cactusX[index];

  int h =
    cactusHeight[index];

  int w =
    cactusWidth[index];

  int bottom =
    GROUND_Y;

  int top =
    bottom - h;

  // Main stem
  display.fillRect(
    x,
    top,
    w >= 9 ? 4 : 3,
    h,
    SSD1306_WHITE
  );

  // ---------------------------------------------------
  // Left arm
  // ---------------------------------------------------

  if (h >= 13) {

    int armY =
      top + 5;

    display.fillRect(
      x - 3,
      armY,
      3,
      3,
      SSD1306_WHITE
    );

    display.fillRect(
      x - 3,
      armY - 3,
      3,
      3,
      SSD1306_WHITE
    );
  }

  // ---------------------------------------------------
  // Right arm
  // ---------------------------------------------------

  if (h >= 11) {

    int armY =
      top + 7;

    display.fillRect(
      x + 4,
      armY,
      3,
      3,
      SSD1306_WHITE
    );

    display.fillRect(
      x + 4,
      armY - 3,
      3,
      3,
      SSD1306_WHITE
    );
  }
}

// =====================================================
// DRAW GROUND
//
// Chrome Dino-style scrolling ground
// =====================================================

void drawGround() {

  // Main ground line
  display.drawLine(
    0,
    GROUND_Y,
    SCREEN_WIDTH - 1,
    GROUND_Y,
    SSD1306_WHITE
  );

  // Moving ground pixels
  for (
    int x = -groundOffset;
    x < SCREEN_WIDTH;
    x += 13
  ) {

    display.drawLine(
      x,
      GROUND_Y + 3,
      x + 4,
      GROUND_Y + 3,
      SSD1306_WHITE
    );
  }

  // Small secondary pixels
  for (
    int x = 7 - groundOffset;
    x < SCREEN_WIDTH;
    x += 29
  ) {

    display.drawPixel(
      x,
      GROUND_Y + 5,
      SSD1306_WHITE
    );
  }
}

// =====================================================
// DRAW SCORE
//
// Format: 00000
// =====================================================

void drawScore() {

  display.setTextSize(1);

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setCursor(
    91,
    0
  );

  char scoreText[6];

  snprintf(
    scoreText,
    sizeof(scoreText),
    "%05d",
    score
  );

  display.print(
    scoreText
  );
}

// =====================================================
// DRAW LIVES
//
// Small indicators on upper-left
// =====================================================

void drawLives() {

  display.setTextSize(1);

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setCursor(
    2,
    0
  );

  display.print("x");

  display.print(lives);
}

// =====================================================
// DRAW PLAYING SCREEN
// =====================================================

void drawGame() {

  display.clearDisplay();

  // Score
  drawScore();

  // Lives
  drawLives();

  // Dino
  drawDino();

  // Cacti
  for (
    int i = 0;
    i < CACTUS_COUNT;
    i++
  ) {

    drawCactus(i);
  }

  // Ground
  drawGround();

  display.display();
}

// =====================================================
// DRAW MENU
// =====================================================

void drawMenu() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  // Title
  display.setTextSize(2);

  display.setCursor(
    22,
    10
  );

  display.println(
    "DINO"
  );

  display.setTextSize(1);

  display.setCursor(
    19,
    31
  );

  display.println(
    "RUNNER"
  );

  // Small dino
  int oldY =
    (int)dinoY;

  dinoY = 38;

  drawDino();

  dinoY =
    oldY;

  display.setCursor(
    24,
    53
  );

  display.println(
    "PRESS BUTTON"
  );

  display.display();
}

// =====================================================
// DRAW GAME OVER
// =====================================================

void drawGameOver() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(2);

  display.setCursor(
    16,
    8
  );

  display.println(
    "GAME"
  );

  display.setCursor(
    16,
    26
  );

  display.println(
    "OVER"
  );

  display.setTextSize(1);

  display.setCursor(
    31,
    47
  );

  display.print(
    "SCORE "
  );

  char scoreText[6];

  snprintf(
    scoreText,
    sizeof(scoreText),
    "%05d",
    score
  );

  display.println(
    scoreText
  );

  display.display();
}

// =====================================================
// BUTTON
// =====================================================

bool buttonPressed() {

  bool currentState =
    digitalRead(
      BUTTON_PIN
    );

  bool pressed =
    false;

  if (
    lastButtonState == HIGH &&
    currentState == LOW
  ) {

    unsigned long now =
      millis();

    if (
      now - lastButtonTime >
      DEBOUNCE_TIME
    ) {

      pressed =
        true;

      lastButtonTime =
        now;

      Serial.println(
        "BUTTON: PRESSED"
      );
    }
  }

  lastButtonState =
    currentState;

  return pressed;
}

// =====================================================
// UPDATE GROUND
// =====================================================

void updateGround() {

  groundOffset +=
    (int)gameSpeed;

  if (
    groundOffset >= 13
  ) {

    groundOffset =
      0;
  }
}

// =====================================================
// UPDATE GAME
// =====================================================

void updateGame() {

  // ---------------------------------------------------
  // Button
  // ---------------------------------------------------

  if (
    buttonPressed()
  ) {

    jumpDino();
  }

  // ---------------------------------------------------
  // Dino physics
  // ---------------------------------------------------

  updateDino();

  // ---------------------------------------------------
  // Cactus movement
  // ---------------------------------------------------

  updateCacti();

  // ---------------------------------------------------
  // Ground
  // ---------------------------------------------------

  updateGround();

  // ---------------------------------------------------
  // Collision
  // ---------------------------------------------------

  if (
    checkCactusCollision()
  ) {

    loseLife();
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );

  delay(500);

  Serial.println();
  Serial.println(
    "======================================"
  );

  Serial.println(
    "       ESP32 CHROME DINO GAME"
  );

  Serial.println(
    "======================================"
  );

  // ---------------------------------------------------
  // Button
  // ---------------------------------------------------

  pinMode(
    BUTTON_PIN,
    INPUT_PULLUP
  );

  Serial.println(
    "BUTTON: GPIO32 initialized"
  );

// ---------------------------------------------------
// OLED
// ---------------------------------------------------

Wire.begin(
  OLED_SDA,
  OLED_SCL
);

Serial.println(
  "OLED: Initializing..."
);

if (
  !display.begin(
    SSD1306_SWITCHCAPVCC,
    OLED_ADDRESS
  )
) {

  Serial.println(
    "OLED ERROR: Initialization failed!"
  );

  while (true) {
    delay(1000);
  }
}

Serial.println(
  "OLED: Initialization successful"
);

// Rotate OLED display 180 degrees
display.setRotation(2);

display.clearDisplay();

display.display();

  // ---------------------------------------------------
  // Random seed
  // ---------------------------------------------------

  randomSeed(
    analogRead(34)
  );

  Serial.println(
    "GAME: Random generator initialized"
  );

  // ---------------------------------------------------
  // Audio
  // ---------------------------------------------------

  setupAudio();

  // ---------------------------------------------------
  // Initial game
  // ---------------------------------------------------

  resetGame();

  gameState =
    MENU;

  Serial.println();
  Serial.println(
    "SYSTEM: DINO READY"
  );

  Serial.println(
    "SYSTEM: Press button to start"
  );

  Serial.println();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // ===================================================
  // AUDIO
  //
  // Runs independently from the game frame.
  // ===================================================

  updateAudio();

  // ===================================================
  // GAME FRAME
  // ===================================================

  unsigned long currentTime =
    millis();

  if (
    currentTime -
    lastFrameTime >=
    FRAME_TIME
  ) {

    lastFrameTime =
      currentTime;

    // -------------------------------------------------
    // MENU
    // -------------------------------------------------

    if (
      gameState ==
      MENU
    ) {

      drawMenu();

      if (
        buttonPressed()
      ) {

        Serial.println(
          "MENU: Starting Dino game"
        );

        resetGame();

        gameState =
          PLAYING;

        playStartSound();
      }
    }

    // -------------------------------------------------
    // PLAYING
    // -------------------------------------------------

    else if (
      gameState ==
      PLAYING
    ) {

      updateGame();

      drawGame();
    }

    // -------------------------------------------------
    // GAME OVER
    // -------------------------------------------------

    else if (
      gameState ==
      GAME_OVER
    ) {

      drawGameOver();

      if (
        buttonPressed()
      ) {

        Serial.println(
          "GAME OVER: Restarting"
        );

        resetGame();

        gameState =
          PLAYING;

        playRestartSound();
      }
    }
  }
}