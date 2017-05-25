// Experimental Flexible Disk Drive Exerciser
// fjkraan@xs4all.nl, 2017-05-25
// This software is released under the 3-clause BSD license


#define SERIALBUFSIZE         11
unsigned long previousMillis = 0;
char serialBuffer[SERIALBUFSIZE];
byte setBufPointer = 0;

// functional pins of the Teensy 3.2
#define LED 13

#define READY       8
#define READDATA    9
#define WRITEPROT  10
#define TRACK00    11
#define INDEX      12

#define RESERVED1  23
#define HEADLOAD   22
#define DRIVESEL   21
#define MOTOR      20
#define DIRECTION  19
#define STEP       18
#define WRITEDATA  17
#define WRITEGATE  16
#define SIDESELECT 15
#define RESERVED2  14

#define STEPPULSE   50
#define STEPRATE   100

bool hlState;
bool dsState;
bool mtState;
bool diState;
bool stState;
bool wdState;
bool wgState;
bool ssState;
byte track;
byte trackCount;

void setup() {
    pinMode(LED, OUTPUT); 
    Serial.begin(19200); 
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
    Serial.println("FDD Exerciser 0.2.0");
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
    track = 255;
    trackCount = 35;
    pinConfig();
}

void loop() {
    commandCollector();
    digitalWrite(LED, !digitalRead(INDEX));
}

void pinConfig() {
    hlState = 1;
    dsState = 1;
    mtState = 1;
    diState = 1;
    stState = 1;
    wdState = 1;
    wgState = 1;
    ssState = 1;

    pinMode(INDEX,     INPUT_PULLUP);
    pinMode(TRACK00,   INPUT_PULLUP);
    pinMode(WRITEPROT, INPUT_PULLUP);
    pinMode(READDATA,  INPUT_PULLUP);
    pinMode(READY,     INPUT_PULLUP);

    pinMode(HEADLOAD,   OUTPUT);
    pinMode(DRIVESEL,   OUTPUT);
    pinMode(MOTOR,      OUTPUT);
    pinMode(DIRECTION,  OUTPUT);
    pinMode(STEP,       OUTPUT);
    pinMode(WRITEDATA,  OUTPUT);
    pinMode(WRITEGATE,  OUTPUT);
    pinMode(SIDESELECT, OUTPUT);
    
    digitalWrite(HEADLOAD,   hlState);
    digitalWrite(DRIVESEL,   dsState);
    digitalWrite(MOTOR,      mtState);
    digitalWrite(DIRECTION,  diState);
    digitalWrite(STEP,       stState);
    digitalWrite(WRITEDATA,  wdState);
    digitalWrite(WRITEGATE,  wgState);
    digitalWrite(SIDESELECT, ssState);
}

void commandCollector() {
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    switch(inByte) {
    case '.':
    case '\r':
    case '\n':
      commandInterpreter();
      clearSerialBuffer();
      setBufPointer = 0;
      break;
    default:
      serialBuffer[setBufPointer] = inByte;
      setBufPointer++;
      if (setBufPointer >= SERIALBUFSIZE) {
        Serial.println("Serial buffer overflow. Cleanup.");
        clearSerialBuffer();
        setBufPointer = 0;
      }
    }
  }
}

void commandInterpreter() {
  byte bufByte = serialBuffer[0];
  switch(bufByte) {
    case 'D':
    case 'd':
      dArgInterpreter();
      break; 
    case 'I':
    case 'i':
      iArgInterpreter();
      break;
    case 'H':
    case 'h':
      hArgInterpreter();
      break;
    case '?':
      Serial.println("DSx, DIx, Hx, Mx, R, STnn, SSx, ?");
      break; 
    case 'M':
    case 'm':
      mArgInterpreter();
      break;
    case 'P':
    case 'p':
      pArgInterpreter();
      break;
    case 'R':
    case 'r':
      rArgInterpreter();
      break;
    case 'S':
    case 's':
      sArgInterpreter();
      break;
    case 'T':
    case 't':
      tArgInterpreter();
      break;
    default:
      Serial.print(bufByte);
      Serial.println(": unknown command");
      return;
  }
}

void dArgInterpreter() {
    if (setBufPointer == 1) return;
    if ((setBufPointer == 2 || setBufPointer == 3) && 
        (serialBuffer[1] == 'S' || serialBuffer[1] == 's')) {
        dsArgInterpreter();
        return;
    }
    if ((setBufPointer == 2 || setBufPointer == 3) && 
        (serialBuffer[1] == 'I' || serialBuffer[1] == 'i')) {
        diArgInterpreter();
        return;
    }
}

