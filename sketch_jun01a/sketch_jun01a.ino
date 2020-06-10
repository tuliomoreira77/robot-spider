#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "page.h" //Our HTML webpage contents with javascripts
#include "faces.h"
#include "pitches.h"
#include "sounds.h"

//SSID and Password of your WiFi router
const char* ssid = "Maracutaia Oi Fibra 2.4G";
const char* password = "maracutaia123";
ESP8266WebServer server(80); //Server on port 80

Adafruit_SSD1306 display(-1);
unsigned long nextBlink = 0;
unsigned long lastBlink = 0;
unsigned long lastVariation = 0;
unsigned long nextVariation = 0;
unsigned long nextEmote = 0;
String currentExpression = "sad";
String lastReceivedExpression = "sad";
unsigned long lastCommandReceived = 0;
unsigned long expressionServiceTick = 0;
boolean awaitCommandResponse = false;
const int buzzerPin = D8; 

char arduinoResponse[258];
int arduinoTimeout = 100;

boolean initRobot = false;

typedef struct
{
  unsigned long lastTick;
  int tickCount;
  int fitness;
  int happiness;
  int energy;
  int hungry;
  boolean sleeping;
  boolean sleepControl;
} Metabolism;

Metabolism metabolism = {.lastTick = 0, .tickCount = 0, .fitness = 800, .happiness = 800, .energy = 1000, .hungry = 255, .sleeping = false, .sleepControl = false};

typedef struct
{
  String frameName;
  unsigned long time;
  int offset_x;
  int offset_y;
} AnimationFrame;

class AnimationQueue {
  private:
    AnimationFrame queue[40];
    int length = 0;
    int queueSize = 40;
  public:
    String neutralFrame = "idle6";
    AnimationFrame* getQueue() {
      return queue;
    }
    void addToQueue(AnimationFrame frame) {
      if(length >= queueSize) {
        return;
      } else {
        queue[length] = frame;
        length++;
      }
    }
    AnimationFrame getFirst() {
      if(length != 0) {
          AnimationFrame frame = queue[0];
          return frame;
      }
    }

    AnimationFrame popFromQueue() {
      if(length != 0) {
        AnimationFrame frame = queue[0];
        for(int i=1; i<queueSize; i++) {
          queue[i-1] = queue[i];
        }
        length--;
        return frame;
      }
    }

    void emptyQueue() {
      if(length == 0) {
        return;
      } else {
        length = 0;
      }
    }

    int getSize() {
      return length;
    }

    int getFreeSize() {
      return queueSize - length;
    }
    
    int addToSize(int value) {
      length = length + value;
    }
};
AnimationQueue animationQueue;
String baseFrame = "idle6";

class CommandQueue {
  private:
    char queue[5];
    int length = 0;
  public:
    void addToQueue(char command) {
      if(length > 5) {
        return;
      } else {
        queue[length] = command;
        length++;
      }
    }

    char popFromQueue() {
      if(length == 0) {
        return '/0';
      } else {
        char command = queue[0];
        for(int i=1; i<5; i++) {
          queue[i-1] = queue[i];
        }
        length--;
        return command;
      }
    }

    void emptyQueue() {
      if(length == 0) {
        return;
      } else {
        length = 0;
      }
    }

    int getSize() {
      return length;
    }
};

CommandQueue commandQueue;

int StrToHex(char str[])
{
  return (int) strtol(str, 0, 16);
}
//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleReceiveSong() {
  String rawSong = server.arg("song");
  int intSize = 2*rawSong.length()/6;
  int song[intSize];
  int j=0;
  for(int i=0; i< rawSong.length(); i = i+6) {
    char temp1[5] = {rawSong[i], rawSong[i+1], rawSong[i+2], rawSong[i+3]};
    char temp2[3] = {rawSong[i+4],rawSong[i+5]}; 
    song[j] = StrToHex(temp1);
    song[j +1] = StrToHex(temp2);
    if(song[j+1] > 127) {
      song[j+1] = (song[j+1] -128) * -1;
    }
    j = j+2;
  }
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plane", "");  

  playSong(song, intSize/2, 88);
}

