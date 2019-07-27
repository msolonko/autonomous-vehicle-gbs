const int dirControlLeft = 3;     //left rear wheel direction            
const int pwmControlLeft = 10;    //left rear wheel speed control  
const int dirControlRight = 34;     //right rear wheel direction           
const int pwmControlRight = 11;    //right rear wheel speed control
const int dirControlFront = 32;     //front wheel direction           
const int pwmControlFront = 9;    //front wheel speed control
const int wheelSpeed = 75;     //maximum wheel speed
const int dirControlLinAct = 36;     //rear wheel direction 
const int pwmControlLinAct = 8;     //rear wheel speed 
bool ACT_EXTENDED = false;
const float FRONT_WHEEL_SPEED_MULTIPLIER = 1; //to account for slower turning speed
const int BUZZER = 50;
const int LIGHTS = 53;

int angleTurn = 0;     //degree of front wheel rotation, maximum value of 72 degrees; positive-CW from a top view, negative-CCW
int frontWheelPos = 0;
int oldWheelPos = 0; //keeps track of angle
int newWheelPos = 0;

   
const int begPoint = 820;
const int midPoint = 1510;
const int endPoint = 2280;

const int LED_BLUE = 2;
const int LED_RED = 12;
const int LED_GREEN = 13;
 
float leftSpeedMult = 0;
float rightSpeedMult =  0;

#include <Servo.h>
Servo AIVDMotor;
const int servoPin = 5;

/*** all variables below are for millis() timing ***/

bool avail = true;

//parallel parking
int parallel_delay = 15000;
bool parallel_routine = false;
unsigned long parallel_starting = 0;

//initial reset
int reset_delay = 15000;
bool resetting = false;
unsigned long reset_starting = 0;

//alpha
int alpha_delay = 0;
bool alpha_working = false;
unsigned long alpha_starting = 0;
int alpha_step = 0;
bool alpha_parking = false;

//beep
int beep_delay = 1000;
bool beep_working = false;
unsigned long beep_starting = 0;

//acceleration
int speed_goal = 0;
int current_speed = 0;
unsigned long acceleration_starting = 0;
int acceleration_delay = 6;

void setup() {         
  pinMode(dirControlLeft, OUTPUT);
  pinMode(dirControlRight, OUTPUT);
  pinMode(pwmControlLeft, OUTPUT);
  pinMode(pwmControlRight, OUTPUT);
  pinMode(dirControlFront, OUTPUT);
  pinMode(pwmControlFront, OUTPUT);
  pinMode(dirControlLinAct, OUTPUT);
  pinMode(pwmControlLinAct, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LIGHTS, OUTPUT);
  AIVDMotor.attach(servoPin);
  Serial.begin(115200);
  avail = false;
  resetWheels();
}

//-------------------------------------------------------------
//Main program
//-------------------------------------------------------------

void loop() 
{
  if(parallel_routine && millis() - parallel_starting > parallel_delay){
    parallelPark(true, 2); //step number two, boolean doesn't matter because direction is already set
    parallel_routine = false;
  }
  if(resetting && millis() - reset_starting > reset_delay){
    resetting = false;
    RED_OFF();
    avail = true;
  }
  if(alpha_working && millis() - alpha_starting > alpha_delay){
    alpha_step += 1;
    if(alpha_parking){
      alphaPark(alpha_step);
    }
    else{
      alphaTurn(alpha_step);
    }
  }
  if(beep_working && millis() - beep_starting > beep_delay){
    beep_working = false;
    digitalWrite(BUZZER, LOW);
  }
  
  
  if(current_speed != speed_goal && millis() - acceleration_starting > acceleration_delay){
    //depending if acceleration is positive or negative, adjusts change variable, then adjusts current speed and updates motors
    int change = (speed_goal > current_speed) ? 1 : -1;
    current_speed += change;

    //checks if direction change to update the digital pins
    if((current_speed - change >= 0 && current_speed < 0) || (current_speed - change <= 0 && current_speed > 0)){
      digitalWrite(dirControlLeft, current_speed < 0);
      digitalWrite(dirControlRight, current_speed < 0);
      digitalWrite(dirControlFront, current_speed < 0);
    }
    
    analogWrite(pwmControlLeft, abs(int(current_speed*leftSpeedMult)));
    analogWrite(pwmControlRight, abs(int(current_speed*rightSpeedMult)));
    analogWrite(pwmControlFront, abs(int(current_speed*FRONT_WHEEL_SPEED_MULTIPLIER))); 
    acceleration_starting = millis();
  }
  
  
  if (Serial.available() > 0) {
    // read the incoming byte:
    char command = Serial.read();
    
    if(command == 'b'){
      BEEP();
    }
    
    if(command == 'l'){
      if (Serial.parseInt() == 1){
        digitalWrite(LIGHTS, HIGH);
      }
      else if (Serial.parseInt() == 0){
        digitalWrite(LIGHTS, LOW);
      }
    }
    
    if(command == 'd'){
      // Serial.println("Move command received");
      // DO NOT PRINT TO SERIAL!!! BLOCKING ISSUES!
      moveCar(Serial.parseInt());
    }
    
    if(command == 's'){
      setFrontWheelDir(Serial.parseInt());
    }
    
    if(command == 'p'){
      parallelPark(false, 1);
    }
    if(command == 'r'){
      actuatorsRetract();
    }
    if(avail && command == 'z'){ //alphaTurn
      avail = false;
      alphaTurn(1);
    }
    if(avail && command == 'y'){ //alphaPark
      avail = false;
      alphaPark(1);
    }
  }

}


