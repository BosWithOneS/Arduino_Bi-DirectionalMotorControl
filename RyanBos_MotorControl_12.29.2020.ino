/*
Ryan Bos
Bi-Directional Motor Control 
Dec 29, 2020
*/

#include <Wire.h>                   //For I2C
#include <LiquidCrystal_I2C.h>      //For LCD I2C
LiquidCrystal_I2C lcd(0x27,16,2);   //Define LCD panel

//I/O
//Inputs
const int INPUT_StopPB                = 13;  //NC PB 
const int INPUT_RunPB                 = 12;  //NO PB
const int INPUT_MotorFWD_PB           = 11;  //NO PB
const int INPUT_MotorREV_PB           = 10;  //NO PB

//Outputs
const int OUTPUT_RunLED               = 5;  //Green LED
const int OUTPUT_IdleLED              = 4;  //Red LED
const int OUTPUT_DirectionSelectLED   = 3;  //Yellow LED  //ON = FWD , OFF = REV
const int OUTPUT_HeartBeatLED         = 2;  //Blue LED

//All relay pins
const int OUTPUT_ControlRelay[]       = {22 , 24 , 26 , 28 , 30 , 32 , 34 , 36}; //8 relay coils

//For HeartBeat subroutine
int  HeartBeatBit;                                                        //HeartBeat Bit acts as a BOOL
long HeartBeatPrevMillis   = 0;                                           //Used for heartbeat 
long HeartBeatInterval     = 1000;                                        //1000 milliseconds
byte FullHeart[8]         = {0x00,0x00,0x0a,0x1f,0x1f,0x0e,0x04,0x00,};   //Character for LCD
byte EmptyHeart[9]        = {0x00,0x00,0x0a,0x15,0x11,0x0a,0x04,0x00,};   //Character for LCD

//Internal bits
int MotorDirectionState = 1; //0 = rev, 1 = fwd //Default = 1
int MotorRunState = 0;      //0 = idle, 1 = run //Default = 0


