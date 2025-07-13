// Acknowledgments
// Creator: Anany Sharma at the University of Florida working under NSF grant. 2405373
// This material is based upon work supported by the National Science Foundation under Grant No. 2405373.
// Any opinions, findings, and conclusions or recommendations expressed in this material are those of the authors and do not necessarily reflect the views of the National Science Foundation.

// --- LIBRARIES ---
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// --- DISPLAY & HARDWARE PINS ---
#define TFT_CS   33
#define TFT_DC   25
#define TFT_RST  26
#define LEFT_SWITCH_PIN 36
#define RIGHT_SWITCH_PIN 39

// --- DISPLAY SETUP ---
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- COLORS (Defined directly) ---
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define BIRD_COLOR  0xFFE0 // Yellow
#define PIPE_COLOR  0x07E0 // Green
#define SKY_COLOR   0x549F // A nice light blue
#define TEXT_COLOR  0xFFFF // White
#define GROUND_COLOR 0x62A6 // Brownish-green


// --- GAME STATES ---
enum GameState {
  START_SCREEN,
  PLAYING,
  GAME_OVER
};
GameState gameState = START_SCREEN;

// --- GAME CONSTANTS ---
#define BIRD_X_POS 60       // Bird's constant horizontal position
#define BIRD_WIDTH 20
#define BIRD_HEIGHT 20
#define GRAVITY 0.3         // Pull on the bird
#define FLAP_VELOCITY -5.0  // Upward velocity on flap
#define GROUND_HEIGHT 10    // Height of the "ground" strip

#define PIPE_WIDTH 40
#define PIPE_GAP_HEIGHT 80  // Vertical space for the bird to pass through
#define PIPE_SPEED 2        // How fast pipes move to the left
#define PIPE_SPACING 180    // Horizontal distance between pipes
#define NUM_PIPES 2         // Use 2 pipes and recycle them

// --- GAME OBJECTS & VARIABLES ---
struct Pipe {
  int x;      // Horizontal position of the pipe's left edge
  int gap_y;  // Vertical position of the center of the gap
  bool scored; // Has the bird already scored on this pipe?
};
Pipe pipes[NUM_PIPES];

float bird_y;
float bird_y_velocity;
int score;
int highScore = 0;
bool button_was_pressed = false; // For handling single press vs. hold

// --- FUNCTION PROTOTYPES ---
void setupGame();
void updateGameLogic();
void drawGame();
void checkCollisions();
void drawStartScreen();
void drawGameOverScreen();

// =========================================================================
//  SETUP
// =========================================================================
void setup() {
  Serial.begin(115200);

  // Initialize display
  tft.init(SCREEN_HEIGHT, SCREEN_WIDTH);
  tft.setRotation(3);
  tft.fillScreen(BLACK);

  // Initialize input pins with pull-up resistors
  pinMode(LEFT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(RIGHT_SWITCH_PIN, INPUT_PULLUP);
  
  // Seed the random number generator
  randomSeed(analogRead(0));

  // Initialize game variables for the first time
  setupGame();
}

// =========================================================================
//  MAIN LOOP
// =========================================================================
void loop() {
  // --- STATE MACHINE ---
  switch (gameState) {
    case START_SCREEN:
      drawStartScreen();
      // Wait for a button press to start the game
      if (!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN)) {
        gameState = PLAYING;
        // debounce by waiting for release
        while(!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN));
      }
      break;

    case PLAYING:
      updateGameLogic();
      drawGame();
      break;

    case GAME_OVER:
      drawGameOverScreen();
      // Wait for a button press to restart
      if (!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN)) {
          setupGame(); // Reset game variables
          gameState = PLAYING;
          // debounce by waiting for release
          while(!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN));
      }
      break;
  }

  delay(10); // Small delay to control frame rate
}

// =========================================================================
//  GAME SETUP & RESET
// =========================================================================
void setupGame() {
  // Reset bird's position and velocity
  bird_y = SCREEN_HEIGHT / 2;
  bird_y_velocity = 0;

  // Reset score
  score = 0;
  
  // Initialize pipe positions
  for (int i = 0; i < NUM_PIPES; i++) {
    pipes[i].x = SCREEN_WIDTH + i * PIPE_SPACING;
    pipes[i].gap_y = random(PIPE_GAP_HEIGHT / 2 + 20, SCREEN_HEIGHT - PIPE_GAP_HEIGHT / 2 - GROUND_HEIGHT - 20);
    pipes[i].scored = false;
  }
}

