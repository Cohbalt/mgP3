#include <Arduino_LSM6DS3.h>
#include <CapacitiveSensor.h>

enum GameState {
  WAITING,
  PLAYING,
  VICTORY,
  GAMEOVER
};

enum KeyState {
  BROKEN,
  DANGER,
  SAFE,
  COMPLETED
};

GameState currentState;
bool gameOverM[] = {1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0};
int value = 0;
int prevMillis;
float angleX = 0, angleY = 0;
float savedAngleX = 0, savedAngleY = 0;
CapacitiveSensor sensor = CapacitiveSensor(3,4);

void vibrate(bool on) {
  pinMode(2, on);
}

bool checkHold() {
  bool check = sensor.capacitiveSensor(30) > 1000;
  if (check && savedAngleX == 0) {
    savedAngleX = angleX;
    savedAngleY = angleY;
  }
  else if (!check) {
    savedAngleX = 0;
    savedAngleY = 0;
  }
  return check;
}

KeyState checkAngles(int angleGoal) {
  float x, y, z, ax, ay, az, dt;

  IMU.readGyroscope(x, y, z);
  IMU.readAcceleration(ax, ay, az);
  
  unsigned long currentMillis = millis();
  dt = (currentMillis - prevMillis) / 1000.0;
  prevMillis = currentMillis;

  float accelAngleX = atan2(ay, az) * 180/PI;
  float accelAngleY = atan(-ax / sqrt(pow(ay, 2) + pow(az, 2))) * 180/PI;

  angleX = 0.95*(angleX + x*dt) + 0.05*accelAngleX;
  angleY = 0.95*(angleY + y*dt) + 0.05*accelAngleY;

  float trueX = (angleX >= -90) ? angleX - 90 : 270 - abs(angleX);

  Serial.print(trueX);
  Serial.print(" ");
  Serial.print(savedAngleY);
  Serial.print(" ");

  int gap = min(90, abs(savedAngleY - angleGoal));

  Serial.print(gap);
  Serial.print(" ");

  if (gap <= 10) {
    if (trueX <= -85) {
      return COMPLETED;
    }
    return SAFE;
  } 
  if (trueX <= -90 + gap - 10) {
    return BROKEN;
  }
  if (trueX <= (-90 + gap - 10) / 2) {
    return DANGER;
  }

  return SAFE;
}

void playSong(GameState state, int note){
  int *music;
  switch(state) {
    case 0: //Undertale
    case 1: //Mario
    case 2: break; //Other
  }
  vibrate(music[note]);
  delay(100);
}

void waiting() {
  while (!checkHold()) 
  {
  }
  currentState = PLAYING;
}

void gamePlay() {
  int angleGoal = random(-60, 60);
  int danger = 0;

  while (currentState == PLAYING) {
    KeyState key = checkAngles(angleGoal);
    if (checkHold()) {
      switch(key) {
        case SAFE:
          Serial.println("SAFE");
          vibrate(0);
          break;
        case DANGER: 
          Serial.println("DANGER");
          vibrate(1);
          break;
        case BROKEN: 
          Serial.println("BROKEN");
          vibrate(0);
          currentState = GAMEOVER;
          break;
        case COMPLETED:
          Serial.println("COMPLETE");
          vibrate(0);
          currentState = VICTORY;
          break;
      }
    }
    vibrate(0);
  }
}

void gameOver() {
  Serial.print("GAME OVER");
  delay(10000);
  playSong(currentState, 1);
}

void setup() {
  randomSeed(analogRead(0));
  value = 0;
  pinMode(2, OUTPUT);
  IMU.begin();

  prevMillis = millis();
}

void loop() {
  waiting();
  gamePlay();
  gameOver();
}
