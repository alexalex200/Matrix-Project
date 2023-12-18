#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

const byte dinPin = A3;    // pin 12 is connected to the MAX7219 pin 1
const byte clockPin = A5;  // pin 11 is connected to the CLK pin 13
const byte loadPin = A4;   // pin 10 is connected to LOAD pin 12

const byte pinX = A0;   // A0 - analog pin connected to X output
const byte pinY = A1;   // A1 - analog pin connected to Y output
const byte pinSW = 13;  // digital pin connected to switch output
const byte pinB = 2;

const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;

const byte buzzerPin = 11;

const byte backlightPin = 3;

char playerName[2]="";
unsigned long lastSwState = 0, lastBState = 0;
byte swState = LOW, bState = LOW;

int pauseGame = 0;
LedControl lc = LedControl(dinPin, clockPin, loadPin);  //DIN, CLK, LOAD, No. DRIVER
byte matrixBrightness;
byte lcdBrightness;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

byte semAnimation = 0;
int frame = 0, startAnimationTime = 0;

byte semGenRandomMatrix = 1;

const byte boatL[3] = { 1, 2, 3 };
byte nrBoat[3] = { 4, 3, 2 };
byte boatsDestroyed = 0, boatsDestroyed2 = 0;
int rotation = 0, indexL = 2, indexNr = nrBoat[indexL];

byte cursorX = 0, cursorRandomX = 0, drumCursorRandomX = 0;
byte cursorY = 0, cursorRandomY = 0, drumCursorRandomY = 0;
unsigned long lastMove = 0;
int moveDelay = 500;

int xValue = 0;
int yValue = 0;

int minThreshold = 400;
int maxThreshold = 600;

byte blinkPlayer = false, blinkBoat = false;
unsigned long blinkPlayerTime = 0, blinkBoatTime = 0, animationTime = 0, cursorAnimationTime = 0;
const int playerBlinkTime = 100, bombBlinkTime = 200, boatBlinkTime = 300, buttonDelay = 1000, animationDelay = 200;


int menu = 0, subMenu = 0, pas = 1;
byte semGamePlaying = 0,semSound;
const byte matrixSize = 8;
byte matrix[matrixSize][matrixSize] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 }
};

byte matrix2[matrixSize][matrixSize] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 }
};

const uint64_t BOAT_IMAGES[] = {
  0x0000010000000000,
  0x0001030001000000,
  0x0103070003010000,
  0x03070f0007030100,
  0x070f1f010f070301,
  0x0f1f3f021e0e0602,
  0x1e3f7f043c1c0c04,
  0x3c7eff0878381808
};

const int BOAT_IMAGES_LEN = sizeof(BOAT_IMAGES) / 8;

byte boatDestroyed(int x, int y,byte matrix2[matrixSize][matrixSize]) {
  int poz = 0;
  while (matrix2[x + poz][y] != 2 && matrix2[x + poz][y] != 4 && x + poz < matrixSize) {
    if (matrix2[x + poz][y] == 1)
      return 0;
    poz++;
  }
  poz = 0;
  while (matrix2[x - poz][y] != 2 && matrix2[x - poz][y] != 4 && x - poz >= 0) {
    if (matrix2[x - poz][y] == 1)
      return 0;
    poz++;
  }
  poz = 0;
  while (matrix2[x][y + poz] != 2 && matrix2[x][y + poz] != 4 && y + poz < matrixSize) {
    if (matrix2[x][y + poz] == 1)
      return 0;
    poz++;
  }
  poz = 0;
  while (matrix2[x][y - poz] != 2 && matrix2[x][y - poz] != 4 && y - poz >= 0) {
    if (matrix2[x][y - poz] == 1)
      return 0;
    poz++;
  }
  return 1;
}

byte possiblePoz(int x, int y) {
  if (x < 0 || x >= matrixSize || y < 0 || y >= matrixSize)
    return 0;
  for (int i = 0; i < boatL[indexL]; i++) {
    if ((!rotation && x + i >= matrixSize) || (rotation && y + i >= matrixSize))
      return 0;
  }
  return 1;
}

