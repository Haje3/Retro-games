/*
  Retro Game Station 50+ (arcade challenge edition)
  Target: ESP32-WROOM-32 / DevKit V1, no PSRAM
  Display: ST7789 240x240, TFT_eSPI, no CS wired
  Pins: SCK=18 MOSI=23 DC=2 RST=4
  Buttons (button to GND; INPUT_PULLUP):
    UP=32 DOWN=33 LEFT=25 RIGHT=26 A=27 B=14 START=16 SELECT=17

  NOTE: This is source code, not a tested binary. Compile and test on hardware.
  The menu contains 52 named challenge variants built from 10 reusable game engines.
*/
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

static const uint8_t PIN_UP=32, PIN_DOWN=33, PIN_LEFT=25, PIN_RIGHT=26;
static const uint8_t PIN_A=27, PIN_B=14, PIN_START=16, PIN_SELECT=17;

struct GameDef { const char* name; uint8_t engine; uint8_t difficulty; uint16_t color; };
GameDef games[] = {
  {"Snake Classic",0,1,TFT_GREEN},{"Snake Speed",0,2,TFT_CYAN},{"Snake Maze",0,3,TFT_YELLOW},{"Snake Rush",0,4,TFT_ORANGE},{"Snake Survival",0,5,TFT_RED},
  {"Pong Solo",1,1,TFT_CYAN},{"Pong Fast",1,2,TFT_YELLOW},{"Pong Pro",1,3,TFT_GREEN},{"Pong Chaos",1,4,TFT_MAGENTA},{"Pong Master",1,5,TFT_RED},
  {"Breakout",2,1,TFT_YELLOW},{"Brick Storm",2,2,TFT_ORANGE},{"Brick Rush",2,3,TFT_RED},{"Brick Wall",2,4,TFT_CYAN},{"Brick Master",2,5,TFT_MAGENTA},
  {"Space Shooter",3,1,TFT_CYAN},{"Alien Wave",3,2,TFT_GREEN},{"Meteor Storm",3,3,TFT_ORANGE},{"Boss Attack",3,4,TFT_RED},{"Galaxy Survival",3,5,TFT_MAGENTA},
  {"Road Dodger",4,1,TFT_GREEN},{"Traffic Rush",4,2,TFT_YELLOW},{"Night Drive",4,3,TFT_CYAN},{"Highway Chaos",4,4,TFT_ORANGE},{"Racing Pro",4,5,TFT_RED},
  {"Flappy Dot",5,1,TFT_CYAN},{"Pipe Runner",5,2,TFT_GREEN},{"Sky Challenge",5,3,TFT_YELLOW},{"Wind Rider",5,4,TFT_MAGENTA},{"Flappy Expert",5,5,TFT_RED},
  {"Catch The Star",6,1,TFT_YELLOW},{"Fruit Catch",6,2,TFT_GREEN},{"Rain Catcher",6,3,TFT_CYAN},{"Gem Collector",6,4,TFT_MAGENTA},{"Catch Master",6,5,TFT_ORANGE},
  {"Reaction Test",7,1,TFT_GREEN},{"Quick Reflex",7,2,TFT_CYAN},{"Color Reflex",7,3,TFT_YELLOW},{"Speed Tap",7,4,TFT_ORANGE},{"Reflex Master",7,5,TFT_RED},
  {"Maze Escape",8,1,TFT_CYAN},{"Maze Timer",8,2,TFT_GREEN},{"Dark Maze",8,3,TFT_MAGENTA},{"Maze Sprint",8,4,TFT_YELLOW},{"Maze Master",8,5,TFT_RED},
  {"Target Practice",9,1,TFT_GREEN},{"Moving Target",9,2,TFT_CYAN},{"Target Rush",9,3,TFT_YELLOW},{"Precision Shot",9,4,TFT_ORANGE},{"Target Master",9,5,TFT_RED},
  {"Snake Tiny",0,1,TFT_WHITE},{"Pong Retro",1,1,TFT_WHITE}
};
const int GAME_COUNT=sizeof(games)/sizeof(games[0]);

