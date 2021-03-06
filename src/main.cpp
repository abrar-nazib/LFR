/*
 *  Code for robot gathering contest
 *  
 *ideal--- left 15 right 160
 *Big object -- R-105 L-75
 *
*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

int DEBUG = 0;      // Make it 0 in main run
int MEMORY = 0;     // Memory not in use
#define trigPin A8  // Must connect to an analog pin
#define echoPin A10 // Must connect to an analog pin
#define STBY 5
#define rMin1 7
#define rMin2 6
#define lMin1 4
#define lMin2 3
#define pwL 2
#define pwR 12
#define btn1 23
#define btn2 29
#define btn3 22
#define btn4 28
#define btn5 34
#define led1 24
#define led2 26
#define led3 32
#define rServo_pin 8
#define lServo_pin 9
#define sda 20 // Important! cannot change this pin
#define scl 21 // Important! cannot change this pin
//-------------------- DISPLAY RELATED ------------------------------------------------
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
//--------------------------------------------------------------------
Servo rServo;
Servo lServo;

//-------------------- IR SENSOR RELATED VARIABLES-----------------------------------------
boolean firstData[8]; /*to eliminate effect of noise*/
int v = 0;
int sensor[8];                                       /*sensor readings are saved here*/
int sensorPin[8] = {A7, A6, A5, A4, A3, A2, A1, A0}; /*arduino pins to read sensors*/
byte NumOfSensors = 8;
byte i; /*just to run for loop!!*/
unsigned int MaxWaitTime = 1024;
byte sensorData;
//---------------------------------------------------------------------------------------------

char directions[500][2] = {'LN', 'RN', 'FN', 'RN', 'rX', 'lN', 'RN', 'LN'}; // memory of the track to follow -> have to be defined according to the track
unsigned int directions_iterator = 0;
double Const1 = 11.6;
double Const2 = 0;
double Const3 = 4.2;
double Shomakolon = 0;
double motorspeed = 180 - (object * 20);
double velocity = 0;
double Vul = 0;
double PIDvalue, RSpeed, LSpeed;
double Parthokko = 0;
double AgerVul = 0;
int threshold[8]; //Array for holding sensor threshold values
int t1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int t2[8] = {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024};
unsigned long int memory[50]; // Memory of the path
int memory_length = 50;
int sumation;
int ct[8] = {-4, -3, -2, -1, 1, 2, 3, 4};
int x[8];
int reading;
int sm = 0;
int rs_init = 160;
int ls_init = 80;
int rs = 105;
int ls = 75;
int object = 0;
int big_obj = 0;
int small_obj = 0;
int srch = 0;
//----------------------Functions used in this code-------------------------------
void readSensors();
void generateBinary();
void generateThreshold();
void deviation();
void PIDval();
void doura();
void Forward(double del, int vel);
void Backward(double del, int vel);
void Right(double del, int vel);
void Left(double del, int vel);
void Stop(double del);
void Tleft();
void Tright();
void BreakR();
void BreakL();
void BreakF();
void pick_object();
void release_object();
double search();
void shift_right(int value);
void detection();
void configurePID();
void save_threshold(int threshold[8]);
void retrieve_threshold();
void configureServo();