byte drawCursor(int x, int y, int cursorX, int cursorY) {
  if ((abs(x - cursorX) + abs(y - cursorY)) != 1)
    return 0;
  return 1;
}

byte drawBoat(int cursorX, int cursorY, int x, int y) {
  if ((rotation && y >= cursorY && y < cursorY + boatL[indexL] && x == cursorX) || (!rotation && x >= cursorX && x < cursorX + boatL[indexL] && y == cursorY)) {
    return 1;
  }
  return 0;
}

byte setBoat(byte matrix[matrixSize][matrixSize], int x, int y) {
  for (int i = 0; i < boatL[indexL]; i++)
    if ((!rotation && matrix[x + i][y]) || (rotation && matrix[x][y + i]) || (!rotation && x + i >= matrixSize) || (rotation && y + i >= matrixSize))
      return 0;
  for (int i = 0; i < matrixSize; i++)
    for (int j = 0; j < matrixSize; j++)
      if (drawBoat(x, y, i, j))
        matrix[i][j] = 1;
  for (int i = 0; i < matrixSize; i++)
    for (int j = 0; j < matrixSize; j++) {
      if (!matrix[i][j]) {
        for (int k = max(0, i - 1); k <= min(i + 1, matrixSize - 1); k++)
          for (int l = max(0, j - 1); l <= min(j + 1, matrixSize - 1); l++)
            if (matrix[k][l] == 1)
              matrix[i][j] = 2;
      }
    }
  return 1;
}

void randomMatrix(byte matrix[matrixSize][matrixSize]) {
  indexL = 2;
  indexNr = nrBoat[indexL];

  byte cursorRandomX;
  byte cursorRandomY;

  byte sem = 1;
  while (sem) {
    Serial.println(indexL);
    cursorRandomX = random(0, matrixSize);
    cursorRandomY = random(0, matrixSize);
    rotation = random(0, 2);

    if (setBoat(matrix, cursorRandomX, cursorRandomY)) {
      for (int i = 0; i < matrixSize; i++) {
        for (int j = 0; j < matrixSize; j++)
          Serial.print(matrix[i][j]);
        Serial.println();
      }
      Serial.println();
      indexNr--;
      if (indexL == 0 && indexNr == 0)
        sem = 0;
      if (indexNr == 0 && indexL != 0) {
        indexL--;
        indexNr = nrBoat[indexL];
      }
    }
  }
}

byte buttonPressListen() {
  if (bState == true && millis() - lastBState > buttonDelay) {
    lastBState = millis();
    return true;
  }
  return false;
}

byte swPressListen() {
  if (swState == true && millis() - lastSwState > buttonDelay) {
    lastSwState = millis();
    return true;
  }
  return false;
}

char joyStickListen() {
  if (xValue < minThreshold && millis() - lastMove > moveDelay) {
    lastMove = millis();
    return 'l';
  }
  if (xValue > maxThreshold && millis() - lastMove > moveDelay) {
    lastMove = millis();
    return 'r';
  }
  if (yValue < minThreshold && millis() - lastMove > moveDelay) {
    lastMove = millis();
    return 'u';
  }
  if (yValue > maxThreshold && millis() - lastMove > moveDelay) {
    lastMove = millis();
    return 'd';
  }
  return 'n';
}