//-------------------------------------------------------------
//Challenge Code 
//-------------------------------------------------------------

void alphaPark(int stepNum){
   /* ALPHA CHALLENGE
   *  TASK:
   *  Wait 5 seconds
   *  Forward 3 meters
   *  Right 3 meters
   */
  int DRIVE_TIME=7500; //estimated time it takes for 3 meters
  if(stepNum == 1){
    alpha_parking = true;
    alpha_working = true;
    alpha_step = 1;
    RED_ON(); //indicates 5 second wait period
    setFrontWheelDir(0);
    alpha_delay = 5000;
  }
  else if(stepNum == 2){
    RED_OFF();
    GREEN_ON(); //indicates beginning of motion
    moveCar(wheelSpeed);
    alpha_delay = DRIVE_TIME;
  }
  else if(stepNum == 3){
    stopCar();
    parallelPark(true, 1);
    alpha_delay = 100;
  }
  else if(stepNum == 4){
    if(parallel_routine){
      alpha_step = 3;
      alpha_delay = 100;
    }
    else{
      alpha_delay = DRIVE_TIME;
    }
  }
  else if(stepNum == 5){
    stopCar();
    GREEN_OFF();
    alpha_working = false;
    avail = true;
  }
  alpha_starting = millis();
}

void alphaTurn(int stepNum){
   /* ALPHA CHALLENGE
   *  TASK:
   *  Wait 5 seconds
   *  Forward 3 meters
   *  Right 3 meters
   */
  int DRIVE_TIME=7500; //estimated time it takes for 3 meters
  if(stepNum == 1){
    alpha_parking = false;
    RED_ON(); //indicates 5 second wait period
    alpha_working = true;
    alpha_step = 1;
    setFrontWheelDir(0);
    alpha_delay = 5000;
  }
  else if(stepNum == 2){
    RED_OFF();
    GREEN_ON(); //indicates beginning of motion
    moveCar(wheelSpeed);
    alpha_delay = DRIVE_TIME;
  }
  else if(stepNum == 3){
    changeWheelPos(72);
    alpha_delay = 3400;
  }
  else if(stepNum == 4){
    changeWheelPos(0);
    alpha_delay = DRIVE_TIME;
  }
  else if(stepNum == 5){
    stopCar();
    GREEN_OFF();
    alpha_working = false;
    avail = true;
  }
  alpha_starting = millis();
}

//-------------------------------------------------------------
//Set Front Wheel Direction and Speeds 
//-------------------------------------------------------------
void setFrontWheelDir(int value){
  if (value == 0){    //Forward orientation --> CW == 0 and angleTurnCCW == 0
    leftSpeedMult = 1.0;
    rightSpeedMult = 1.0;
    frontWheelPos = midPoint;
  }
  else if(value > 0){
   leftSpeedMult = (32/tan(value*.0175) + 10)/(32/sin(value*.0175));
   rightSpeedMult = (32/tan(value*.0175) - 10)/(32/sin(value*.0175));
   frontWheelPos = midPoint - (midPoint-begPoint)*float(value)/90;  //uses midPoint and begPoint to calulate position based on angle
  }
  else if(value < 0){
     leftSpeedMult = (32/tan(-1*value*.0175) - 10)/(32/sin(-1*value*.0175));
     rightSpeedMult = (32/tan(-1*value*.0175) + 10)/(32/sin(-1*value*.0175));
     frontWheelPos = -(endPoint-midPoint)*float(value)/90 + midPoint;  //uses midPoint and endPoint to calulate position based on angle
  }
  AIVDMotor.writeMicroseconds(frontWheelPos);
  oldWheelPos = value; //updates angle value
}