// =========================================================================
//  GAME LOGIC UPDATES (PHYSICS, INPUT, SCORING)
// =========================================================================
void updateGameLogic() {
  // --- Handle Input (Flap) ---
  bool button_is_pressed = !digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN);
  if (button_is_pressed && !button_was_pressed) {
    bird_y_velocity = FLAP_VELOCITY;
  }
  button_was_pressed = button_is_pressed;

  // --- Bird Physics ---
  bird_y_velocity += GRAVITY;
  bird_y += bird_y_velocity;

  // --- Pipe Logic ---
  for (int i = 0; i < NUM_PIPES; i++) {
    pipes[i].x -= PIPE_SPEED; // Move pipes to the left

    // If a pipe goes off-screen, recycle it
    if (pipes[i].x < -PIPE_WIDTH) {
      pipes[i].x = SCREEN_WIDTH; // Move it to the far right
      pipes[i].gap_y = random(PIPE_GAP_HEIGHT / 2 + 20, SCREEN_HEIGHT - PIPE_GAP_HEIGHT / 2 - GROUND_HEIGHT - 20);
      pipes[i].scored = false; // Reset the scored flag
    }
    
    // --- Scoring Logic ---
    // If the bird's x position is past the pipe's x position and it hasn't been scored yet
    if (!pipes[i].scored && pipes[i].x + PIPE_WIDTH < BIRD_X_POS) {
      score++;
      pipes[i].scored = true; // Mark as scored
      if (score > highScore) {
          highScore = score;
      }
    }
  }
  
  // --- Collision Detection ---
  checkCollisions();
}


// =========================================================================
//  COLLISION DETECTION
// =========================================================================
void checkCollisions() {
  // Check for collision with top/bottom of the screen
  if (bird_y < 0 || bird_y + BIRD_HEIGHT > SCREEN_HEIGHT - GROUND_HEIGHT) {
    gameState = GAME_OVER;
    return;
  }
  
  // Check for collision with pipes
  for (int i = 0; i < NUM_PIPES; i++) {
    // Check if bird is horizontally aligned with the current pipe
    if (BIRD_X_POS + BIRD_WIDTH > pipes[i].x && BIRD_X_POS < pipes[i].x + PIPE_WIDTH) {
      // Check if bird hits the top or bottom pipe
      if (bird_y < pipes[i].gap_y - PIPE_GAP_HEIGHT / 2 || bird_y + BIRD_HEIGHT > pipes[i].gap_y + PIPE_GAP_HEIGHT / 2) {
        gameState = GAME_OVER;
        return;
      }
    }
  }
}

// =========================================================================
//  DRAWING FUNCTIONS (all draw directly to the tft)
// =========================================================================
void drawStartScreen() {
    tft.fillScreen(SKY_COLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(50, 40);
    tft.println("Flappy Bird");
    tft.setTextSize(2);
    tft.setCursor(70, 90);
    tft.println("Press to Start");
}

void drawGameOverScreen() {
    tft.fillScreen(SKY_COLOR);
    tft.setTextSize(3);
    tft.setTextColor(RED);
    tft.setCursor(70, 30);
    tft.println("GAME OVER");
    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(90, 80);
    tft.print("Score: ");
    tft.println(score);
    tft.setCursor(90, 110);
    tft.print("High: ");
    tft.println(highScore);
}

void drawGame() {
  // --- Clear screen with sky color ---
  // This is the primary cause of flicker without a canvas
  tft.fillScreen(SKY_COLOR);

  // --- Draw Pipes ---
  for (int i = 0; i < NUM_PIPES; i++) {
    // Top pipe rectangle
    tft.fillRect(pipes[i].x, 0, PIPE_WIDTH, pipes[i].gap_y - PIPE_GAP_HEIGHT / 2, PIPE_COLOR);
    // Bottom pipe rectangle
    int bottom_pipe_y = pipes[i].gap_y + PIPE_GAP_HEIGHT / 2;
    tft.fillRect(pipes[i].x, bottom_pipe_y, PIPE_WIDTH, SCREEN_HEIGHT - bottom_pipe_y, PIPE_COLOR);
  }

  // --- Draw Ground ---
  tft.fillRect(0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_COLOR);
  
  // --- Draw Bird ---
  tft.fillRect(BIRD_X_POS, (int)bird_y, BIRD_WIDTH, BIRD_HEIGHT, BIRD_COLOR);
  
  // --- Draw Score ---
  tft.setTextSize(2);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(10, 10);
  tft.print("Score: ");
  tft.print(score);
}