void displayMatrix(byte matrix[matrixSize][matrixSize], int cursorX, int cursorY, int pausetime) {
  while (pausetime) {
    Serial.println(pausetime);
    for (int row = 0; row < matrixSize; row++)
      for (int col = 0; col < matrixSize; col++) {
        if (drawCursor(row, col, cursorX, cursorY)) {
          if (blinkPlayer)
            lc.setLed(0, row, col, true);
          else
            lc.setLed(0, row, col, false);
        } else {
          lc.setLed(0, row, col, false);
          if (matrix[row][col] == 3) {
            if (blinkBoat)
              lc.setLed(0, row, col, true);
            else
              lc.setLed(0, row, col, false);
          }
          if (matrix[row][col] == 4)
            lc.setLed(0, row, col, true);
        }
      }
    if (millis() - blinkPlayerTime > playerBlinkTime) {
      blinkPlayer = !blinkPlayer;
      blinkPlayerTime = millis();
    }

    if (millis() - blinkBoatTime > boatBlinkTime) {
      blinkBoat = !blinkBoat;
      blinkBoatTime = millis();
    }
    pausetime--;
  }
  
}

void displayAnimation(uint64_t image[], int imgLen, int duration) {

  if (millis() - animationTime > animationDelay) {
    for (int i = 0; i < 8; i++) {
      byte row = (image[frame] >> i * 8) & 0xFF;
      for (int j = 0; j < 8; j++) {
        lc.setLed(0, i, j, bitRead(row, j));
      }
    }

    frame++;
    if (frame == imgLen)
      frame = 0;
    animationTime = millis();
  }
}