//-----------------------------Starting point------------------
void setup()
{
  Serial.begin(9600);
  pinMode(rMin1, OUTPUT);
  pinMode(rMin2, OUTPUT);
  pinMode(lMin1, OUTPUT);
  pinMode(lMin2, OUTPUT);
  pinMode(STBY, OUTPUT);

  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(btn3, INPUT_PULLUP);
  pinMode(btn4, INPUT_PULLUP);
  pinMode(btn5, INPUT_PULLUP);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  pinMode(pwR, OUTPUT);
  pinMode(pwL, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(STBY, HIGH);

  rServo.attach(rServo_pin);
  lServo.attach(lServo_pin);
  //----------------------DISPLAY------------------
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  //-------------------------------------------------
  for (int load_memory = 0; load_memory < memory_length; load_memory++)
  {
    memory[load_memory] = 0;
  }
  lServo.write(ls);
  rServo.write(rs);
  while (true)
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("CALIBRATE  <==>  RUN");
    display.setCursor(0, 10);
    display.print("btn2");
    display.setCursor(100, 10);
    display.print("btn1");
    display.display();
    if (digitalRead(btn2) == LOW)
    {
      generateThreshold();
      save_threshold(threshold);
      break;
    }
    else if (digitalRead(btn1) == LOW)
    {
      retrieve_threshold();
      break;
    }
  }
}
//------------------------------Main Loop-----------------------------------------------------
void loop()
{
  readSensors();
  generateBinary();
  deviation();
  PIDval();
  doura();
  if (sumation > 4 || sumation == 0)
    detection();
  if (srch == 1)
  {
    double distance = search();
    if (distance > 5 && distance < 15)
    {
      pick_object();
    }
  }
}
//--------------------------------------------------------------------------------------
void save_threshold(int threshold[8])
{
  for (int threshold_iterator = 0; threshold_iterator < 8; threshold_iterator++)
  {
    byte first_half = B00000000;
    byte second_half = B00000000;
    first_half = (threshold[threshold_iterator] >> 8) | first_half;
    second_half = (threshold[threshold_iterator] & B11111111) | second_half;
    EEPROM.write(threshold_iterator * 2 + 1, first_half);
    EEPROM.write(threshold_iterator * 2 + 2, second_half);
  }
}
//---------------------------------------------------------------------------------------
void retrieve_threshold()
{
  for (int threshold_iterator = 0; threshold_iterator < 8; threshold_iterator++)
  {
    threshold[threshold_iterator] = (EEPROM.read(threshold_iterator * 2 + 1) << 8) | (EEPROM.read(threshold_iterator * 2 + 2));
  }
}
//-----------------------------------------------------------------------------------------
void configurePID()
{
  directions_iterator = 0;
  int constvalue = 1;
  while (true)
  {
    if (constvalue == 1)
    {
      display.clearDisplay();
      display.setCursor(4, 0);
      display.print("Const1");
      display.setCursor(5, 10);
      display.print(Const1);
      display.display();
      if (digitalRead(btn5) == LOW)
      {
        Const1 = Const1 + 0.1;
        display.setCursor(5, 10);
        display.print(Const1);
        display.display();
      }
      else if (digitalRead(btn3) == LOW)
      {
        Const1 = Const1 - 0.1;
        display.setCursor(5, 10);
        display.print(Const1);
        display.display();
      }
      if (digitalRead(btn1) == LOW || digitalRead(btn2) == LOW)
      {
        constvalue = !constvalue;
      }
      if (digitalRead(btn4) == LOW)
        break;
    }
    else if (constvalue == 0)
    {
      display.clearDisplay();
      display.setCursor(4, 0);
      display.print("Const3");
      display.setCursor(5, 10);
      display.print(Const3);
      display.display();
      if (digitalRead(btn5) == LOW)
      {
        Const3 = Const3 + 0.1;
        display.setCursor(5, 10);
        display.print(Const3);
        display.display();
      }
      else if (digitalRead(btn3) == LOW)
      {
        Const3 = Const3 - 0.1;
        display.setCursor(5, 10);
        display.print(Const3);
        display.display();
      }
      if (digitalRead(btn1) == LOW || digitalRead(btn2) == LOW)
      {
        constvalue = !constvalue;
      }
      if (digitalRead(btn4) == LOW)
        break;
    }
    if (digitalRead(btn4) == LOW)
      break;
  }
}

void configureServo()
{
  int constvalue = 1;
  while (true)
  {

    if (constvalue == 1)
    {
      display.clearDisplay();
      display.setCursor(4, 0);
      display.print("Rservo");
      display.setCursor(5, 10);
      display.print(rs);
      display.display();
      if (digitalRead(btn5) == LOW)
      {
        rs++;
        display.setCursor(5, 10);
        display.print(rs);
        display.display();
      }
      else if (digitalRead(btn3) == LOW)
      {
        rs--;
        display.setCursor(5, 10);
        display.print(rs);
        display.display();
      }
      if (digitalRead(btn1) == LOW || digitalRead(btn2) == LOW)
      {
        constvalue = !constvalue;
      }
      if (digitalRead(btn4) == LOW)
        break;
    }
    else if (constvalue == 0)
    {
      display.clearDisplay();
      display.setCursor(4, 0);
      display.print("LServo");
      display.setCursor(5, 10);
      display.print(ls);
      display.display();
      if (digitalRead(btn5) == LOW)
      {
        ls++;
        display.setCursor(5, 10);
        display.print(ls);
        display.display();
      }
      else if (digitalRead(btn3) == LOW)
      {
        ls--;
        display.setCursor(5, 10);
        display.print(ls);
        display.display();
      }
      if (digitalRead(btn1) == LOW || digitalRead(btn2) == LOW)
      {
        constvalue = !constvalue;
      }
      if (digitalRead(btn4) == LOW)
        break;
    }
    if (digitalRead(btn4) == LOW)
      break;
    rServo.write(rs);
    lServo.write(ls);
  }
}

