#include <TM1637Display.h>                                           // https://github.com/avishorp/TM1637
#include <ClickEncoder.h>                                            // https://github.com/0xPIT/encoder/tree/arduino
#include <TimerOne.h>                                                // https://playground.arduino.cc/Code/Timer1/

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x) 
#endif

const int enc_pin_A = 4;                                              // rotary encoder first data pin A at D5 pin
const int enc_pin_B = 5;                                              // rotary encoder second data pin B at D4 pin, if the encoder moved in the wrong direction, swap A and B
const int enc_pin_SW = 7;                                             // rotary encoder switch button at D7 pin
const int rpm_pin = 9;                                                // RPM output to step motor at D9 pin  
const int rpm_en_pin = 8;                                             // step motor driver enable at D8 pin 
const int dir_pin = 6;                                                // step motor driver rotation direction at D6 pin  

bool colon = true;                                                    // timer colon active at start
bool off = true;                                                     
int RPM, lastRPM, last_tempRPM, tempRPM;
int16_t value, lastValue;
unsigned long colon_ms, lastAccelTime, lastDecelTime, lastRPMTime, himillis;

uint8_t hi[] = {
  0x00, 0x00,
  SEG_F|SEG_B|SEG_G|SEG_C|SEG_E,                                      // H
  SEG_B|SEG_C,                                                        // I
};

TM1637Display display( 2, 3 );                                        // TM1637 CLK connected to D2 and DIO to D3

ClickEncoder *encoder;

void timerIsr() {                                                     // encoder interupt service routine
  
  encoder -> service();  
  
}


void setup() {
  
  Serial.begin( 115200 );                                             // serial for debug

  display.setBrightness( 0x01 );                                      // medium brightness, to reduce LM1117-5V current and overheating, maximum is 7
 
  encoder = new ClickEncoder( enc_pin_A, enc_pin_B, enc_pin_SW, 4 );  // rotary encoder init, last value is encoder steps per notch
  
  Timer1.initialize( 1000 );                                          // set the timer period in us, 1ms
  Timer1.attachInterrupt( timerIsr );                                 // attach the service routine
  
  pinMode( rpm_pin, OUTPUT );   
  pinMode( rpm_en_pin, OUTPUT ); 
  digitalWrite(rpm_en_pin, HIGH);          
  pinMode( dir_pin, OUTPUT );  
  digitalWrite(dir_pin, HIGH);                               
  lastValue = 0;   
  lastAccelTime = 0;
  lastDecelTime = 0;
  colon_ms = millis();
  himillis = 0;
  RPM=20;

}


void menuRPM()  {

    value += encoder -> getValue();
    
    if ( value > lastValue ) {
      if ( RPM >= 50 )
        RPM = 50;                                                     // max rpm is 50 cw
      else if (( RPM >=-2) && ( RPM <=0)) {
        RPM  = 2;                                                     // min rpm is 2
      }
      else
        RPM +=1;                                                      // one rotary step is 1 rpm  
      }
          
    else if ( value < lastValue ) {
      if ( RPM <= -50 )
        RPM = -50;                                                    // max rpm is -50 ccw
      else if (( RPM <=2) && ( RPM >=0)) {
        RPM = -2;
      }
      else
        RPM -=1;                                                      // one rotary step is 1 rpm
    }
    
    if ( value != lastValue ) {
      lastRPMTime = millis(); 
      lastValue = value;
      DEBUG_PRINT( "RPM value: " );
      DEBUG_PRINTLN( RPM );
      lastRPM=RPM;
    }                                               

  
  
  display.showNumberDecEx( RPM, 0x80 >> false , false );              // show rpm speed, no colon, no leading zeros
  
  buttonCheck();                                                      // check rotary encoder button
  rpmCheck();
  
}



void rpmCheck()  {

  if (!off) {
    int tempRPM = last_tempRPM;  

      if ( tempRPM < RPM ) {                                              // acceleration 
        if ( millis() - lastAccelTime > 200 ) {
          lastAccelTime = millis();
          tempRPM++;
        }
      }
      else if ( tempRPM > RPM) {
        if ( millis() - lastDecelTime > 200 ) {                           // dececeleration
          lastDecelTime = millis();
          tempRPM--;
        }
      }
    if ( last_tempRPM != tempRPM ){
        DEBUG_PRINT( tempRPM );
        DEBUG_PRINTLN( " TEMPRPM" );
    }
  
    last_tempRPM = tempRPM;
  
    if ( last_tempRPM >= 0 )
      digitalWrite( dir_pin, HIGH );
    else
      digitalWrite( dir_pin, LOW );

     if ( abs(last_tempRPM) <= 2 )                                        // min rpm value is 2 or -2
      Timer1.pwm( rpm_pin, 512, 9375);                                    // 9375 = 18750 / 2
     else                                       // one motor revolution is 200 steps, with driver factor 1/16 per step is 3200, 
      Timer1.pwm( rpm_pin, 512, (int) 18750 / abs(last_tempRPM)   ); 
                                              // so for one revolution per minute is 60 000 000 / 3200 = 18750 period, 512 is duty 50%
  }
  
  else
    if ( last_tempRPM ==0 ) {                                             // default 20 rpm if done, to prevent error readings from encoder if rpm = 0
      Timer1.pwm( rpm_pin, 512, 750);
  }
  
}


void buttonCheck() {
  
 ClickEncoder::Button b = encoder -> getButton();
   if ( b != ClickEncoder::Open ) {
      DEBUG_PRINT( "Button: " );
      
      #define VERBOSECASE( label ) case label: DEBUG_PRINTLN( #label ); break;
      
      switch ( b ) {
         VERBOSECASE( ClickEncoder::Pressed );
         VERBOSECASE( ClickEncoder::Released )
         
       case ClickEncoder::Clicked:
         DEBUG_PRINTLN( "ClickEncoder::Clicked" );
            if ( off )  {                                         // set rpm
                digitalWrite(rpm_en_pin, LOW);
                DEBUG_PRINTLN( "start" );
                off = false;
            }
            else {                                                    // start or go back to countdown if rpm set 
              off = true;
              last_tempRPM = 0;                                               
              digitalWrite( rpm_en_pin, HIGH );
              DEBUG_PRINTLN( "finished" );
            } 
            value = encoder -> getValue();
            lastValue = value;   											// set last encoder value
       break;                
      } 
  }
}


void loop() {
  
  menuRPM();
  
}