void changeWheelPos(int nWPos){
  //makes tiny changes to direction for smooth turning
  int o = oldWheelPos;
  if (nWPos < oldWheelPos){
    for (int i = o; i >= nWPos; i--){
      changeWheelPosHelper(i);
      delay(8); 
    }
  }
  else{
     for (int i = o; i <= nWPos; i++){
      changeWheelPosHelper(i);
      delay(8);  
    }
  }
} 

  void changeWheelPosHelper(int val){
    //not used when programming, just used inside other functions
    setFrontWheelDir(val);
    analogWrite(pwmControlLeft, leftSpeedMult * wheelSpeed);
    analogWrite(pwmControlRight, rightSpeedMult * wheelSpeed);
    analogWrite(pwmControlFront, wheelSpeed*FRONT_WHEEL_SPEED_MULTIPLIER);
  }
  
  void parallelPark(bool LEFT, int stepNum){
    //set direction and setup servo
    //step 1 sets directions, step 2 begins motion
    if(stepNum == 1){
      digitalWrite(dirControlLeft, LEFT);     
      digitalWrite(dirControlRight, !LEFT);
      digitalWrite(dirControlFront, LEFT);
      AIVDMotor.writeMicroseconds(begPoint);
      leftSpeedMult=1; //for smooth stopping
      rightSpeedMult=1;
      //orients actuators and servo for sideways motion
      if(!ACT_EXTENDED){
        actuatorsExtend();
        parallel_delay = 15000;
      }
      else{
        parallel_delay = 3000;
      }
      parallel_routine = true;
      parallel_starting = millis();
    }
    else if(stepNum == 2){
    //start motion
      for (int i = 0; i <= wheelSpeed; i++){
        analogWrite(pwmControlLeft, i);
        analogWrite(pwmControlRight, i);
        analogWrite(pwmControlFront, i*FRONT_WHEEL_SPEED_MULTIPLIER);
        delay(5); 
      }
      current_speed = wheelSpeed;
      speed_goal = wheelSpeed;
    }
  }
  
  void moveCar(int power){   //Moves forward or backward
    //direction setting
    speed_goal = constrain(power,-212,212); // 212 is 255/1.2
    acceleration_starting = millis();
  }


void resetWheels(){
  //prepares wheels for forward motion
  RED_ON();
  actuatorsRetract();
  resetting = true;
  setFrontWheelDir(0);
  reset_starting = millis();
  digitalWrite(LIGHTS,LOW);
}

void stopCar(){
  //stops car
  for (int i = wheelSpeed; i >= 0; i--){
    analogWrite(pwmControlLeft, int(i*abs(leftSpeedMult)));
    analogWrite(pwmControlRight, int(i*abs(rightSpeedMult)));
    analogWrite(pwmControlFront, int(i*FRONT_WHEEL_SPEED_MULTIPLIER));
    delay(5); 
  }
  current_speed = 0;
}


//status LED abstraction
void RED_ON(){
  LED_FUNC(LED_RED, HIGH);
}
void RED_OFF(){
  LED_FUNC(LED_RED, LOW);
}
void GREEN_ON(){
  LED_FUNC(LED_GREEN, HIGH);
}
void GREEN_OFF(){
  LED_FUNC(LED_GREEN, LOW);
}
void BLUE_ON(){
  LED_FUNC(LED_BLUE, HIGH);
}
void BLUE_OFF(){
  LED_FUNC(LED_BLUE, LOW);
}

void actuatorsExtend(){
   digitalWrite(dirControlLinAct, LOW);
   analogWrite(pwmControlLinAct, 255);
   ACT_EXTENDED = true;
}
void actuatorsRetract(){
  digitalWrite(dirControlLinAct, HIGH);
  analogWrite(pwmControlLinAct, 255);
  ACT_EXTENDED = false;
}

void LED_FUNC(int pin, bool on){
  digitalWrite(pin, on);
}

void BEEP(){
  digitalWrite(BUZZER, HIGH);
  beep_working = true;
  beep_starting = millis();
}