void displayMenu() {
  if (menu == 0) {
    lcd.setCursor(3, 0);
    lcd.print("SeaBattle");

    lcd.setCursor(2, 1);
    lcd.write("Press Start");

    if (buttonPressListen()) {
      menu++;
      lcd.clear();
    }
  }

  if (menu == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Menu:");
    lcd.setCursor(0, 1);

    if (subMenu == 0)
      lcd.print("Play           ");
    if (subMenu == 1)
      lcd.print("Settings       ");
    if (subMenu == 2)
      lcd.print("About          ");
    if(subMenu==3)
      lcd.print("How to play    ");

    char direction = joyStickListen();
    if (direction == 'd')
      subMenu++;
    if (direction == 'u')
      subMenu--;

    if (subMenu < 0)
      subMenu = 3;
    if (subMenu > 3)
      subMenu = 0;

    if (buttonPressListen()) {
      menu = menu + subMenu + 1;
      if(subMenu==3)
        menu=9;
      subMenu = 0;
      lcd.clear();
    }
  }

  if (menu == 2) {
    semGamePlaying = 1;
  }

  if (menu == 3) {
    lcd.setCursor(0, 0);
    lcd.print("Settings:");
    lcd.setCursor(0, 1);

    if (subMenu == 0)
      lcd.print("LCD Brightness     ");
    if (subMenu == 1)
      lcd.print("Matrx Brightness");
    if (subMenu == 2)
      lcd.print("Sound             ");
    if (subMenu == 3)
      lcd.print("Set Name          ");

    char direction = joyStickListen();
    if (direction == 'd')
      subMenu++;
    if (direction == 'u')
      subMenu--;

    if (subMenu < 0)
      subMenu = 3;
    if (subMenu > 3)
      subMenu = 0;

    if (buttonPressListen()) {
      menu = menu + 2 + subMenu;
      subMenu = 0;
      if(menu==7)
        subMenu=semSound;
      if(menu==8)
        lcd.cursor();
      lcd.clear();
    }
  }

  if (menu == 4) {
    if (subMenu == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Game Name:          ");
      lcd.setCursor(0, 1);
      lcd.print("Sea Battle          ");
    }
    if (subMenu == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Author:          ");
      lcd.setCursor(0, 1);
      lcd.print("Anca Alexandru   ");
    }
    if (subMenu == 2) {
      lcd.setCursor(0, 0);
      lcd.print("GitHub username:   ");
      lcd.setCursor(0, 1);
      lcd.print("alexalex200        ");
    }

    char direction = joyStickListen();
    if (direction == 'd')
      subMenu++;
    if (direction == 'u')
      subMenu--;

    if (subMenu < 0)
      subMenu = 2;
    if (subMenu > 2)
      subMenu = 0;

    if (buttonPressListen()) {
      menu = 1;
      subMenu = 0;
      lcd.clear();
    }
  }

  if (menu == 5) {
    lcd.setCursor(0, 0);
    lcd.print("LCD Brightness");
    lcd.setCursor(0, 1);

    for (int i = 0; i < 5; i++)
      if (i < lcdBrightness)
        lcd.write(255);
      else
        lcd.print(" ");

    char direction = joyStickListen();
    if (direction == 'r' && lcdBrightness < 5) {
      lcdBrightness++;
      EEPROM.update(11, lcdBrightness);
    }
    if (direction == 'l' && lcdBrightness > 0) {
      lcdBrightness--;
      EEPROM.update(11, lcdBrightness);
    }
    if (buttonPressListen()) {
      menu = 1;
      subMenu=0;
      lcd.clear();
    }
  }

  if (menu == 6) {
    lcd.setCursor(0, 0);
    lcd.print("Matrx Brightness");
    lcd.setCursor(0, 1);

    for (int i = 0; i < 5; i++)
      if (i < matrixBrightness)
        lcd.write(255);
      else
        lcd.print(" ");

    char direction = joyStickListen();
    if (direction == 'r' && matrixBrightness < 5) {
      matrixBrightness++;
      EEPROM.update(10, matrixBrightness);
    }
    if (direction == 'l' && matrixBrightness > 0) {
      matrixBrightness--;
      EEPROM.update(10, matrixBrightness);
    }
    if (buttonPressListen()) {
      menu = 1;
      lcd.clear();
    }
  }
  if (menu == 7) {
    lcd.setCursor(0, 0);
    lcd.print("Sound");
    lcd.setCursor(0, 1);

    if(subMenu==0){
      lcd.setCursor(0,1);
      lcd.print("NO ");
    }

    if(subMenu==1){
      lcd.setCursor(0,1);
      lcd.print("YES");
    }

    char direction = joyStickListen();
    if (direction == 'd')
      subMenu++;
    if (direction == 'u')
      subMenu--;

    if (subMenu < 0)
      subMenu = 1;
    if (subMenu > 1)
      subMenu = 0;

    if (buttonPressListen()) {
      menu = 1;
      EEPROM.update(12, subMenu);
      semSound=subMenu;
      subMenu=0;
      lcd.clear();
    }
  }

  if (menu == 8) {
    lcd.setCursor(0, 0);
    lcd.print("Set Name");
    lcd.setCursor(0, 1);
    lcd.print(playerName);
    lcd.print("           ");

    lcd.setCursor(subMenu,1);
    delay(20);

    char direction = joyStickListen();
    if (direction == 'r')
      subMenu++;
    if (direction == 'l')
      subMenu--;
    if (direction == 'd')
    {
      playerName[subMenu]--;
      if(playerName[subMenu]<65)
        playerName[subMenu]=90;
    }
    if (direction == 'u')
    {
      playerName[subMenu]++;
      if(playerName[subMenu]>90)
        playerName[subMenu]=65;
    }

    if (subMenu < 0)
      subMenu = 2;
    if (subMenu > 2)
      subMenu = 0;

    if (buttonPressListen()) {
      menu = 1;
      EEPROM.update(20,playerName[0]);
      EEPROM.update(30,playerName[1]);
      EEPROM.update(40,playerName[2]);
      subMenu=0;
      lcd.clear();
      lcd.noCursor();
    }
  }
  if(menu==9)
  {
    lcd.setCursor(0,0);
    char text[]="In each round, each player takes a turn to announce a target square in the opponent's grid which is to be shot at. The opponent announces whether or not the square is occupied by a ship, and if it is a “hit” they mark this on their own primary grid.";
    int lenText= sizeof(text)/sizeof(char);

    for (int i=0;i<16  && i+16*subMenu <lenText ;i++)
      lcd.print(text[i+16*subMenu]);
    lcd.setCursor(0,1);
    for (int i=0;i<16 && i+16*(subMenu+1) <lenText;i++)
      lcd.print(text[i+16*(subMenu+1)]);
    
    char direction = joyStickListen();
    if (direction == 'd')
      subMenu++;
    if (direction == 'u')
      subMenu--;

    if (subMenu < 0)
      subMenu = lenText/16;
    if (subMenu > lenText/16)
      subMenu = 0;
    
    if (buttonPressListen()) {
      menu = 1;
      subMenu=0;
      lcd.clear();
    }
  }

}
void displayGame() {
  if (pas == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Placing:");
    lcd.setCursor(0, 1);
    if (indexL == 2)
      lcd.print("Ship");
    if (indexL == 1)
      lcd.print("Boat");
    if (indexL == 0)
      lcd.print("Mine");
    for (int row = 0; row < matrixSize; row++)
      for (int col = 0; col < matrixSize; col++) {

        if (drawBoat(cursorX, cursorY, row, col)) {
          if (blinkBoat)
            lc.setLed(0, row, col, true);
          else
            lc.setLed(0, row, col, false);
        } else {
          if (matrix[row][col] == 1)
            lc.setLed(0, row, col, true);
          else
            lc.setLed(0, row, col, false);
        }
      }
    if (buttonPressListen()) {
      if (setBoat(matrix, cursorX, cursorY)) {
        cursorX = 0;
        cursorY = 0;
        rotation = 0;

        indexNr--;
        if (indexNr == 0 && indexL != 0) {
          indexL--;
          indexNr = nrBoat[indexL];
        }

        if (indexL == 0 && indexNr == 0) {
          pas = 2;
          if (semGenRandomMatrix) {
            semGenRandomMatrix = 0;
            cursorX = 0;
            cursorY = 0;
            randomMatrix(matrix2);
          }
          lcd.clear();
          for (int row = 0; row < matrixSize; row++)
            for (int col = 0; col < matrixSize; col++)
              lc.setLed(0, row, col, false);
        }
      }
    }

    if (swPressListen()) {
      rotation = !rotation;
      if (!possiblePoz(cursorX, cursorY))
        rotation = !rotation;
    }
  }

  if (pas == 2) {

    lcd.setCursor(0, 0);
    lcd.print("Boats left:");
    lcd.print(2 - boatsDestroyed);

    lcd.setCursor(0, 1);
    lcd.print("Playing:");
    lcd.print(playerName);

    for (int row = 0; row < matrixSize; row++)
      for (int col = 0; col < matrixSize; col++) {
        if (drawCursor(row, col, cursorX, cursorY)) {
          if (blinkPlayer)
            lc.setLed(0, row, col, true);
          else
            lc.setLed(0, row, col, false);
        } else {
          lc.setLed(0, row, col, false);
          if (matrix2[row][col] == 3) {
            if (blinkBoat)
              lc.setLed(0, row, col, true);
            else
              lc.setLed(0, row, col, false);
          }
          if (matrix2[row][col] == 4)
            lc.setLed(0, row, col, true);
        }
      }
    if (buttonPressListen()&&matrix2[cursorX][cursorY]!=3&&matrix2[cursorX][cursorY]!=4) {
      if (matrix2[cursorX][cursorY] == 1) {
        matrix2[cursorX][cursorY] = 3;
        if (boatDestroyed(cursorX, cursorY,matrix2)) {
          boatsDestroyed++;
          for (int i = 0; i < matrixSize; i++)
            for (int j = 0; j < matrixSize; j++) {
              if (matrix2[i][j] == 2) {
                for (int k = max(0, i - 1); k <= min(i + 1, matrixSize - 1); k++)
                  for (int l = max(0, j - 1); l <= min(j + 1, matrixSize - 1); l++)
                    if (matrix2[k][l] == 3)
                      matrix2[i][j] = 4;
              }
            }
        }
      } else
        matrix2[cursorX][cursorY] = 4;

      pas = 3;
      do {
        cursorRandomX = random(0, matrixSize - 1);
        cursorRandomY = random(0, matrixSize - 1);
      } while (matrix[cursorRandomX][cursorY] == 3 || matrix[cursorRandomX][cursorY] == 4);

      drumCursorRandomX = 0;
      drumCursorRandomY = 0;
      cursorAnimationTime = millis();
      Serial.println("pc");
      displayMatrix(matrix2, cursorX, cursorY, 100);

      if (boatsDestroyed == 2) {
        for (int row = 0; row < matrixSize; row++)
          for (int col = 0; col < matrixSize; col++) {
            matrix2[row][col] = 0;
            matrix[row][col] = 0;
            lc.setLed(0, row, col, false);
          }
        cursorX = 0;
        cursorY = 0;
        boatsDestroyed = 0;
        pas = 4;
        lcd.clear();
      }
    }
  }

  if (pas == 3) {

    lcd.setCursor(0, 0);
    lcd.print("Boats left:");
    lcd.print(2 - boatsDestroyed2);
    lcd.setCursor(0, 1);
    lcd.print("Playing:PC              ");

    for (int row = 0; row < matrixSize; row++)
      for (int col = 0; col < matrixSize; col++) {
        if (drawCursor(row, col, drumCursorRandomX, drumCursorRandomY)) {
          if (blinkPlayer)
            lc.setLed(0, row, col, true);
          else
            lc.setLed(0, row, col, false);
        } else {
          lc.setLed(0, row, col, false);
          if (matrix[row][col] == 3) {
            if (blinkBoat)
              lc.setLed(0, row, col, true);
            else
              lc.setLed(0, row, col, false);
          }
          if (matrix[row][col] == 4)
            lc.setLed(0, row, col, true);
        }
      }

    if (millis() - cursorAnimationTime > buttonDelay) {
      if (drumCursorRandomX < cursorRandomX)
        drumCursorRandomX++;
      if (drumCursorRandomX > cursorRandomX)
        drumCursorRandomX--;
      if (drumCursorRandomY < cursorRandomY)
        drumCursorRandomY++;
      if (drumCursorRandomY > cursorRandomY)
        drumCursorRandomY--;

      if (drumCursorRandomX == cursorRandomX && drumCursorRandomY == cursorRandomY) {
        if (matrix[cursorRandomX][cursorRandomY] == 1) {
          matrix[cursorRandomX][cursorRandomY] = 3;
          if (boatDestroyed(cursorRandomX, cursorRandomY,matrix)) {
            boatsDestroyed2++;
            for (int i = 0; i < matrixSize; i++)
              for (int j = 0; j < matrixSize; j++) {
                if (matrix[i][j] == 2) {
                  for (int k = max(0, i - 1); k <= min(i + 1, matrixSize - 1); k++)
                    for (int l = max(0, j - 1); l <= min(j + 1, matrixSize - 1); l++)
                      if (matrix[k][l] == 3)
                        matrix[i][j] = 4;
                }
              }
          }
        } else
          matrix[cursorRandomX][cursorRandomY] = 4;

        pas = 2;
        cursorX = 0;
        cursorY = 0;
        Serial.println("player");
        displayMatrix(matrix, cursorRandomX, cursorRandomY, 100);

        if (boatsDestroyed2 == 2) {
          for (int row = 0; row < matrixSize; row++)
            for (int col = 0; col < matrixSize; col++) {
              matrix2[row][col] = 0;
              matrix[row][col] = 0;
              lc.setLed(0, row, col, false);
            }
          cursorX = 0;
          cursorY = 0;
          boatsDestroyed2 = 0;
          pas = 4;
          lcd.clear();
        }
      }
      cursorAnimationTime = millis();
    }
  }

  if (pas == 4) {

    lcd.setCursor(0, 0);
    lcd.print("Restart?");
    lcd.setCursor(0, 1);

    if (subMenu == 0)
      lcd.print("YES");
    if (subMenu == 1)
      lcd.print("NO ");

    char direction = joyStickListen();
    if (direction == 'd')
      subMenu++;
    if (direction == 'u')
      subMenu--;

    if (subMenu < 0)
      subMenu = 1;
    if (subMenu > 1)
      subMenu = 0;

    if (buttonPressListen()) {
      if (subMenu == 0) {
        boatsDestroyed = 0;
        boatsDestroyed2 = 0;
        pas = 1;
        lcd.clear();
      } else {
        boatsDestroyed = 0;
        boatsDestroyed2 = 0;
        pas = 1;
        semGamePlaying = 0;
        semGenRandomMatrix = 1;
        menu = 0;
        lcd.clear();
      }
    }
  } else {
    char direction = joyStickListen();
    if (direction != 'n')
      Serial.println(direction);
    if (direction == 'u')
      if (possiblePoz(cursorX - 1, cursorY))
        cursorX -= 1;
    if (direction == 'd')
      if (possiblePoz(cursorX + 1, cursorY))
        cursorX += 1;
    if (direction == 'l')
      if (possiblePoz(cursorX, cursorY - 1))
        cursorY -= 1;
    if (direction == 'r')
      if (possiblePoz(cursorX, cursorY + 1))
        cursorY += 1;

    if (millis() - blinkPlayerTime > playerBlinkTime) {
      blinkPlayer = !blinkPlayer;
      blinkPlayerTime = millis();
    }

    if (millis() - blinkBoatTime > boatBlinkTime) {
      blinkBoat = !blinkBoat;
      blinkBoatTime = millis();
    }
  }
}
int indexMelodi=0;
int long pauseBetweenNotes=0,startNote=0;
void playSound() {
  int melody[] = {
    659, 698, 783, 659, 587, 523, 587, 659,                 // Notes E5, F5, G5, E5, D5, C5, D5, E5
    659, 698, 783, 659, 587, 523, 587, 659, 587, 523, 493,  // Repeat the first part and add some variation
    523, 587, 659, 523, 783, 783, 783, 880, 783, 659, 587,  // More variation in the second part
    523, 587, 659, 587, 523, 493, 523, 587                  // Repeat and end
  };

  int noteDurations[] = {
    4, 4, 4, 4, 4, 4, 4, 2,
    4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 2
  };

  if(millis()-startNote>pauseBetweenNotes)
  {
    if (melody[indexMelodi] == 0) {
      noTone(buzzerPin);
    } else {
      tone(buzzerPin, melody[indexMelodi], noteDurations[indexMelodi] * 100);
    }

    indexMelodi++;
    if(indexMelodi >= sizeof(melody) / sizeof(melody[0]))
      indexMelodi=0;
    startNote=millis();
    pauseBetweenNotes = noteDurations[indexMelodi] * 100 * 1.1;
  }
  
}

