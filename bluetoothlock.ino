#include <Servo.h>
#include <SoftwareSerial.h>

// for debugging, switch bluetooth to Serial
#define output bluetooth

// state variables
#define LOCKED 0
#define LOCKING 1
#define UNLOCKED 2

// This is the passcode sent via bluetooth
char password[] = "poop";
// IMPORTANT: passcode may NOT be more than 20 characters or
//    you will get a buffer overflow
#define len strlen(password)

// setup pins
#define servopin 9
#define rxpin 0
#define txpin 1
#define buttonpin 7
#define redpin 5
#define greenpin 6

// declare objects
Servo locker;
// check servo data for these values
#define minpulse 650
#define maxpulse 2250
SoftwareSerial bluetooth(rxpin,txpin);

// angle values for lock and unlock positions
#define lock 180
#define unlock 85

// time before the lock locks, in seconds
#define MAXCOUNT 30

// used to hold the three states (locked, locking, and unlocked)
int state;

// count inputted characters from bluetooth
int count;

// button state variables
int buttonstate;
int lastbuttonstate;
#define DOWN 1
#define UP 0

// a pseudo shift register implemented with a circular array
int startpos;
int sz;
char vals[20];
char value[21];

void put(char c) {
  if (sz < len) {
    vals[sz] = c;
    sz++;
  }
  else {
    vals[startpos % len] = c;
    startpos++;
  }
  if (startpos > 1023) startpos = startpos%len;
}

void get() {
  int k = 0;
  for(int i = startpos; k < len; ++i, ++k) {
    value[k] = vals[i%len];
  }
  value[len] = '\0';
  Serial.print("value: ");
  Serial.println(value);
}


/*************************************************
* IMPLEMENATION STARTS HERE
*************************************************/
void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
  bluetooth.listen();
  
  count = 0;
  
  locker.attach(servopin,minpulse,maxpulse);
  locker.write(unlock);  //begin in unlocked position
  
  state = UNLOCKED;
  
  pinMode(rxpin, INPUT);
  pinMode(txpin, OUTPUT);
  pinMode(buttonpin, INPUT);
  pinMode(servopin, OUTPUT);
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
}

void loop() {
  switch (state) {
    case UNLOCKED:
      locker.write(unlock);
      digitalWrite(redpin, LOW);
      digitalWrite(greenpin, HIGH);
      while(output.available())    //discard bluetooth
        output.read();
      buttonstate = digitalRead(buttonpin);
      if (buttonstate == HIGH && buttonstate != lastbuttonstate) {
        Serial.println("Go to LOCKING STATE");
        state = LOCKING;
      }
      lastbuttonstate = buttonstate;
      break;
      
    case LOCKING:
      locker.write(unlock);
      digitalWrite(greenpin, LOW);
      holdUnlock();
      if (state == UNLOCKED) return;
      while (output.available())    //discard bluetooth
        output.read();
      state = LOCKED;
      Serial.println("Go to LOCKED");
      output.print("Enter password: ");
      break;
      
    case LOCKED:
      locker.write(lock);
      digitalWrite(greenpin, LOW);
      digitalWrite(redpin, HIGH);
      buttonstate = digitalRead(buttonpin);
      if (buttonstate == HIGH && buttonstate != lastbuttonstate) {
        Serial.println("Go to UNLOCKED");
        state = UNLOCKED;
        return;
      }
      lastbuttonstate = buttonstate;
      if (output.available()) {
        char p = output.read();
        put(p);
        count++;
        get();
        Serial.println(value);
        Serial.println(count);
      }
      if (count == len) {
        count = 0;
        Serial.print("Entered password: ");
        Serial.println(value);
        if (strcmp(value,password) == 0) {
          Serial.println("Valid password entered");
          bluetooth.println("Unlocking...");
          holdUnlock();
          if (state == UNLOCKED) return;
          Serial.println("Locking again");
          bluetooth.println("Locking");
        }
        else {
          Serial.println("Invalid password");
          bluetooth.print("Invalid password: ");
          bluetooth.println(value);
        }
        output.print("Enter password: ");
      }
      
      break;
  }
}


// holds an unlock state for MAXCOUNT seconds
void holdUnlock() {
  for(int i = MAXCOUNT; i >= 0; --i) {
    Serial.println(i);
    digitalWrite(redpin, HIGH);
    boolean buttonpressed = checkDelay(buttonpin,500);
    if (buttonpressed) {
      Serial.println("Go to UNLOCKED");
      state = UNLOCKED;
      return;
    }
    digitalWrite(redpin, LOW);
    buttonpressed = checkDelay(buttonpin,500);
    if (buttonpressed) {
      Serial.println("Go to UNLOCKED");
      state = UNLOCKED;
      return;
    }
  }
}

// returns true if a button was pressed during a delayed timeframe
boolean checkDelay(int button, int time) {
  boolean haspressed = false;
  for(int centitime = time/10; centitime >= 0; --centitime) {
    buttonstate = digitalRead(button);
    if (buttonstate == HIGH && buttonstate != lastbuttonstate)
      haspressed = true;
    lastbuttonstate = buttonstate;
    delay(10);
  }
  return haspressed;
}