void handleRoot() {
  String s = MAIN_page; //Read HTML contents
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", s); //Send web page
}

void handleLED() {
  String t_state = server.arg("command"); //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
  commandQueue.addToQueue(t_state.charAt(0));
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plane", ""); //Send web page
}

void handleInitRobot() {
  initRobot = true;
  playSong(initBeep, initBeepLength, initBeepTempo);
  commandQueue.addToQueue('9');
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plane", ""); //Send web page
}

void handleReceivedFace() {
  String expression = server.arg("expression");
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.send(200, "text/plane", "");
}
//==============================================================
//                  SETUP
//==============================================================
void setup(void) {
  Serial.begin(115200);

  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/initRobot", handleInitRobot);
  server.on("/sendCommand", handleLED);
  server.on("/sendFace", handleReceivedFace);
  server.on("/playSong", handleReceiveSong);
  server.on("/", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Max-Age", "10000");
    server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "" );
  });

  server.begin();                  //Start server
  Serial.println("HTTP server started");
  pinMode(buzzerPin, OUTPUT); 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  playSong(happyBeep, happyBeepLength, happyBeepTempo);
}
//==============================================================
//                     LOOP
//==============================================================
void loop(void) {
  server.handleClient();          //Handle client requests
  if(initRobot == true) {
    verifyCommandResponse();
    calculateMetabolism();
    expressionService();
  }
}

void playSong(int *melody, int notes, int tempo) {
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
    divider = melody[thisNote + 1];
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5;
    }
    tone(buzzerPin, melody[thisNote], noteDuration * 0.9);
    delay(noteDuration);
    noTone(buzzerPin);
  }
}

void sendCommand(char c) {
  char command[3] = {'C', c};
  Serial.print(command);
  awaitCommandResponse = true;
  //String expression = getExpression2Movement(c);
  //currentExpression = expression;
  lastCommandReceived = millis();
}

/*SLTPPP
byte = Sync
byte = length
byte = type
n bytes = payload*/
void verifyCommandResponse() {
  if(Serial.available()) {
    unsigned long start = millis();
    arduinoResponse[0] = Serial.read();
    if(arduinoResponse[0] == 'S') {
      int count = 0;
      while(count == 0) { //Recebe o tamanho
        if(millis() >= start + arduinoTimeout) {
          return;
        }
        if(Serial.available()) {
          arduinoResponse[1] = Serial.read();
          count++;
        }
      }
      int index = 2;
      int length = arduinoResponse[1]; 
      while(index < length) { //Recebe o payload
        if(millis() >= start + arduinoTimeout) {
          return;
        }
        if(Serial.available()) {
          Serial.println(arduinoResponse[1]);
          arduinoResponse[index] = Serial.read();
          index++;
        }
      }
      if(arduinoResponse[2] == 'A') {
        metabolism.hungry = StrToHex(arduinoResponse +3);
        Serial.println(metabolism.hungry);
      }
    }  
  }
}

String getExpression2Movement(char movement) {
  switch(movement) {
    case '0':
        return "neutral";
        break;
      case '1':
        return "neutral";
        break;
      case '2':
        return "neutral";
        break;
      case '3':
       return "neutral";
        break;
      case '4':
        return "neutral";
        break;
      case '5':
        return "neutral";
        break;
      case '6':
        return "happy";
        break;
      case '7':
        return "neutral";
        break;
      case '8':
        return "closed";
        break;
      case '9':
        return "neutral";
        break;
  }
}

