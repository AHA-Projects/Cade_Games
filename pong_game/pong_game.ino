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

// --- GAME STATES ---
enum GameState {
  START_SCREEN,
  PLAYING,
  GAME_OVER
};
GameState gameState = START_SCREEN;

// --- GAME CONSTANTS ---
#define PADDLE_WIDTH 80
#define PADDLE_HEIGHT 10
#define PADDLE_Y_POS (SCREEN_HEIGHT - PADDLE_HEIGHT - 2) // Player paddle Y position
#define AI_PADDLE_Y_POS 2                               // AI paddle Y position
#define PADDLE_SPEED 6

#define BALL_SIZE 10
#define WINNING_SCORE 5

// --- COLORS ---
#define PADDLE_COLOR 0xFFFF // White
#define BALL_COLOR   0xFFE0 // Yellow
#define BG_COLOR     0x0000 // Black
#define TEXT_COLOR   0xFFFF // White
#define RED          0xF800

// --- GAME OBJECTS & VARIABLES ---
float player_x;
float ai_x;
float ball_x, ball_y;
float ball_vx, ball_vy; // Ball velocity
int playerScore;
int aiScore;

// --- FUNCTION PROTOTYPES ---
void serveBall(int direction);
void resetGame();
void updateGameLogic();
void drawGame();
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
  tft.fillScreen(BG_COLOR);

  // Initialize input pins with pull-up resistors
  pinMode(LEFT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(RIGHT_SWITCH_PIN, INPUT_PULLUP);
  
  // Seed the random number generator
  randomSeed(analogRead(0));

  // Initialize game variables for the first time
  resetGame();
  gameState = START_SCREEN; // Start at the title screen
}

// =========================================================================
//  MAIN LOOP
// =========================================================================
void loop() {
  // --- STATE MACHINE ---
  switch (gameState) {
    case START_SCREEN:
      drawStartScreen();
      if (!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN)) {
        resetGame(); // Ensure scores are 0
        gameState = PLAYING;
        while(!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN));
      }
      break;

    case PLAYING:
      updateGameLogic();
      drawGame();
      // Check for win condition
      if (playerScore >= WINNING_SCORE || aiScore >= WINNING_SCORE) {
          gameState = GAME_OVER;
      }
      break;

    case GAME_OVER:
      drawGameOverScreen();
      if (!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN)) {
          resetGame();
          gameState = START_SCREEN;
          while(!digitalRead(LEFT_SWITCH_PIN) || !digitalRead(RIGHT_SWITCH_PIN));
      }
      break;
  }

  delay(10); // Control frame rate
}

// =========================================================================
//  GAME SETUP & BALL SERVING
// =========================================================================
void resetGame() {
    playerScore = 0;
    aiScore = 0;
    player_x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
    ai_x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
    serveBall(random(2) == 0 ? 1 : -1); // Serve to a random player
}

void serveBall(int direction) {
  ball_x = SCREEN_WIDTH / 2;
  ball_y = SCREEN_HEIGHT / 2;
  ball_vx = random(2, 4) * (random(2) == 0 ? 1 : -1); // Random horizontal speed and direction
  ball_vy = 3 * direction; // Serve up or down
}

// =========================================================================
//  GAME LOGIC UPDATES
// =========================================================================
void updateGameLogic() {
  // --- Player Input ---
  if (!digitalRead(LEFT_SWITCH_PIN)) {
    player_x -= PADDLE_SPEED;
  }
  if (!digitalRead(RIGHT_SWITCH_PIN)) {
    player_x += PADDLE_SPEED;
  }

  // Constrain player paddle to screen
  if (player_x < 0) player_x = 0;
  if (player_x > SCREEN_WIDTH - PADDLE_WIDTH) player_x = SCREEN_WIDTH - PADDLE_WIDTH;

  // --- AI Logic ---
  // Simple AI: tries to center its paddle on the ball's x position
  float ai_target_x = ball_x - PADDLE_WIDTH / 2;
  if (ai_x < ai_target_x) ai_x += PADDLE_SPEED * 0.7; // Make AI slightly slower
  if (ai_x > ai_target_x) ai_x -= PADDLE_SPEED * 0.7;

  // Constrain AI paddle to screen
  if (ai_x < 0) ai_x = 0;
  if (ai_x > SCREEN_WIDTH - PADDLE_WIDTH) ai_x = SCREEN_WIDTH - PADDLE_WIDTH;


  // --- Ball Physics & Collision ---
  ball_x += ball_vx;
  ball_y += ball_vy;

  // Wall collisions (left/right)
  if (ball_x < 0 || ball_x > SCREEN_WIDTH - BALL_SIZE) {
    ball_vx = -ball_vx;
  }
  
  // Paddle collisions
  // Player paddle
  if (ball_vy > 0 && ball_y + BALL_SIZE >= PADDLE_Y_POS && ball_x + BALL_SIZE > player_x && ball_x < player_x + PADDLE_WIDTH) {
    ball_vy = -ball_vy; // Bounce
    // Optional: Add a bit of horizontal velocity based on where it hit the paddle
    ball_vx += (ball_x - (player_x + PADDLE_WIDTH / 2)) / 10.0;
  }
  // AI paddle
  if (ball_vy < 0 && ball_y <= AI_PADDLE_Y_POS + PADDLE_HEIGHT && ball_x + BALL_SIZE > ai_x && ball_x < ai_x + PADDLE_WIDTH) {
    ball_vy = -ball_vy; // Bounce
  }

  // --- Scoring ---
  if (ball_y > SCREEN_HEIGHT) { // AI scores
    aiScore++;
    serveBall(-1); // Serve to AI
  }
  if (ball_y < 0) { // Player scores
    playerScore++;
    serveBall(1); // Serve to Player
  }
}

// =========================================================================
//  DRAWING FUNCTIONS
// =========================================================================
void drawStartScreen() {
    tft.fillScreen(BG_COLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(110, 40);
    tft.println("PONG");
    tft.setTextSize(2);
    tft.setCursor(70, 90);
    tft.println("Press to Start");
}

void drawGameOverScreen() {
    tft.fillScreen(BG_COLOR);
    tft.setTextSize(3);
    tft.setTextColor(RED);
    tft.setCursor(70, 30);
    tft.println("GAME OVER");
    
    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(90, 80);
    if (playerScore > aiScore) {
        tft.println("YOU WIN!");
    } else {
        tft.println("AI WINS!");
    }
    
    tft.setCursor(70, 120);
    tft.println("Press to Restart");
}

void drawGame() {
  // This is the main source of flicker. The whole screen is cleared.
  tft.fillScreen(BG_COLOR);

  // Draw dashed center line
  for (int i = 0; i < SCREEN_WIDTH; i += 15) {
    tft.drawFastVLine(i, SCREEN_HEIGHT / 2, 5, TEXT_COLOR);
  }
  
  // Draw paddles
  tft.fillRect((int)player_x, PADDLE_Y_POS, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_COLOR);
  tft.fillRect((int)ai_x, AI_PADDLE_Y_POS, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_COLOR);

  // Draw ball
  tft.fillCircle((int)ball_x, (int)ball_y, BALL_SIZE / 2, BALL_COLOR);
  
  // Draw scores
  tft.setTextSize(2);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(10, SCREEN_HEIGHT / 2 - 25);
  tft.print(aiScore);
  tft.setCursor(10, SCREEN_HEIGHT / 2 + 10);
  tft.print(playerScore);
}
