/*
  Retro Game Station - ESP32 DevKit V1 + ST7789 240x240 (no CS)
  Games: Snake, Pong, Breakout, Space Shooter, Flappy-style
  Controls:
    UP=32 DOWN=33 LEFT=25 RIGHT=26 A=27 B=14 START=16 SELECT=17
  Display:
    SCK=18 MOSI=23 DC=2 RST=4 CS=-1
  Library: TFT_eSPI (configure User_Setup.h as described in README)
  Board: ESP32 Dev Module / ESP32 DevKit V1
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

static const int SW = 240, SH = 240;
static const uint8_t PIN_UP=32, PIN_DOWN=33, PIN_LEFT=25, PIN_RIGHT=26;
static const uint8_t PIN_A=27, PIN_B=14, PIN_START=16, PIN_SELECT=17;

enum GameId { MENU=0, SNAKE, PONG, BREAKOUT, SHOOTER, FLAPPY };
GameId game = MENU;
int menuIndex = 0;
const char* gameNames[] = {"Snake", "Pong", "Breakout", "Space Shooter", "Flappy"};
const int GAME_COUNT = 5;

bool upNow, downNow, leftNow, rightNow, aNow, bNow, startNow, selectNow;
bool upPrev=false, downPrev=false, leftPrev=false, rightPrev=false;
bool aPrev=false, bPrev=false, startPrev=false, selectPrev=false;
uint32_t lastFrame=0;
const uint16_t BG = TFT_BLACK, FG = TFT_WHITE, ACCENT = TFT_CYAN;

bool pressed(uint8_t pin) { return digitalRead(pin) == LOW; }
void readButtons() {
  upNow=pressed(PIN_UP); downNow=pressed(PIN_DOWN);
  leftNow=pressed(PIN_LEFT); rightNow=pressed(PIN_RIGHT);
  aNow=pressed(PIN_A); bNow=pressed(PIN_B);
  startNow=pressed(PIN_START); selectNow=pressed(PIN_SELECT);
}
bool edge(bool now, bool &prev) { bool v=now && !prev; prev=now; return v; }
bool upHit(){return edge(upNow,upPrev);}
bool downHit(){return edge(downNow,downPrev);}
bool leftHit(){return edge(leftNow,leftPrev);}
bool rightHit(){return edge(rightNow,rightPrev);}
bool aHit(){return edge(aNow,aPrev);}
bool bHit(){return edge(bNow,bPrev);}
bool startHit(){return edge(startNow,startPrev);}
bool selectHit(){return edge(selectNow,selectPrev);}

void textCenter(const String &s, int y, uint8_t size=2, uint16_t color=TFT_WHITE) {
  tft.setTextSize(size); tft.setTextColor(color, BG);
  int w = s.length() * 6 * size;
  tft.setCursor((SW-w)/2, y); tft.print(s);
}
void drawHeader(const char* title) {
  tft.fillScreen(BG);
  tft.fillRect(0,0,SW,25,TFT_DARKCYAN);
  tft.setTextColor(TFT_WHITE,TFT_DARKCYAN);
  tft.setTextSize(2); tft.setCursor(7,5); tft.print(title);
}
void backToMenu() { game=MENU; tft.fillScreen(BG); }

void drawMenu() {
  tft.fillScreen(BG);
  tft.fillRect(0,0,SW,34,TFT_NAVY);
  tft.setTextColor(TFT_YELLOW,TFT_NAVY); tft.setTextSize(2);
  tft.setCursor(15,9); tft.print("RETRO GAME STATION");
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY,BG);
  tft.setCursor(16,43); tft.print("ESP32 + ST7789 240x240");
  for(int i=0;i<GAME_COUNT;i++){
    int y=64+i*27;
    if(i==menuIndex) {
      tft.fillRoundRect(12,y-3,216,23,5,TFT_DARKGREEN);
      tft.drawRoundRect(12,y-3,216,23,5,TFT_CYAN);
      tft.setTextColor(TFT_WHITE,TFT_DARKGREEN);
    } else tft.setTextColor(TFT_LIGHTGREY,BG);
    tft.setTextSize(2); tft.setCursor(25,y+2);
    tft.printf("%c %s", i==menuIndex?'>':' ', gameNames[i]);
  }
  tft.setTextSize(1); tft.setTextColor(TFT_YELLOW,BG);
  tft.setCursor(14,210); tft.print("UP/DOWN select  A/START play");
  tft.setCursor(14,224); tft.print("B/SELECT returns to menu");
}

void showGameOver(const char* title, int score) {
  tft.fillRect(22,80,196,84,TFT_DARKGREY);
  tft.drawRect(22,80,196,84,TFT_WHITE);
  textCenter(title,91,2,TFT_YELLOW);
  tft.setTextColor(TFT_WHITE,TFT_DARKGREY); tft.setTextSize(1);
  tft.setCursor(70,120); tft.printf("Score: %d",score);
  tft.setCursor(42,143); tft.print("A/START retry  B menu");
}

// ---------- SNAKE ----------
struct Point { int8_t x,y; };
Point snake[100];
int snakeLen, snakeDX, snakeDY, foodX, foodY, snakeScore;
uint32_t snakeTick;
bool snakeOver;
void placeFood() {
  foodX=random(0,20); foodY=random(0,20);
  for(int i=0;i<snakeLen;i++) if(snake[i].x==foodX && snake[i].y==foodY) { placeFood(); return; }
}
void snakeInit() {
  drawHeader("SNAKE");
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY,BG);
  tft.setCursor(8,30); tft.print("D-pad moves | B menu");
  snakeLen=4; for(int i=0;i<snakeLen;i++){snake[i].x=8-i;snake[i].y=9;}
  snakeDX=1; snakeDY=0; snakeScore=0; snakeOver=false; snakeTick=millis();
  placeFood();
}
void snakeDraw() {
  tft.fillRect(0,48,SW,192,BG);
  tft.fillCircle(6+foodX*11,54+foodY*9,3,TFT_RED);
  for(int i=0;i<snakeLen;i++) tft.fillRect(2+snake[i].x*11,50+snake[i].y*9,8,7,i==0?TFT_YELLOW:TFT_GREEN);
  tft.setTextColor(TFT_WHITE,BG); tft.setTextSize(1); tft.setCursor(170,30); tft.printf("S:%d",snakeScore);
}
void snakeUpdate() {
  if(bHit() || selectHit()){backToMenu();return;}
  if(snakeOver) { if(aHit()||startHit()) snakeInit(); return; }
  if(upHit()&&snakeDY!=1){snakeDX=0;snakeDY=-1;}
  if(downHit()&&snakeDY!=-1){snakeDX=0;snakeDY=1;}
  if(leftHit()&&snakeDX!=1){snakeDX=-1;snakeDY=0;}
  if(rightHit()&&snakeDX!=-1){snakeDX=1;snakeDY=0;}
  if(millis()-snakeTick<135) return;
  snakeTick=millis();
  Point h={(int8_t)(snake[0].x+snakeDX),(int8_t)(snake[0].y+snakeDY)};
  if(h.x<0||h.x>=20||h.y<0||h.y>=20){snakeOver=true;showGameOver("GAME OVER",snakeScore);return;}
  for(int i=0;i<snakeLen;i++) if(h.x==snake[i].x&&h.y==snake[i].y){snakeOver=true;showGameOver("GAME OVER",snakeScore);return;}
  bool eat=(h.x==foodX&&h.y==foodY);
  if(eat&&snakeLen<99){snakeLen++;snakeScore+=10;placeFood();}
  for(int i=snakeLen-1;i>0;i--)snake[i]=snake[i-1];
  snake[0]=h; snakeDraw();
}

// ---------- PONG ----------
int paddleY, ballX, ballY, ballDX, ballDY, pongScore;
bool pongOver;
uint32_t pongTick;
void pongInit(){
  drawHeader("PONG");
  tft.setTextSize(1);tft.setTextColor(TFT_LIGHTGREY,BG);tft.setCursor(8,30);tft.print("UP/DOWN move | B menu");
  paddleY=105;ballX=115;ballY=120;ballDX=3;ballDY=2;pongScore=0;pongOver=false;pongTick=millis();
}
void pongDraw(){
  tft.fillRect(0,48,SW,192,BG);tft.drawFastVLine(120,48,192,TFT_DARKGREY);
  tft.fillRect(12,paddleY,5,32,TFT_CYAN);tft.fillCircle(ballX,ballY,4,TFT_YELLOW);
  tft.setTextColor(TFT_WHITE,BG);tft.setTextSize(1);tft.setCursor(180,30);tft.printf("S:%d",pongScore);
}
void pongUpdate(){
  if(bHit()||selectHit()){backToMenu();return;}
  if(pongOver){if(aHit()||startHit())pongInit();return;}
  if(upNow)paddleY-=4;if(downNow)paddleY+=4;
  paddleY=constrain(paddleY,48,208);
  if(millis()-pongTick<25)return;pongTick=millis();
  ballX+=ballDX;ballY+=ballDY;
  if(ballY<53||ballY>234)ballDY=-ballDY;
  if(ballX<=19&&ballY>=paddleY-4&&ballY<=paddleY+36){ballDX=abs(ballDX);pongScore++;ballDX=min(ballDX+((pongScore%4)==0),5);}
  if(ballX<0||ballX>239){pongOver=true;showGameOver("PONG OVER",pongScore);return;}
  if(ballX>230){ballDX=-abs(ballDX);}
  pongDraw();
}

// ---------- BREAKOUT ----------
struct Brick { bool alive; };
Brick bricks[5][8];
int brX,brY,brDX,brDY,brPaddle,brScore;
bool brOver,brWin;
uint32_t brTick;
void breakoutInit(){
  drawHeader("BREAKOUT");
  tft.setTextSize(1);tft.setTextColor(TFT_LIGHTGREY,BG);tft.setCursor(8,30);tft.print("LEFT/RIGHT move | B menu");
  for(int r=0;r<5;r++)for(int c=0;c<8;c++)bricks[r][c].alive=true;
  brX=120;brY=190;brDX=2;brDY=-3;brPaddle=105;brScore=0;brOver=false;brWin=false;brTick=millis();
}
void breakoutDraw(){
  tft.fillRect(0,48,SW,192,BG);
  uint16_t colors[5]={TFT_RED,TFT_ORANGE,TFT_YELLOW,TFT_GREEN,TFT_CYAN};
  for(int r=0;r<5;r++)for(int c=0;c<8;c++)if(bricks[r][c].alive)tft.fillRect(5+c*29,55+r*13,25,9,colors[r]);
  tft.fillRect(brPaddle,222,34,5,TFT_WHITE);tft.fillCircle(brX,brY,3,TFT_YELLOW);
  tft.setTextColor(TFT_WHITE,BG);tft.setTextSize(1);tft.setCursor(180,30);tft.printf("S:%d",brScore);
}
void breakoutUpdate(){
  if(bHit()||selectHit()){backToMenu();return;}
  if(brOver||brWin){if(aHit()||startHit())breakoutInit();return;}
  if(leftNow)brPaddle-=4;if(rightNow)brPaddle+=4;brPaddle=constrain(brPaddle,0,206);
  if(millis()-brTick<20)return;brTick=millis();
  brX+=brDX;brY+=brDY;
  if(brX<4||brX>236)brDX=-brDX;if(brY<49)brDY=abs(brDY);
  if(brY>=218&&brX>=brPaddle&&brX<=brPaddle+34){brDY=-abs(brDY);brDX=(brX<brPaddle+17)?-2:2;}
  int c=(brX-5)/29,r=(brY-55)/13;
  if(r>=0&&r<5&&c>=0&&c<8&&bricks[r][c].alive&&brY<120){bricks[r][c].alive=false;brDY=-brDY;brScore+=10;}
  if(brY>240){brOver=true;showGameOver("GAME OVER",brScore);return;}
  bool any=false;for(int rr=0;rr<5;rr++)for(int cc=0;cc<8;cc++)if(bricks[rr][cc].alive)any=true;
  if(!any){brWin=true;showGameOver("YOU WIN!",brScore);return;}
  breakoutDraw();
}

// ---------- SPACE SHOOTER ----------
struct Shot { int x,y; bool on; };
struct Enemy { int x,y; bool on; };
Shot shots[5]; Enemy enemies[6];
int shipX, shooterScore, shooterDir;
bool shooterOver;
uint32_t shooterTick;
void shooterInit(){
  drawHeader("SPACE SHOOTER");
  tft.setTextSize(1);tft.setTextColor(TFT_LIGHTGREY,BG);tft.setCursor(8,30);tft.print("LEFT/RIGHT move A fire");
  shipX=120;shooterScore=0;shooterOver=false;shooterDir=1;shooterTick=millis();
  for(auto &s:shots)s.on=false;
  for(int i=0;i<6;i++){enemies[i].on=true;enemies[i].x=24+(i%3)*90;enemies[i].y=60+(i/3)*28;}
}
void shooterDraw(){
  tft.fillRect(0,48,SW,192,BG);
  tft.fillTriangle(shipX,222,shipX-9,236,shipX+9,236,TFT_CYAN);
  for(auto s:shots)if(s.on)tft.fillRect(s.x-1,s.y,3,7,TFT_YELLOW);
  for(auto e:enemies)if(e.on)tft.fillRect(e.x-7,e.y-5,14,10,TFT_RED);
  tft.setTextColor(TFT_WHITE,BG);tft.setTextSize(1);tft.setCursor(180,30);tft.printf("S:%d",shooterScore);
}
void shooterUpdate(){
  if(bHit()||selectHit()){backToMenu();return;}
  if(shooterOver){if(aHit()||startHit())shooterInit();return;}
  if(leftNow)shipX-=4;if(rightNow)shipX+=4;shipX=constrain(shipX,10,230);
  if(aHit()){for(auto &s:shots)if(!s.on){s.on=true;s.x=shipX;s.y=214;break;}}
  if(millis()-shooterTick<30)return;shooterTick=millis();
  for(auto &s:shots)if(s.on){s.y-=7;if(s.y<48)s.on=false;}
  bool any=false;
  for(auto &e:enemies)if(e.on){any=true;e.x+=shooterDir*2;if(e.x<10||e.x>230){shooterDir=-shooterDir;e.y+=8;}if(e.y>205){shooterOver=true;showGameOver("INVASION!",shooterScore);return;}
    for(auto &s:shots)if(s.on&&abs(s.x-e.x)<10&&abs(s.y-e.y)<9){s.on=false;e.on=false;shooterScore+=10;}}
  if(!any){for(int i=0;i<6;i++){enemies[i].on=true;enemies[i].x=24+(i%3)*90;enemies[i].y=55+(i/3)*25;} }
  shooterDraw();
}

// ---------- FLAPPY ----------
int birdY,birdV,pipeX,pipeGapY,flapScore;
bool flapOver;
uint32_t flapTick;
void flappyInit(){
  drawHeader("FLAPPY");
  tft.setTextSize(1);tft.setTextColor(TFT_LIGHTGREY,BG);tft.setCursor(8,30);tft.print("A/UP flap | B menu");
  birdY=130;birdV=0;pipeX=240;pipeGapY=random(90,160);flapScore=0;flapOver=false;flapTick=millis();
}
void flappyDraw(){
  tft.fillRect(0,48,SW,192,BG);
  tft.fillCircle(48,birdY,7,TFT_YELLOW);
  tft.fillRect(pipeX,48,24,pipeGapY-28,TFT_GREEN);
  tft.fillRect(pipeX,pipeGapY+35,24,240-(pipeGapY+35),TFT_GREEN);
  tft.setTextColor(TFT_WHITE,BG);tft.setTextSize(1);tft.setCursor(180,30);tft.printf("S:%d",flapScore);
}
void flappyUpdate(){
  if(bHit()||selectHit()){backToMenu();return;}
  if(flapOver){if(aHit()||startHit())flappyInit();return;}
  if(aHit()||upHit())birdV=-5;
  if(millis()-flapTick<28)return;flapTick=millis();
  birdV=min(birdV+1,5);birdY+=birdV;pipeX-=3;
  if(pipeX< -24){pipeX=240;pipeGapY=random(85,160);flapScore++;}
  bool inPipe=(pipeX<55&&pipeX+24>41);
  if(birdY<53||birdY>236||(inPipe&&(birdY-7<pipeGapY-28||birdY+7>pipeGapY+35))){
    flapOver=true;showGameOver("GAME OVER",flapScore);return;
  }
  flappyDraw();
}

void launchGame(int index) {
  switch(index){
    case 0:game=SNAKE;snakeInit();break;
    case 1:game=PONG;pongInit();break;
    case 2:game=BREAKOUT;breakoutInit();break;
    case 3:game=SHOOTER;shooterInit();break;
    case 4:game=FLAPPY;flappyInit();break;
  }
}

void setup() {
  pinMode(PIN_UP,INPUT_PULLUP);pinMode(PIN_DOWN,INPUT_PULLUP);
  pinMode(PIN_LEFT,INPUT_PULLUP);pinMode(PIN_RIGHT,INPUT_PULLUP);
  pinMode(PIN_A,INPUT_PULLUP);pinMode(PIN_B,INPUT_PULLUP);
  pinMode(PIN_START,INPUT_PULLUP);pinMode(PIN_SELECT,INPUT_PULLUP);
  randomSeed(esp_random());
  tft.init();tft.setRotation(0);tft.fillScreen(BG);
  drawMenu();
}

void loop() {
  readButtons();
  if(game==MENU){
    if(upHit()){menuIndex=(menuIndex+GAME_COUNT-1)%GAME_COUNT;drawMenu();}
    if(downHit()){menuIndex=(menuIndex+1)%GAME_COUNT;drawMenu();}
    if(aHit()||startHit())launchGame(menuIndex);
  } else {
    switch(game){
      case SNAKE:snakeUpdate();break;
      case PONG:pongUpdate();break;
      case BREAKOUT:breakoutUpdate();break;
      case SHOOTER:shooterUpdate();break;
      case FLAPPY:flappyUpdate();break;
      default:game=MENU;break;
    }
  }
  delay(1);
}