void setup() {
  lc.shutdown(0, false);  // turn off power saving, enables display
  lc.clearDisplay(0);     // clear screen
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  pinMode(dinPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(loadPin, OUTPUT);
  randomSeed(analogRead(2));
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(backlightPin, OUTPUT);

  // EEPROM.update(10,2);
  // EEPROM.update(11,2);

  EEPROM.get(10, matrixBrightness);
  EEPROM.get(11, lcdBrightness);
  EEPROM.get(12, semSound);
  // EEPROM.update(13, 65);
  // EEPROM.update(14, 65);
  // EEPROM.update(15, 65);

  EEPROM.get(20,playerName[0]);
  EEPROM.get(30,playerName[1]);
  EEPROM.get(40,playerName[2]);
  
}

void loop() {
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);
  swState = !digitalRead(pinSW);
  bState = !digitalRead(pinB);

  if(semSound)
    playSound();
  lc.setIntensity(0, map(matrixBrightness,0,4,0,15));
  analogWrite(backlightPin, map(lcdBrightness,0,5,0,250));
  Serial.println(lcdBrightness);
  if (!semGamePlaying) {
    startAnimationTime = millis();
    displayAnimation(BOAT_IMAGES, BOAT_IMAGES_LEN, 100);
    displayMenu();
  } else {
    displayGame();
  }
}