void setup() 
{

LCD_Setup(); //Run set up parameters for LCD screen

  //Input pinmode declare
pinMode(INPUT_RunPB ,        INPUT_PULLUP);
pinMode(INPUT_StopPB ,       INPUT_PULLUP);
pinMode(INPUT_MotorFWD_PB ,  INPUT_PULLUP);
pinMode(INPUT_MotorREV_PB ,  INPUT_PULLUP);

 //Output pinmode declare
pinMode(OUTPUT_RunLED ,             OUTPUT);
pinMode(OUTPUT_IdleLED ,            OUTPUT);
pinMode(OUTPUT_DirectionSelectLED , OUTPUT);  
pinMode(OUTPUT_HeartBeatLED ,       OUTPUT);

//Output Control Relay pinmode declare
  for (int i = 0 ; i < 8 ; i++)
  {
    pinMode(OUTPUT_ControlRelay[i] , OUTPUT); //Set all relays as outputs
  }


 //Inturrupt declare
attachInterrupt(digitalPinToInterrupt(INPUT_RunPB) , ISR_001 , CHANGE);   //Spare
attachInterrupt(digitalPinToInterrupt(INPUT_StopPB) , ISR_002 , CHANGE);  //Spare


Serial.begin(9600); //Start serial monitor
} 
void loop() 
{
HeartBeatLoop();                                    //Subroutine for the heart beat light
StateSelectLoop();                                  //Subroutine to determine whether motor is running or at idle

  switch(MotorRunState)                             //Select wheather to call RunLoop or IdleLoop //Default = IdleLoop
  {
    case HIGH:                                      //Program in run mode
    RunLoop();                                      //Call RunLoop
    DirectionSelectLoop();                          //Only call direction select while motor is in run state
    break;

    case LOW:                                       //Program in idle mode
    IdleLoop();                                     //Call Idle loop //Calls by default on start up
    digitalWrite(OUTPUT_DirectionSelectLED, LOW);   //Turn light off when motor is at idle
    break;
  }

 
  for(int j = 2 ; j<8 ; j++)                        //Only want to turn off relays 2-8
  {
    digitalWrite(OUTPUT_ControlRelay[j] , HIGH);    //Used to turn all the unused control relays off to save power //HIGH = off
  }

}//End main 
void StateSelectLoop()
{
  int RunPB_ButtonState = digitalRead(INPUT_RunPB);       //Read the state of RunPB
  int StopPB_ButtonState = digitalRead(INPUT_StopPB);     //Read the state of StopPB

        if(StopPB_ButtonState == 0)                       //Check if StopPB has been pressed
        {
          MotorRunState = LOW;                            //Set MotorRunState to 0 so the motor idles
           lcd.clear();                                   //Clear LCD on button press
        }
            
        else if(RunPB_ButtonState == 1)                   //Check is RunPB has been pressed
        {
          MotorRunState = HIGH;                           //Set MotorRunState to 1 so motor runs
          lcd.clear();                                    //Clear LCD on button press
        }
           
}
void DirectionSelectLoop()
{
  int MotorFWD_ButtonState = digitalRead(INPUT_MotorFWD_PB);  //Button state for Motor Forward PB
  int MotorREV_ButtonState = digitalRead(INPUT_MotorREV_PB);  //Button state for Motor Reverse PB

        if(MotorFWD_ButtonState == 1)                         //Check if Motor Forward PB pressed
        {
          MotorDirectionState = HIGH;                         //Set direction state high = forward
          lcd.clear();                                        //Clear LCD on button press
        }
        else if(MotorREV_ButtonState == 1)                    //Check if Motor Reverse PB pressed
        {
          MotorDirectionState = LOW;                          //Set direction state low = reverse
          lcd.clear();                                        //Clear LCD on button press
        }
}
void RunLoop()
{
  digitalWrite(OUTPUT_RunLED, HIGH);  //Run light on
  digitalWrite(OUTPUT_IdleLED, LOW);  //Idle light off

  lcd.setCursor(0,0);                 //Print at start of top row
  lcd.print("Motor Running");         //Print on top row of LCD

  Serial.println("Run Loop");         //Print in serial monitor

      switch(MotorDirectionState)     //Select whether to call MotorForwardLoop or MotorReverseLoop
      {                               //Only used when inside run loop
        case HIGH:                    //Porgram in forward mode
        MotorForwardLoop();           //Call MotorForwardLoop
        break;

        case LOW:                     //Program in reverse mode
        MotorReverseLoop();           //Call MotorReverseLoop
        break;  
      }
} 
void IdleLoop()
{
  digitalWrite(OUTPUT_IdleLED, HIGH);             //Idle light on
  digitalWrite(OUTPUT_RunLED, LOW);               //Run light off

  lcd.setCursor(0,0);                             //Print on top row of LCD
  lcd.print("Motor Idle");                        //Print on top row of LCD

  digitalWrite(OUTPUT_ControlRelay[0] , HIGH);    //Relay 0 OFF
  digitalWrite(OUTPUT_ControlRelay[1] , HIGH);    //Relay 1 OFF
  
  Serial.println("Idle Loop");                    //Print in serial monitor
}
void MotorForwardLoop()
{
  digitalWrite(OUTPUT_DirectionSelectLED, HIGH);  //Trun LED on to indicate forward
  lcd.setCursor(0,1);                             //Set at beginning of sencond line
  lcd.print("Forward");                           //Print Forward  

  digitalWrite(OUTPUT_ControlRelay[0] , HIGH);    //Relay 0 OFF
  digitalWrite(OUTPUT_ControlRelay[1] , LOW);     //Relay 1 ON

  Serial.println("Forward Loop");                 //Print in serial monitor
}
void MotorReverseLoop()
{
  digitalWrite(OUTPUT_DirectionSelectLED, LOW); //Turn LED off to indicate reverse
  lcd.setCursor(0,1);                           //Set at beginning of sencond line
  lcd.print("Reverse");                         //Print Reverse

  digitalWrite(OUTPUT_ControlRelay[0] , LOW);   //Relay 0 ON
  digitalWrite(OUTPUT_ControlRelay[1] , HIGH);  //Relay 1 OFF

  Serial.println("Reverse Loop");               //Print in serial monitor
}
void LCD_Setup() 
{
lcd.init();                     //LCD initialize
lcd.backlight();                //Turn on LCD backlight 

lcd.createChar(0,FullHeart);    //Create full heart character 
 lcd.createChar(1,EmptyHeart);  //Create empty hear character
}
void HeartBeatLoop()
{
  unsigned long HeartBeatCurrentMillis = millis();                      //Assign the value of the currnt millis coutner to HeartBeatCurrentMillis
  
  if(HeartBeatCurrentMillis - HeartBeatPrevMillis > HeartBeatInterval)  //Check if Current millis - previous millis is greater than heartbeat interval
  {
      HeartBeatPrevMillis = HeartBeatCurrentMillis;                     //Set prev milis to current millis

      if(HeartBeatBit == 0)                                             //If HeartBeatBit is 0
      {
        HeartBeatBit = 1;                                               //Set HeartBeatBit to 1
        digitalWrite(OUTPUT_HeartBeatLED, HIGH);                        //Turn HeartBeatLED HIGH
        lcd.setCursor(15,0);                                            //Print in top right corner
        lcd.write(byte(1));                                             //Print empty heart character
      }
      else
      {
        HeartBeatBit = 0;                                               //Set HeartBeatBit to 0
        digitalWrite(OUTPUT_HeartBeatLED, LOW);                         //Turn HeartBeatLED LOW
        lcd.setCursor(15,0);                                            //Print in top right corner
        lcd.write(byte(0));                                             //Print full heart character
      }
  }
}
void ISR_001() //Not used
{
  //state = !state; //Easy way to change the state of a tag !!!!!! instead of having it equal 1 or 0
  }
void ISR_002()//Not used
{
  //state = !state; //Easy way to change the state of a tag !!!!!! instead of having it equal 1 or 0
  }