//-----------------------------------------------------------------------------------------
void shift_right(int value)
{
  memory[value] = memory[value - 1];
  if (value == 1)
    return;
  shift_right(value - 1);
}
//------------------------------------------------------------------------------------------
void readSensors()
{
  /*
  * Code to read sensor data -> identical to the documentation with some simple modifications 
  * 
  */
  for (i = 0; i < NumOfSensors; i++)
  {
    digitalWrite(sensorPin[i], HIGH);
    pinMode(sensorPin[i], OUTPUT);
  }
  delayMicroseconds(20);
  for (i = 0; i < NumOfSensors; i++)
  {
    pinMode(sensorPin[i], INPUT);
    digitalWrite(sensorPin[i], LOW);
    sensor[i] = MaxWaitTime;
    firstData[i] = false;
  }
  unsigned long startTime = micros();

  while (micros() - startTime < MaxWaitTime)
  {
    unsigned int time = micros() - startTime;
    for (i = 0; i < NumOfSensors; i++)
    {
      if ((digitalRead(sensorPin[i]) == LOW) && (firstData[i] == false))
      {
        sensor[i] = time;
        firstData[i] = true;
      }
    }
  }
  if (DEBUG)
  {
    for (int sensor_iterator = 0; sensor_iterator < NumOfSensors; sensor_iterator++)
    {
      Serial.print(" ");
      Serial.print(sensor[sensor_iterator]);
    }
  }
}
//---------------------------------------------------------------------------------------
void generateBinary()
{
  if (DEBUG)
    display.clearDisplay();

  sumation = 0;
  for (int cx = 0; cx < NumOfSensors; cx++)
  {
    if (sensor[cx] > threshold[cx])
    {
      x[cx] = 1;
    }
    else
    {
      x[cx] = 0;
    }
    sumation += x[cx];
  }

  sensorData = 0;

  for (int cxx = 0; cxx < NumOfSensors; cxx++)
  {
    sensorData = (sensorData << 1) | x[cxx];
    if (DEBUG)
    {
      display.setCursor(cxx + 4, 0);
      display.print(x[cxx]);
      display.display();
    }
  }
  if (MEMORY)
  {
    shift_right(memory_length - 1);
    memory[0] = sensorData;
  }
  if (DEBUG)
    display.clearDisplay();
}
//--------------------------------------------------------------------------------
void generateThreshold()
{
  display.clearDisplay();
  display.setCursor(2, 0);
  display.print("Calibrating...");
  display.display();
  for (int th = 0; th < 300; th++)
  {
    Forward(1, 100);
    readSensors();
    for (int sense = 0; sense < NumOfSensors; sense++)
    {
      if (sensor[sense] > t1[sense])
        t1[sense] = sensor[sense];
      if (sensor[sense] < t2[sense])
        t2[sense] = sensor[sense];
    }
  }
  for (int thr = 0; thr < NumOfSensors; thr++)
  {
    threshold[thr] = (t1[thr] + t2[thr]) / 2;
  }
  Stop(10);
  while (true)
  {
    delay(10);
    if (digitalRead(btn4) == LOW)
      break;
  }
}
//----------------------------------THE MAIN CALCULATION&EXECUTION------------------------------------------------
void deviation()
{
  if (sensorData == B00000000)
    Vul = Vul;
  else if (sensorData == B00011000)
    Vul = 0; //0001 1000
  else if (sensorData == B00111100)
    Vul = 0; //0011 1100
  //------------------------TWO SENSOR------------------------
  else if (sensorData == B00001100) //12
    Vul = 7.5;                      //0000 1100
  else if (sensorData == B00110000)
    Vul = -7.5; //0011 0000
  else if (sensorData == B00000110)
    Vul = 12.5; //0000 0110
  else if (sensorData == B01100000)
    Vul = -12.5; //0110 0000
  else if (sensorData == B00000011)
    Vul = 20; //0000 0011
  else if (sensorData == B11000000)
    Vul = -20; //1100 0000
  else if (sensorData == B00000001)
    Vul = 25; //0000 0001
  else if (sensorData == B10000000)
    Vul = -25; //1000 0000
  //-----------------------------three sensor---------------
  else if (sensorData == B00011100)
    Vul = 4; //0001 1100
  else if (sensorData == B00111000)
    Vul = -4; //0011 1000
  else if (sensorData == B00001110)
    Vul = 10; //0000 1110
  else if (sensorData == B01110000)
    Vul = -10; //0111 0000
  else if (sensorData == B00000111)
    Vul = 16; //0000 0111
  else if (sensorData == B11100000)
    Vul = -16; //1110 0000
  //----------------------four sensor-------------------------
  else if (sensorData == B00011110)
    Vul = 7.5; //0001 1110
  else if (sensorData == B01111000)
    Vul = -7.5; //0111 1000
  else if (sensorData == B00001111)
    Vul = -12.5; //0000 1111
  else if (sensorData == B11110000)
    Vul = -12.5; //1111 0000
  else
  {
    Vul = 0;
    sm = 0;
    for (int cnt = 0; cnt < NumOfSensors; cnt++)
    {
      Vul += (x[cnt] * 5 * ct[cnt]);
      sm += x[cnt];
    }
    Vul = Vul / sm;
  }
}
//-----------------------------------------------------------------------------------
void PIDval()
{
  Shomakolon = Shomakolon + Vul;
  Parthokko = Vul - AgerVul;
  PIDvalue = (Const1 * Vul) + (Const3 * Parthokko) + (Shomakolon * Const2);
  AgerVul = Vul;
  if (DEBUG)
  {
    Serial.print(" ");
    Serial.print(Vul);
  }
}
//------------------------------------------------------------------------------------
void doura()
{
  if (Vul > 0)
  {
    RSpeed = motorspeed - PIDvalue;
    LSpeed = motorspeed;
  }
  else if (Vul < 0)
  {
    LSpeed = motorspeed + PIDvalue;
    RSpeed = motorspeed;
  }
  else
  {
    RSpeed = motorspeed;
    LSpeed = motorspeed;
  }

  if (RSpeed < 5)
    RSpeed = 5;
  if (LSpeed < 5)
    LSpeed = 5;

  analogWrite(pwR, RSpeed);
  analogWrite(pwL, LSpeed);
  digitalWrite(rMin1, HIGH);
  digitalWrite(lMin1, HIGH);
  digitalWrite(rMin2, LOW);
  digitalWrite(lMin2, LOW);
}
//--------------------------------Driving Functions--------------------------------------
void Forward(double del, int vel)
{
  analogWrite(pwR, vel);
  analogWrite(pwL, vel);
  digitalWrite(rMin1, HIGH);
  digitalWrite(rMin2, LOW);
  digitalWrite(lMin1, HIGH);
  digitalWrite(lMin2, LOW);
  delay(del);
}
//--------------------------------------------------------------------------------------
void Backward(double del, int vel)
{
  analogWrite(pwR, vel);
  analogWrite(pwL, vel);
  digitalWrite(rMin2, HIGH);
  digitalWrite(rMin1, LOW);
  digitalWrite(lMin2, HIGH);
  digitalWrite(lMin1, LOW);
  delay(del);
}
//--------------------------------------------------------------------------------------
void Right(double del, int vel)
{
  analogWrite(pwR, vel);
  analogWrite(pwL, vel);
  digitalWrite(rMin1, LOW);
  digitalWrite(lMin1, HIGH);
  digitalWrite(rMin2, HIGH);
  digitalWrite(lMin2, LOW);
  delay(del);
}
//--------------------------------------------------------------------------------------
void Left(double del, int vel)
{
  analogWrite(pwR, vel);
  analogWrite(pwL, vel);
  digitalWrite(rMin1, HIGH);
  digitalWrite(lMin1, LOW);
  digitalWrite(rMin2, LOW);
  digitalWrite(lMin2, HIGH);
  delay(del);
}
//---------------------------------------------------------------------------------------
void Stop(double del)
{
  analogWrite(pwR, 0);
  analogWrite(pwL, 0);
  digitalWrite(rMin1, LOW);
  digitalWrite(lMin1, LOW);
  digitalWrite(rMin2, LOW);
  digitalWrite(lMin2, LOW);
  delay(del);
}
//---------------------------------Breaking functions--------------------------------------
void BreakF()
{
  Stop(10);
  Backward(80, 250);
  Stop(10);
}
//-----------------------------------------------------------------------------------------
void BreakL()
{
  Stop(10);
  Right(50, 250);
  Stop(10);
  // Right(40, 200);
  // Stop(10);
}
//-----------------------------------------------------------------------------------------
void BreakR()
{
  Stop(10);
  Left(50, 250);
  Stop(10);
  // Left(40, 200);
  // Stop(10);
}
//----------------------------------------------------------------------------------------
void Tleft()
{
  BreakF();
  while (1)
  {
    Left(10, 240);
    readSensors();
    generateBinary();
    if (x[0] == 1 || x[1] == 1)
    {
      while (true)
      {
        Left(10, 50);
        readSensors();
        generateBinary();
        if (x[3] == 1 || x[4] == 1)
        {
          BreakL();
          break;
        }
      }
      break;
    }
  }
}
//----------------------------------------------------------------------------------------
void Tright()
{
  BreakF();
  while (1)
  {
    Right(10, 240);
    readSensors();
    generateBinary();
    if (x[6] == 1 || x[7] == 1)
    {
      while (true)
      {
        Right(10, 50);
        readSensors();
        generateBinary();
        if (x[3] == 1 || x[4] == 1)
        {
          BreakR();
          break;
        }
      }
      break;
    }
  }
}
//----------------------------------Searching For Object------------------------------------
double search()
{
  double duration = 0.00;
  double CM = 0.00;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);

  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 1700);
  CM = (duration / 58.82);
  return CM;
}
//-------------------------------------------------------------------------------------------
void pick_object()
{
  BreakF();
  while (search() > 5)
  {
    Forward(10, 80);
  }
  //assumption
  Stop(100);
  for (int servo_angle = 0; servo_angle < 90; servo_angle++)
  {
    if (big_obj)
    {
      //rServo.write(servo_angle);
      //lServo.write(servo_angle);
      big_obj = 0;
    }
    else if (small_obj)
    {
      //small object grabbing code
      small_obj = 0;
    }

    delay(10);
  }
}
//--------------------------------------------------------------------------------------------
void release_object()
{
  BreakF();
  for (int servo_angle = 90; servo_angle > 0; servo_angle--)
  {
    //rServo.write(servo_angle);
    //lServo.write(servo_angle);
    delay(10);
  }
  while (search() < 8)
  {
    Backward(10, 50);
  }
}
//------------------------------------Case Detection-------------------------------------------
void detection()
{
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  for (int detect = 0; detect < 20; detect++)
  {
    readSensors();
    generateBinary();
    deviation();
    PIDval();
    doura();
  }

  unsigned long int memory_value = 0;
  if (MEMORY)
  {

    for (int memory_iterator = 0; memory_iterator < memory_length; memory_iterator++)
    {
      memory_value = memory_value + memory[memory_iterator];
    }
  }

  /*
  *Desicion making code will go here -> depends on on field values
  * 
  */

  if (directions[directions_iterator][0] == 'L')
  {
    Tleft();
    object = 0;
  }
  else if (directions[directions_iterator][0] == 'R')
  {
    Tright();
    object = 0;
  }
  else if (directions[directions_iterator][0] == 'F')
  {
    Forward(10, 10);
    object = 0;
  }
  else if (directions[directions_iterator][0] == 'r')
  {
    Tright();
    object = 1;
  }
  else if (directions[directions_iterator][0] == 'f')
  {
    Forward(10, 10);
    object = 1;
  }
  else if (directions[directions_iterator][0] == 'l')
  {
    Tleft();
    object = 1;
  }

  if (directions[directions_iterator][1] == 'N')
  {
    srch = 0;
  }

  if (directions[directions_iterator][1] == 'X')
  {
    srch = 1;
    big_obj = 1;
  }
  if (directions[directions_iterator][1] == 'x')
  {
    srch = 1;
    small_obj = 1;
  }

  directions_iterator++;

  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  //---------------------------------------------------------------
  if (MEMORY)
  {

    while (true)
    {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print(memory_value);
      display.display();
      if (digitalRead(btn4) == LOW)
        break;
    }
  }
}