void dsArgInterpreter() {
    if (setBufPointer == 2) {
        Serial.print("DS");
        Serial.println(dsState, DEC);
    } else if (setBufPointer == 3) {
        dsState = serialBuffer[2] == '0' ? 0 : 1;
        digitalWrite(DRIVESEL, dsState);
        Serial.print("DS");
        Serial.println(dsState, DEC);
    } else {
        Serial.print("DS ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void diArgInterpreter() {
    if (setBufPointer == 2) {
        Serial.print("DI");
        Serial.println(diState, DEC);
    } else if (setBufPointer == 3) {
        diState = serialBuffer[2] == '0' ? 0 : 1;
        digitalWrite(DIRECTION, diState);
        Serial.print("DI");
        Serial.println(diState, DEC);        
    } else {
        Serial.print("DI ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void hArgInterpreter() {
    if (setBufPointer == 1) { 
        Serial.print("H");
        Serial.println(hlState, DEC);
     } else if (setBufPointer == 2) {
        hlState = serialBuffer[1] == '0' ? 0 : 1;
        digitalWrite(HEADLOAD, hlState);
        Serial.print("H");
        Serial.println(hlState, DEC);        
     } else {
        Serial.print("H ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);  
    }
}

void iArgInterpreter() {
    if (setBufPointer == 1) { 
        Serial.println("Ix T0 WP RD Ry / Tr");
        Serial.print(" ");
        Serial.print(digitalRead(INDEX), DEC);
        Serial.print("  ");
        Serial.print(digitalRead(TRACK00), DEC);
        Serial.print("  ");
        Serial.print(digitalRead(WRITEPROT), DEC);
        Serial.print("  ");
        Serial.print(digitalRead(READDATA), DEC);
        Serial.print("  ");
        Serial.print(digitalRead(READY), DEC);    
        Serial.print("   ");
        Serial.println(track, DEC);    
    } else {
        Serial.print("I ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}
    
void mArgInterpreter() {
     if (setBufPointer == 1) { 
          Serial.print("M");
          Serial.println(mtState, DEC);
     } else if (setBufPointer == 2) {
          mtState = serialBuffer[1] == '0' ? 0 : 1;
          digitalWrite(MOTOR, mtState);
          Serial.print("M");
          Serial.println(mtState, DEC);        
     } else {
        Serial.print("M ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void pArgInterpreter() {
    if (setBufPointer == 2) {
        if (serialBuffer[1] == '0') {
            runUpDownPattern();
        } else if (serialBuffer[1] == '1') {
            runRandomPattern();
        }
    } else {
        Serial.print("P ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void rArgInterpreter() {
    if (setBufPointer == 1) {
        Serial.println("R");
        pinConfig();
    } else {
        Serial.print("R ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void sArgInterpreter() {
    if (setBufPointer == 1) { 
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    } else if ((setBufPointer == 2 || setBufPointer == 4) && 
          (serialBuffer[1] == 'T' || serialBuffer[1] == 't')) {
        stArgInterpreter();
    } else if ((setBufPointer == 2 || setBufPointer == 3) && 
        (serialBuffer[1] == 'S' || serialBuffer[1] == 's')) {
        ssArgInterpreter();
    } else if ((setBufPointer == 2) && 
        (serialBuffer[1] == 'R' || serialBuffer[1] == 'r')) {
        srArgInterpreter();
    } else if ((setBufPointer == 2 || setBufPointer == 4) && 
        (serialBuffer[1] == 'K' || serialBuffer[1] == 'k')) {
        skArgInterpreter();
    } else {
        Serial.print("S* ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void stArgInterpreter() {
    if (setBufPointer == 2) {
        Serial.print("ST");
        Serial.println(digitalRead(STEP), DEC);
    } else if (setBufPointer == 4) {
        int steps = digits2Value(serialBuffer[2], serialBuffer[3]);
        makeSteps(steps);
        Serial.print("ST");
        Serial.println(steps, DEC);
    } else {
        Serial.print("ST ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void ssArgInterpreter() {
    if (setBufPointer == 2) {
        Serial.print("SS");
        Serial.println(ssState, DEC);
    } else if (setBufPointer == 3) {
        ssState = serialBuffer[2] == '0' ? 0 : 1;
        digitalWrite(SIDESELECT, ssState);
        Serial.print("SS");
        Serial.println(ssState, DEC);
    } else {
        Serial.print("SS ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);
    }
}

void srArgInterpreter() {
    driveOn();
    unsigned int highTime;
    unsigned int lowTime;
    lowTime  = pulseIn(INDEX, LOW);
    highTime = pulseIn(INDEX, HIGH);
    double revolutionTime = lowTime + highTime;
    double rpm = 60000000.0/revolutionTime;
    Serial.print("SR ");
    Serial.print(" ");
    Serial.print(rpm, 2);
    Serial.println(" rpm");
    driveOff();
}

void skArgInterpreter() {
     if (setBufPointer == 2) {
          Serial.print("SK");
          Serial.println(track, DEC);
     } else if (setBufPointer == 4) {
          driveOn();
          byte wantedTrack = digits2Value(serialBuffer[2], serialBuffer[3]);
          seekTrack(wantedTrack);
          Serial.print("SK");
          Serial.println(track, DEC);
          driveOff();
     } else {
          Serial.print("SK ");
          Serial.print("unsupported arg size: ");
          Serial.println(setBufPointer);
    }
}

void tArgInterpreter() {
    if (setBufPointer == 1) { 
        Serial.print("T ");
        Serial.println(trackCount, DEC);
    } else if (setBufPointer == 3) {
        trackCount = digits2Value(serialBuffer[1], serialBuffer[2]);
        Serial.print("T ");
        Serial.println(trackCount, DEC);
    } else {
        Serial.print("T ");
        Serial.print("unsupported arg size: ");
        Serial.println(setBufPointer);     
    }
}

void clearSerialBuffer() {
  byte i;
  for (i = 0; i < SERIALBUFSIZE; i++) {
    serialBuffer[i] = 0;
  }
}

byte digits2Value(byte msd, byte lsd) {
    byte steps;
    steps  = (msd >= '0' && msd <= '9') ? (msd - '0') * 10 : 0;
    steps += (lsd >= '0' && lsd <= '9') ? (lsd - '0') : 0;
    return steps;
}

void makeSteps(int steps) {
    for (byte stepCount = 0; stepCount < steps; stepCount++) {
        digitalWrite(STEP, LOW);
        delay(STEPPULSE);
        digitalWrite(STEP, HIGH);
        delay(STEPRATE - STEPPULSE);
    }
    if (diState == 0)
        track = track + steps;
    else
        track = track - steps;
}

void gotoTrack0() {
    if (digitalRead(TRACK00) == 0) {
        track = 0;
        return;
    }
    digitalWrite(DIRECTION, 1);
    while (digitalRead(TRACK00) != 0) {
        makeSteps(1);
    }
}

void runUpDownPattern() {
    bool keyPress = 1;
    if (digitalRead(TRACK00) == 0) track = 0;
    while (keyPress) {
        gotoTrack0();
        Serial.println("P0 0");
        seekTrack(trackCount);
        Serial.print("P0 ");
        Serial.println(track, DEC);
        if (Serial.available() > 0) keyPress = 0;
    }
}

void runRandomPattern() {
    bool keyPress = 1;
    if (digitalRead(TRACK00) == 0) track = 0;
    if (track == 255) {
        gotoTrack0();
        track = 0;
    }
    while(keyPress) {
        int randomTrack = random(trackCount);
        seekTrack(randomTrack);
        Serial.print("P1 ");
        Serial.println(track, DEC);        
        if (Serial.available() > 0) keyPress = 0;
    } 
}

void seekTrack(byte wantedTrack) {
          if (digitalRead(TRACK00) == 0) track = 0;
          if (track == 255) {
              gotoTrack0();
              track = 0;
          }
          int steps = track - wantedTrack;
          if (steps < 0) {
              steps = -steps;
              digitalWrite(DIRECTION, 0);
          } else {
              digitalWrite(DIRECTION, 1);
          }
          makeSteps(steps);
          track = wantedTrack;
}

void driveOn() {
    digitalWrite(DRIVESEL, LOW);
    digitalWrite(MOTOR, LOW);
    delay(1000);
}

void driveOff() {
    digitalWrite(DRIVESEL, HIGH);
    digitalWrite(MOTOR, HIGH);
}

