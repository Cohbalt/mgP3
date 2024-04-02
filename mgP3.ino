#include <Arduino_LSM6DS3.h>
#include <CapacitiveSensor.h>

///ENUM Dictating the current looping state of the game.
enum GameState {
  WAITING,
  PLAYING,
  VICTORY,
  GAMEOVER
};

///ENUM Dictating the current state of the keyturn.
enum KeyState {
  BROKEN,
  DANGER,
  SAFE,
  COMPLETED
};

//Global GameState to hold the current loop the game is in.
GameState currentState;

//Tones for main menu jingle
int mainMenuTone[7] = {500, 1000, 1250, 1500, 800, 1500, 800};
int mainMenuDur[7] = {500, 250, 250, 64, 64, 64, 64};

//Used to hold the time for our integrals
int prevMillis;

//Holds the calculated angles for future calculation
float angleX = 0, angleY = 0;
float savedAngleX = 0, savedAngleY = 0;

//Capacitive Sensor setup
CapacitiveSensor sensor = CapacitiveSensor(3,4);

//Plays a tone for a specific amount of time (includes delay)
void playFor(int ton, int del) {
  tone(12, ton);
  delay(del);
  noTone(12);
}

//Vibrates the motor
void vibrate(bool on) {
  digitalWrite(2, on);
}
 
 //Checks our capacitive sensor to see if the user is holding it.
 //If this is the start of the hold, then we save the angleX and angleY
 //If this is the end of the hold, we clear the angleX and angleY.
 //We then return whehter the user is holding it.
bool checkHold() {
  bool check = sensor.capacitiveSensor(30) > 2000;
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

//Here we check the angles being delivered from the IMU.
//To do this, we retrieve the angular velocity from the IMU.
//We then retrieve the regular acceleration from the IMU.
//We then multiply the velocity of the IMU with the time to get rotational position.
//We calculate the angle of the regular acceleration, and use it as a correction factor for the velocity
//This works since, at rest, gravity will be pulling on the sensor causing acceleration in that direction.
//We then calculate the difference between our calculated vertical angle and the target vertical angle.
//Depending on this difference, the threshold for Danger and Broken becomes increasingly more strict.
//If the difference is < 10, then the key is able to turn fully without breaking.
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

  Serial.println(gap);

  if (gap <= 10) {
    if (trueX <= -85) {
      return COMPLETED;
    }
    return SAFE;
  } 
  if (trueX <= -90 + gap - 10) {
    return BROKEN;
  }
  if (trueX <= (-90 + gap - 10) * 2/3) {
    return DANGER;
  }
  return SAFE;
}

//Waiting loop that plays the main menu theme and warms up the angle checking system.
void waiting() {
  int lastMill = millis();
  int i = 0;
  angleX = angleY = 0;
  tone(12, mainMenuTone[i]);
  while (!checkHold()) 
  {
    int currMill = millis();
    if (currMill - lastMill > mainMenuDur[i]) {
      i = (i + 1) % 7;
      tone(12, mainMenuTone[i]);
      lastMill = millis();
    }
  }

  playFor(1500, 200);
  playFor(800, 200);
  
  for (int i = 0; i < 50; i++) {
    checkAngles(0);
  }

  currentState = PLAYING;
}

//Continuously checks the angle to ensure proper angle measurement.
//Only when the button is being held does the KeyState actually affect anything.
//Uses the KeyState to determine when to vibrate and when to progress.
void gamePlay() {
  int angleGoal = random(-60, 60);
  Serial.println(angleGoal);
  int danger = 0;

  while (currentState == PLAYING) {
    KeyState key = checkAngles(angleGoal);
    if (checkHold()) {
      switch(key) {
        case SAFE:
          ////Serial.println("SAFE");
          vibrate(0);
          break;
        case DANGER: 
          //Serial.println("DANGER");
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
    else {
      vibrate(0);
    }
  }
  vibrate(0);
}

//Based on whether the gamestate is VICTORY or GAMEOVER, sends a tone to the user to indicate loss or win.
void gameOver() {
  switch (currentState) {
    case GAMEOVER:
      playFor(500, 500);
      playFor(250, 250);
      break;
    case VICTORY:
      playFor(1000, 500);
      playFor(1500, 250);
      break;
  }
  delay(2000);
}

//Sets up the IMU, random number generator, and pins.
void setup() {
  randomSeed(analogRead(0));
  pinMode(2, OUTPUT);
  IMU.begin();
  prevMillis = millis();
}

//Loops through the main menu, gameplay, and gameover sections.
void loop() {
  waiting();
  gamePlay();
  gameOver();
}