void calculateMetabolism() {
  unsigned long now = millis();
  if(now >= metabolism.lastTick + 500) {
    //Serial.println("Metabolismo: ");
    //Serial.println(metabolism.fitness);
    //Serial.println(metabolism.happiness);
    //Serial.println(metabolism.energy);
    //Serial.println(metabolism.hungry);
    //Serial.println(animationQueue.getFreeSize());
    if(metabolism.sleeping == true) {
      metabolism.energy = metabolism.energy + 5;
      if(metabolism.energy > 900) {
        injectSleep2NeutralAnimation();
        metabolism.happiness = 500;
        metabolism.fitness = 500;
        metabolism.sleeping = false;
        currentExpression = "neutral";
      }
    } else {
      if(metabolism.fitness >= 2)
        metabolism.fitness = metabolism.fitness -2;
      if(metabolism.fitness < 350) {
        if(metabolism.happiness >= 1)
          metabolism.happiness = metabolism.happiness -1;
      }
      if(metabolism.energy < 200) {
        if(metabolism.happiness >= 1)
          metabolism.happiness = metabolism.happiness -1;
      }
      if(metabolism.energy < 50) {
        injectSleepAnimation();
        metabolism.sleeping = true;
        metabolism.sleepControl = false;
        currentExpression = "closed";
        commandQueue.emptyQueue();
        commandQueue.addToQueue('8');
      }
      if(metabolism.tickCount >= 20) {
        metabolism.tickCount = 0;
        if(metabolism.energy > 1) {
          metabolism.energy = metabolism.energy -5;
        }
      }
      if(metabolism.hungry < 200) {
        animationQueue.neutralFrame = "idle23";
      }
    }
    metabolism.lastTick = now;
    metabolism.tickCount++;
    takeAction(now);
  }
}

void takeAction(unsigned long now) {
  if(metabolism.sleeping != true) {
    processFace();
    processQueue();
  } else {
    if(metabolism.sleepControl == false) {
      processQueue();
      metabolism.sleepControl = true;
    }
  }
}

void processFace() {
  if(metabolism.sleeping == true) {
    currentExpression = "closed";
  } else if(metabolism.happiness < 50) {
    currentExpression = "angry";
  } else if (metabolism.happiness < 300) {
    currentExpression = "sad";
  } else if (metabolism.fitness < 500) {
    currentExpression = "bored";
  } else if (metabolism.happiness > 700) {
    currentExpression = "happy";
  } else {
    currentExpression = "neutral";
  }
}

void processQueue() {
  if(commandQueue.getSize() > 0) {
    char c = commandQueue.popFromQueue();
    switch(c) {
      case '0':
        metabolism.fitness = metabolism.fitness + 5;
        metabolism.energy = metabolism.energy - 5;
        break;
      case '1':
        metabolism.fitness = metabolism.fitness + 2;
        metabolism.energy = metabolism.energy -2;
        break;
      case '2':
        metabolism.fitness = metabolism.fitness + 20;
        metabolism.energy = metabolism.energy - 20;
        metabolism.happiness = metabolism.happiness + 10;
        break;
      case '3':
       metabolism.fitness = metabolism.fitness + 20;
        metabolism.energy = metabolism.energy - 20;
        metabolism.happiness = metabolism.happiness + 10;
        break;
      case '4':
        metabolism.fitness = metabolism.fitness + 20;
        metabolism.energy = metabolism.energy - 20;
        metabolism.happiness = metabolism.happiness + 10;
        break;
      case '5':
        metabolism.fitness = metabolism.fitness + 20;
        metabolism.energy = metabolism.energy - 20;
        metabolism.happiness = metabolism.happiness + 10;
        break;
      case '6':
        metabolism.fitness = metabolism.fitness + 10;
        metabolism.energy = metabolism.energy - 10;
        metabolism.happiness = metabolism.happiness + 50;
        break;
      case '7':
        metabolism.fitness = metabolism.fitness + 10;
        metabolism.energy = metabolism.energy - 10;
        metabolism.happiness = metabolism.happiness + 50;
        break;
      case '8':
        break;
      case '9':
        break;
    }
    sendCommand(c);
  }
}

void expressionService() {
    unsigned long now = millis();
    if(metabolism.sleeping == false) {
      blinkEyes(now);
      injectSmallVariation(now);
      if(metabolism.hungry > 50) {
        injectEmote(now);
      }
    }
    if(animationQueue.getSize() > 0) {
      AnimationFrame frame = animationQueue.getFirst();
      if(now >= expressionServiceTick + frame.time) {
        display.clearDisplay();
        display.drawBitmap(frame.offset_x, frame.offset_y, string2Frame(frame.frameName), 128, 64, 1);
        display.display();
        animationQueue.popFromQueue();
        expressionServiceTick = now;
      }
    }
}