bool pressed(uint8_t p){ return digitalRead(p)==LOW; }
bool edge(uint8_t p) {
  static uint32_t last[40]={0};
  if (!pressed(p)) return false;
  uint32_t now=millis();
  if(now-last[p]<170) return false;
  last[p]=now; return true;
}
void waitRelease() {
  uint32_t start=millis();
  while ((pressed(PIN_A)||pressed(PIN_B)||pressed(PIN_START)) && millis()-start<1500) delay(10);
}
void centerText(const char* s,int y,uint16_t c,uint8_t size=2) {
  tft.setTextSize(size); tft.setTextColor(c,TFT_BLACK);
  int16_t w=tft.textWidth(s); tft.setCursor((240-w)/2,y); tft.print(s);
}
void drawHeader(const char* title,uint16_t color) {
  tft.fillScreen(TFT_BLACK); tft.fillRect(0,0,240,27,color);
  tft.setTextColor(TFT_BLACK,color); tft.setTextSize(2); tft.setCursor(6,6); tft.print(title);
}
int selected=0, page=0;
void drawMenu(){
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0,0,240,30,TFT_NAVY);
  tft.setTextColor(TFT_WHITE,TFT_NAVY); tft.setTextSize(2); tft.setCursor(7,7); tft.print("RETRO GAME 52");
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
  tft.setCursor(7,36); tft.printf("Game %d/%d  UP/DOWN select",selected+1,GAME_COUNT);
  for(int i=0;i<6;i++){
    int idx=(selected/6)*6+i;
    int y=54+i*27;
    if(idx>=GAME_COUNT) break;
    uint16_t bg=(idx==selected)?TFT_DARKCYAN:TFT_BLACK;
    tft.fillRoundRect(5,y,230,23,4,bg);
    tft.setTextColor(idx==selected?TFT_WHITE:games[idx].color,bg);
    tft.setTextSize(1); tft.setCursor(12,y+8); tft.printf("%02d. %s",idx+1,games[idx].name);
  }
  tft.setTextColor(TFT_LIGHTGREY,TFT_BLACK); tft.setTextSize(1);
  tft.setCursor(6,224); tft.print("A/START: play  B: back");
}
void gameOver(int score,const char* msg){
  tft.fillScreen(TFT_BLACK); centerText(msg,65,TFT_RED,2);
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE,TFT_BLACK); tft.setCursor(65,105); tft.printf("Score %d",score);
  centerText("A: retry",155,TFT_GREEN,2); centerText("B: menu",185,TFT_CYAN,2);
}
bool abortGame(){ return edge(PIN_B)||edge(PIN_SELECT); }
int runSnake(uint8_t d,uint16_t col){
  const int cell=10, cols=22, rows=19;
  int x[80],y[80],len=4,dx=1,dy=0;
  for(int i=0;i<len;i++){x[i]=8-i;y[i]=9;}
  int fx=random(1,cols-1),fy=random(1,rows-1),score=0;
  uint32_t last=0; drawHeader("SNAKE",col);
  while(true){
    if(edge(PIN_UP)&&dy==0){dx=0;dy=-1;} if(edge(PIN_DOWN)&&dy==0){dx=0;dy=1;}
    if(edge(PIN_LEFT)&&dx==0){dx=-1;dy=0;} if(edge(PIN_RIGHT)&&dx==0){dx=1;dy=0;}
    if(abortGame()) return -1;
    if(millis()-last < (210-d*25)) {delay(4);continue;} last=millis();
    int nx=x[0]+dx,ny=y[0]+dy;
    if(nx<0||nx>=cols||ny<0||ny>=rows){gameOver(score,"SNAKE OVER");return score;}
    for(int i=0;i<len;i++) if(x[i]==nx&&y[i]==ny){gameOver(score,"SNAKE OVER");return score;}
    for(int i=len-1;i>0;i--){x[i]=x[i-1];y[i]=y[i-1];} x[0]=nx;y[0]=ny;
    if(nx==fx&&ny==fy){if(len<78)len++;score+=10;fx=random(1,cols-1);fy=random(1,rows-1);}
    tft.fillRect(0,28,240,212,TFT_BLACK);
    tft.fillRect(fx*cell,28+fy*cell,8,8,TFT_RED);
    for(int i=0;i<len;i++)tft.fillRect(x[i]*cell,28+y[i]*cell,8,8,i?col:TFT_WHITE);
    tft.setTextSize(1);tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setCursor(4,30);tft.printf("%d",score);
  }
}
int runPong(uint8_t d,uint16_t col){
  int px=105, bx=120,by=110,vx=2+d,vy=2,score=0; uint32_t last=0;
  drawHeader("PONG",col);
  while(true){
    if(edge(PIN_LEFT))px-=12;if(edge(PIN_RIGHT))px+=12;
    px=constrain(px,0,190);
    if(abortGame())return -1;
    if(millis()-last<18)continue;last=millis();
    bx+=vx;by+=vy;
    if(bx<3||bx>237)vx=-vx;if(by<32)vy=-vy;
    if(by>=208&&bx>=px&&bx<=px+50&&vy>0){vy=-abs(vy);score++;}
    if(by>238){gameOver(score,"PONG OVER");return score;}
    tft.fillRect(0,28,240,212,TFT_BLACK);tft.fillRect(px,218,50,6,col);tft.fillCircle(bx,by,4,TFT_WHITE);
    tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setTextSize(1);tft.setCursor(5,34);tft.printf("HITS %d",score);
  }
}
int runBreakout(uint8_t d,uint16_t col){
  const int bw=29,bh=10; bool bricks[6][7];for(int r=0;r<6;r++)for(int c=0;c<7;c++)bricks[r][c]=true;
  int px=95,bx=120,by=170,vx=2+d/2,vy=-3,score=0;uint32_t last=0;
  drawHeader("BREAKOUT",col);
  while(true){
    if(edge(PIN_LEFT))px-=14;if(edge(PIN_RIGHT))px+=14;px=constrain(px,0,190);
    if(abortGame())return -1;
    if(millis()-last<18)continue;last=millis();bx+=vx;by+=vy;
    if(bx<3||bx>237)vx=-vx;if(by<31)vy=-vy;
    if(by>207&&by<226&&bx>=px&&bx<=px+50)vy=-abs(vy);
    if(by>239){gameOver(score,"BALL LOST");return score;}
    for(int r=0;r<6;r++)for(int c=0;c<7;c++)if(bricks[r][c]&&bx>=c*34+2&&bx<c*34+32&&by>=36+r*13&&by<48+r*13){bricks[r][c]=false;vy=-vy;score+=5;}
    tft.fillRect(0,28,240,212,TFT_BLACK);
    for(int r=0;r<6;r++)for(int c=0;c<7;c++)if(bricks[r][c])tft.fillRect(c*34+2,36+r*13,30,9,col);
    tft.fillRect(px,218,50,5,TFT_WHITE);tft.fillCircle(bx,by,4,TFT_YELLOW);
    tft.setTextSize(1);tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setCursor(4,30);tft.printf("%d",score);
  }
}
int runShooter(uint8_t d,uint16_t col){
  int px=120,score=0;int ex[5],ey[5];for(int i=0;i<5;i++){ex[i]=random(8,232);ey[i]=random(35,130);}
  int shotsX[5],shotsY[5];for(int i=0;i<5;i++){shotsX[i]=-1;shotsY[i]=-1;}
  uint32_t last=0;drawHeader("SPACE SHOOTER",col);
  while(true){
    if(edge(PIN_LEFT))px-=9;if(edge(PIN_RIGHT))px+=9;px=constrain(px,6,234);
    if(edge(PIN_A)){for(int i=0;i<5;i++)if(shotsY[i]<0){shotsX[i]=px;shotsY[i]=205;break;}}
    if(abortGame())return -1;
    if(millis()-last<24)continue;last=millis();
    for(int i=0;i<5;i++){if(shotsY[i]>=0)shotsY[i]-=7;ey[i]+=1+d/2;if(ey[i]>220){ey[i]=32;ex[i]=random(8,232);}
      if(shotsY[i]>=ey[i]-5&&shotsY[i]<=ey[i]+8&&abs(shotsX[i]-ex[i])<12){score+=10;ey[i]=32;ex[i]=random(8,232);shotsY[i]=-1;}
    }
    tft.fillRect(0,28,240,212,TFT_BLACK);
    for(int i=0;i<5;i++){tft.fillCircle(ex[i],ey[i],6,TFT_RED);if(shotsY[i]>=0)tft.fillRect(shotsX[i]-1,shotsY[i],3,8,TFT_WHITE);}
    tft.fillTriangle(px,199,px-9,216,px+9,216,col);
    tft.setTextSize(1);tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setCursor(4,30);tft.printf("SCORE %d",score);
    for(int i=0;i<5;i++)if(ey[i]>190&&abs(ex[i]-px)<12){gameOver(score,"SHIP HIT");return score;}
  }
}
int runDodger(uint8_t d,uint16_t col){
  int px=120,score=0,ox[3]={60,150,200},oy[3]={35,90,145};uint32_t last=0;drawHeader("ROAD DODGER",col);
  while(true){
    if(edge(PIN_LEFT))px-=12;if(edge(PIN_RIGHT))px+=12;px=constrain(px,15,225);
    if(abortGame())return -1;
    if(millis()-last<24)continue;last=millis();
    for(int i=0;i<3;i++){oy[i]+=2+d;if(oy[i]>235){oy[i]=-10;ox[i]=random(30,210);score++;}}
    tft.fillRect(0,28,240,212,TFT_BLACK);
    for(int y=30;y<240;y+=32)tft.fillRect(117,y,4,15,TFT_DARKGREY);
    tft.fillRect(px-8,202,16,25,col);
    for(int i=0;i<3;i++){tft.fillRect(ox[i]-8,oy[i],16,25,TFT_RED);if(oy[i]>185&&oy[i]<228&&abs(ox[i]-px)<15){gameOver(score,"CRASH!");return score;}}
    tft.setTextSize(1);tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setCursor(4,30);tft.printf("SCORE %d",score);
  }
}
int runFlappy(uint8_t d,uint16_t col){
  int y=120,vel=0,pipeX=240,gapY=100,score=0;uint32_t last=0;drawHeader("FLAPPY DOT",col);
  while(true){
    if(edge(PIN_A)||edge(PIN_UP))vel=-5;
    if(abortGame())return -1;
    if(millis()-last<28)continue;last=millis();vel+=1;y+=vel;pipeX-=3+d/2;
    if(pipeX< -24){pipeX=240;gapY=random(70,165);score++;}
    if(y<30||y>235||(pipeX<48&&pipeX>12&&(y<gapY-32||y>gapY+32))){gameOver(score,"FLAPPY OVER");return score;}
    tft.fillRect(0,28,240,212,TFT_BLACK);tft.fillRect(pipeX,28,14,gapY-32,TFT_GREEN);tft.fillRect(pipeX,gapY+32,14,240-gapY-32,TFT_GREEN);tft.fillCircle(48,y,6,col);
    tft.setTextSize(1);tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setCursor(4,30);tft.printf("%d",score);
  }
}
int runCatch(uint8_t d,uint16_t col){
  int px=120,fx=random(10,230),fy=35,score=0;uint32_t last=0;drawHeader("CATCH IT",col);
  while(true){
    if(edge(PIN_LEFT))px-=12;if(edge(PIN_RIGHT))px+=12;px=constrain(px,15,225);
    if(abortGame())return -1;
    if(millis()-last<24)continue;last=millis();fy+=2+d;
    if(fy>235){fy=30;fx=random(10,230);}
    if(fy>205&&abs(fx-px)<18){score+=10;fy=30;fx=random(10,230);}
    tft.fillRect(0,28,240,212,TFT_BLACK);tft.fillRect(px-15,218,30,7,col);tft.fillCircle(fx,fy,5,TFT_YELLOW);
    tft.setTextSize(1);tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setCursor(4,30);tft.printf("%d",score);
  }
}
int runReaction(uint8_t d,uint16_t col){
  drawHeader("REACTION",col);centerText("Wait for GREEN",65,TFT_WHITE,2);centerText("then press A",95,TFT_WHITE,2);
  delay(random(1200,3500));tft.fillScreen(TFT_GREEN);uint32_t start=millis();
  while(!pressed(PIN_A)){if(pressed(PIN_B)||pressed(PIN_SELECT))return -1;if(millis()-start>5000){gameOver(0,"TOO SLOW");return 0;}delay(1);}
  int ms = millis() - start;
  char msg[24];
  snprintf(msg, sizeof(msg), "MS %d", ms);
  gameOver(5000 - ms, msg);
  return 5000 - ms;
}
int runMaze(uint8_t d,uint16_t col){
  const uint8_t map[8][12]={
    {1,1,1,1,1,1,1,1,1,1,1,1},{1,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,1,1,0,1},{1,0,1,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,1,0,1,0,1},{1,0,0,0,0,0,0,1,0,0,0,1},
    {1,1,1,1,1,1,0,0,0,1,0,1},{1,1,1,1,1,1,1,1,1,1,1,1}
  };
  int x=1,y=1,moves=0;drawHeader("MAZE ESCAPE",col);
  while(true){
    int nx=x,ny=y;if(edge(PIN_LEFT))nx--;if(edge(PIN_RIGHT))nx++;if(edge(PIN_UP))ny--;if(edge(PIN_DOWN))ny++;
    if(abortGame())return -1;
    if(nx>=0&&nx<12&&ny>=0&&ny<8&&!map[ny][nx]){x=nx;y=ny;moves++;}
    tft.fillRect(0,28,240,212,TFT_BLACK);
    for(int r=0;r<8;r++)for(int c=0;c<12;c++)if(map[r][c])tft.fillRect(c*20,32+r*24,19,23,TFT_DARKGREY);
    tft.fillRect(200,152,19,23,TFT_GREEN);tft.fillCircle(x*20+10,32+y*24+12,7,col);
    if(x==10&&y==5){gameOver(1000-moves,"ESCAPED!");return 1000-moves;}delay(90);
  }
}
int runTarget(uint8_t d,uint16_t col){
  int x=random(25,215),y=random(50,205),score=0,shots=0;uint32_t end=millis()+30000;
  drawHeader("TARGET PRACTICE",col);
  while(millis()<end){
    if(edge(PIN_LEFT))x=max(15,x-12);if(edge(PIN_RIGHT))x=min(225,x+12);
    if(edge(PIN_UP))y=max(35,y-12);if(edge(PIN_DOWN))y=min(220,y+12);
    if(edge(PIN_A)){shots++;if(abs(x-120)<18&&abs(y-120)<18)score+=10;}
    if(abortGame())return -1;
    tft.fillRect(0,28,240,212,TFT_BLACK);
    tft.drawCircle(120,120,22,TFT_RED);
    tft.drawCircle(120,120,12,TFT_RED);
    tft.drawCircle(120,120,3,TFT_RED);
    tft.drawLine(x-8,y,x+8,y,col);
    tft.drawLine(x,y-8,x,y+8,col);
    tft.setTextColor(TFT_WHITE,TFT_BLACK);tft.setTextSize(1);tft.setCursor(4,30);tft.printf("SCORE %d  SHOTS %d",score,shots);delay(30);
  }
  gameOver(score,"TIME UP");return score;
}
int playGame(int idx){
  GameDef g=games[idx];
  switch(g.engine){
    case 0:return runSnake(g.difficulty,g.color);
    case 1:return runPong(g.difficulty,g.color);
    case 2:return runBreakout(g.difficulty,g.color);
    case 3:return runShooter(g.difficulty,g.color);
    case 4:return runDodger(g.difficulty,g.color);
    case 5:return runFlappy(g.difficulty,g.color);
    case 6:return runCatch(g.difficulty,g.color);
    case 7:return runReaction(g.difficulty,g.color);
    case 8:return runMaze(g.difficulty,g.color);
    default:return runTarget(g.difficulty,g.color);
  }
}
void setup(){
  pinMode(PIN_UP,INPUT_PULLUP);pinMode(PIN_DOWN,INPUT_PULLUP);pinMode(PIN_LEFT,INPUT_PULLUP);pinMode(PIN_RIGHT,INPUT_PULLUP);
  pinMode(PIN_A,INPUT_PULLUP);pinMode(PIN_B,INPUT_PULLUP);pinMode(PIN_START,INPUT_PULLUP);pinMode(PIN_SELECT,INPUT_PULLUP);
  randomSeed(esp_random());tft.init();tft.setRotation(0);tft.fillScreen(TFT_BLACK);
  centerText("RETRO GAME",70,TFT_CYAN,3);centerText("52 CHALLENGES",112,TFT_WHITE,2);centerText("Starting...",160,TFT_GREEN,2);delay(1000);drawMenu();
}
void loop(){
  if(edge(PIN_UP)){selected=(selected-1+GAME_COUNT)%GAME_COUNT;drawMenu();}
  if(edge(PIN_DOWN)){selected=(selected+1)%GAME_COUNT;drawMenu();}
  if(edge(PIN_LEFT)){selected=(selected-6+GAME_COUNT)%GAME_COUNT;drawMenu();}
  if(edge(PIN_RIGHT)){selected=(selected+6)%GAME_COUNT;drawMenu();}
  if(edge(PIN_A)||edge(PIN_START)){
    int score=playGame(selected);
    waitRelease();
    if(score>=0){
      while(true){
        if(edge(PIN_A)||edge(PIN_START)){score=playGame(selected);waitRelease();if(score<0)break;}
        if(edge(PIN_B)||edge(PIN_SELECT))break;
        delay(10);
      }
    }
    drawMenu();
  }
  if(edge(PIN_B)||edge(PIN_SELECT)){drawMenu();}
  delay(5);
}