void injectEmote(unsigned long int now) {
  if(now >= nextEmote) {
    nextEmote = now + random(5000, 20000);
    if(currentExpression.equals("sad")) {
      injectSadAnimation();
    } else if(currentExpression.equals("angry")) {
      injectAngryAnimation();
    } else if(currentExpression.equals("happy")) {
      injectHappyAnimation();
    }
  }
}

void injectSmallVariation(unsigned long int now) {
  if(now >= nextVariation) {
    lastVariation = now;
    nextVariation = now + random(1000, 5000);
    injectNeutralSmallAnimation();
  }
}

void blinkEyes(unsigned long int now) {
  if (now >= nextBlink) {
    lastBlink = now;
    nextBlink = now + random(3000, 15000);
    if(currentExpression.equals("lalala")) {
        injectSadBlinkAnimation();
    } else {
        injectNeutralBlinkAnimation();
    }
  }
}

//ANIMATIONS
const unsigned char* string2Frame(String frameName) {
  if(frameName.equals("idle6")) {
    return idle6;
  }
  if(frameName.equals("idle6")) {
    return idle6;
  }
  if(frameName.equals("idle9")) {
    return idle9;
  }
  if(frameName.equals("idle10")) {
    return idle10;
  }
  if(frameName.equals("idle11")) {
    return idle11;
  }
  if(frameName.equals("idle12")) {
    return idle12;
  }
  if(frameName.equals("idle17")) {
    return idle17;
  }
  if(frameName.equals("idle18")) {
    return idle18;
  }
  if(frameName.equals("idle19")) {
    return idle19;
  }
  if(frameName.equals("idle20")) {
    return idle20;
  }
  if(frameName.equals("idle21")) {
    return idle21;
  }
  if(frameName.equals("idle22")) {
    return idle22;
  }
  if(frameName.equals("idle23")) {
    return idle23;
  }
}

// CORRIGIR BUG ONDE O TEMPO DA ANIMACAO E CONTROLADO PELO FRAME DA FRENTE
//------------NEUTRAL
void injectNeutralAnimation() {
  if(animationQueue.getFreeSize() < 1) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  frames[index] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=0, .offset_y=0};
  animationQueue.addToSize(1);
}

//---------NEUTRAL-SMALL-VARIATION 
int getNeutralSmallAnimationSize() {
  return 3;
}

void injectNeutralSmallAnimation() {
  if(animationQueue.getFreeSize() < getNeutralSmallAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  int randon_x = random(0,8) - 3;
  int randon_y = random(0,8) -3;
    
  frames[index] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = animationQueue.neutralFrame, .time = 50, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+2] = {.frameName = animationQueue.neutralFrame, .time = 500, .offset_x=randon_x, .offset_y=randon_y};
  
  animationQueue.addToSize(getNeutralSmallAnimationSize());
}

//--------------NEUTRAL-BLINK
int getNeutralBlinkAnimationSize() {
  return 6;
}
void injectNeutralBlinkAnimation() {
  if(animationQueue.getFreeSize() < getNeutralBlinkAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  frames[index] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = "idle9", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+2] = {.frameName = "idle10", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+3] = {.frameName = "idle11", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+4] = {.frameName = "idle12", .time = 100, .offset_x=0, .offset_y=0};
  frames[index+5] = {.frameName = animationQueue.neutralFrame, .time = 25, .offset_x=0, .offset_y=0};
  animationQueue.addToSize(getNeutralBlinkAnimationSize());
}

//-----------------HAPPY
int getHappyAnimationSize() {
  return 6;
}
void injectHappyAnimation() {
  if(animationQueue.getFreeSize() < getAngryAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  int randon_x = random(0,8) - 3;
  int randon_y = random(0,8) -3;
  frames[index] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = "idle6", .time = 100, .offset_x=randon_x, .offset_y=randon_x};
  frames[index+2] = {.frameName = "idle21", .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+3] = {.frameName = "idle22", .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+4] = {.frameName = "idle21", .time = 1000, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+5] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  animationQueue.addToSize(getHappyAnimationSize());
}

//-------------ANGRY
int getAngryAnimationSize() {
  return 6;
}
void injectAngryAnimation() {
  if(animationQueue.getFreeSize() < getAngryAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  int randon_x = random(0,8) - 3;
  int randon_y = random(0,8) -3;
  frames[index] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = "idle6", .time = 100, .offset_x=randon_x, .offset_y=randon_x};
  frames[index+2] = {.frameName = "idle19", .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+3] = {.frameName = "idle20", .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+4] = {.frameName = "idle19", .time = 1000, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+5] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  animationQueue.addToSize(getAngryAnimationSize());
}

//----------------SLEEP
int getSleep2NeutralAnimationSize() {
  return 3;
}
void injectSleep2NeutralAnimation() {
  if(animationQueue.getFreeSize() < getSleepAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  frames[index] = {.frameName = "idle11", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = "idle12", .time = 100, .offset_x=0, .offset_y=0};
  frames[index+2] = {.frameName = animationQueue.neutralFrame, .time = 25, .offset_x=0, .offset_y=0};
  animationQueue.addToSize(getSleep2NeutralAnimationSize());
}

int getSleepAnimationSize() {
  return 4;
}
void injectSleepAnimation() {
  if(animationQueue.getFreeSize() < getSleepAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  frames[index] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = "idle9", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+2] = {.frameName = "idle10", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+3] = {.frameName = "idle11", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+4] = {.frameName = "idle11", .time = 1000, .offset_x=0, .offset_y=0};
  animationQueue.addToSize(getSleepAnimationSize());
}

//-------------SAD
int getSadAnimationSize() {
  return 6;
}
void injectSadAnimation() {
  if(animationQueue.getFreeSize() < getSadAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  int randon_x = random(0,8) - 3;
  int randon_y = random(0,8) -3;
  frames[index] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=randon_x, .offset_y=randon_x};
  frames[index+2] = {.frameName = "idle17", .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+3] = {.frameName = "idle18", .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+4] = {.frameName = "idle17", .time = 1000, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+5] = {.frameName = animationQueue.neutralFrame, .time = 100, .offset_x=randon_x, .offset_y=randon_y};
  animationQueue.addToSize(getSadAnimationSize());
}

int getSadSmallAnimationSize() {
  return 3;
}
void injectSadSmallAnimation() {
  if(animationQueue.getFreeSize() < getSadSmallAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  int randon_x = random(0,8) - 3;
  int randon_y = random(0,8) -3;
  frames[index] = {.frameName = "idle18", .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = "idle18", .time = 50, .offset_x=randon_x, .offset_y=randon_y};
  frames[index+2] = {.frameName = "idle18", .time = 500, .offset_x=randon_x, .offset_y=randon_y};
  animationQueue.addToSize(getSadSmallAnimationSize());
}

int getSadBlinkAnimationSize() {
  return 6;
}
void injectSadBlinkAnimation() {
  if(animationQueue.getFreeSize() < getSadBlinkAnimationSize()) {
    return;
  }
  AnimationFrame* frames = animationQueue.getQueue();
  int index = animationQueue.getSize();
  frames[index] = {.frameName = "idle18", .time = 100, .offset_x=0, .offset_y=0};
  frames[index+1] = {.frameName = "idle9", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+2] = {.frameName = "idle10", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+3] = {.frameName = "idle11", .time = 25, .offset_x=0, .offset_y=0};
  frames[index+4] = {.frameName = "idle12", .time = 100, .offset_x=0, .offset_y=0};
  frames[index+5] = {.frameName = "idle18", .time = 25, .offset_x=0, .offset_y=0};
  animationQueue.addToSize(getSadBlinkAnimationSize());
}

