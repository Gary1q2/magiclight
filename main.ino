#include <JsonParserGeneratorRK.h>
#include <neopixel.h>


#define PIXEL_COUNT 24
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B  

#define BUTTON D5
#define POT A1
#define HOLD_TIME 3
#define NUM_COLORS 10
#define LIMBO 69
#define TIMER 7200

Adafruit_NeoPixel ring(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE); // Create ring object


bool lightOn = false;   // Light status
int startPress = 0;     // Count down time to close lights
int cantPress = false;  // Prevents triggering button hold multiple times
int currColor = -1;     // Current color of LEDs
int brightness = 20;    // Current brightness of LEDs
int lastOn = 0;         // Keep track when light was last turned on or pressed
int red = 0;              
int green = 0;          // Rememeber the RGB values
int blue = 0;

int shutSec = -1;       // Timer to shut off the light
int currSec = -1;       // Stores current Time.second() to check if a second has passed

int potValue = 0;

// Color array containing 8 colors represented with 3 values
int colorArray[NUM_COLORS][3] = {{255,   0,   0},  // Red
                                 {255,  60,   0},  // Orange
                                 {255, 191,   0},  // Yellow
                                 {191, 255,   0},  // kinda meh light green?
                                 {  0, 255,   0},  // Green
                                 {  0, 255, 255},  // Light blue
                                 {  0,   0, 255},  // Blue
                                 {140,   0, 255},  // Purple  hmm
                                 {255,   0, 191},  // Pink hmmm
                                 {255, 255, 255}}; // White
                        




void handler(const String& event, const String& data);
void setLEDColor(const int& color, const int& delayTime);
void publishEvent(const String& event, const String& message);
void turnOff(const int& delayTime);
int approachValue(const int& value, const int& target);
void spiralLight(const int& color, const int& delayTime);
void rainbowOff(const int& delayTime);
void raving();
void updatePotBrightness(); 

void setup() {
    Serial.begin(9600);
    Serial.println("Setup!");
    
    // Setup switch
    pinMode(BUTTON, INPUT_PULLUP);
    
    // Start the NeoPixel object
    ring.begin();       
    ring.setBrightness(brightness);
    
    // Subscribe to other light's events
    Particle.subscribe("turn_on", handler, MY_DEVICES);
    Particle.subscribe("turn_off", handler, MY_DEVICES);
}


void loop() {
    
    // Update global brightness based on pot
    if (cantPress != LIMBO) {
        updatePotBrightness();
    }

    // Auto turn off light if time is up
    if (lightOn == true) {
        if (currSec == -1) { currSec = Time.second(); }
        
        // Counting down the timer using Time.second()
        if (currSec != Time.second()) {
            currSec = Time.second();
            shutSec--;
            Serial.printlnf("shutSec = %d   ------ lightOn = %d", shutSec, lightOn);
        }
        
        // Turn off light once timer is up
        if (shutSec <= 0) {
            Serial.println("10 seconds up, LIGHTS AUTO TURNING OFF");
            rainbowOff(4);
        }
    }
    
    int buttonState = digitalRead(BUTTON);
    
    // No button pressed
    if (buttonState == HIGH) {
        
        // Button was just released
        if (cantPress == true || cantPress == LIMBO) {
            Serial.println("Button released");
            
            if (cantPress == true) {
                
                // Set all lights to the same random colour + don't allow same color as before
                int randColor = random(NUM_COLORS);
                while (randColor == currColor) { randColor = random(NUM_COLORS); }
                spiralLight(randColor, 15);
                
                shutSec = TIMER;
                
                // Publish TURN ON event
                String message = "{\"id\":" + System.deviceID() + ",\"color\":"+ randColor +"}";
                publishEvent("turn_on", message);
            }
            
            if (cantPress == LIMBO) {
                turnOff(10);
            }
            
            cantPress = false;
            startPress = 0;    
        }
       
    // Button is being pressed
    } else  {
        
        // Button is pressed + registered
        if (cantPress == false) {
            Serial.println("Button pressed DOWN");
        
            cantPress = true;
            startPress = Time.second();
    
        // Button is still being held down
        } else if (cantPress == true) {
            if (Time.second() - startPress >= HOLD_TIME) {
                Serial.println("Held for 3 seconds, lights OFF");
                
                // Set all lights to blank
                rainbowOff(4);
                
                // Publish TURN OFF event
                String message = "{\"id\":" + System.deviceID() + "}";
                publishEvent("turn_off", message);

                cantPress = LIMBO;
                delay(20);
            }
            
        // Button must have been clacked...
        } else if (cantPress == LIMBO) {
            Serial.printlnf("Raving");
            raving();
        }
    }
    
    ring.show();
}
















// Handles the events
void handler(const String& event, const String& data) {
    
    // Parse the JSON       
    JsonParser parser;
    parser.addString(data);
    parser.parse();
    
    String id;
    parser.getOuterValueByKey("id", id);
    
    // Don't receive own messages
    if (id != System.deviceID()) {
        Serial.println("Received event = " + event);
    
        if (event == "turn_on") {
            int color;
            parser.getOuterValueByKey("color", color);
            
            // Turn lights on to the corresponding colour
            spiralLight(color, 15);
            shutSec = TIMER;
            
        } else if (event == "turn_off") {
            
            // Turn lights off
            rainbowOff(4);
        }
    }
}

// Set the lights to flash through the rainbow (cowboy style, doesn't set global variables)
void raving() {
    int delayTime;
    
    // Set brightness to 1 (1 because otherwise light closes)
    brightness = 1;
    ring.setBrightness(brightness);
    ring.show();
    
    for (int i = 0; i < NUM_COLORS; i++) {
        
        // Set the delayTime based on pot
        potValue = analogRead(POT);
        int multiple = 5;
        delayTime = ((map(potValue, 0, 4095, 10, 100) + multiple/2) / multiple) * multiple / 10;
 
        if (i == 1 || i == 3 || i == 8) {
            continue;
        }
 
        Serial.printlnf("Color = %d", i);
        // Set ring to a certain colour
        Serial.printlnf("Colors [%d, %d, %d]", colorArray[i][0], colorArray[i][1], colorArray[i][2]);
        for (int j = 0; j < PIXEL_COUNT; j++) {
            ring.setPixelColor(j, colorArray[i][0], colorArray[i][1], colorArray[i][2]);
        }
        ring.show();

        // Delay before flashing in
        delay(delayTime*10);

        // Flashing in
        while (brightness < 100) {
            brightness++;
            ring.setBrightness(brightness);
            ring.show();
            delay(delayTime/12);
        }
        
        // Delay before flashing out
        delay(delayTime*6);
        
        // Flashing out
        while (brightness > 1) {
            brightness--;
            ring.setBrightness(brightness);
            ring.show();
            delay(delayTime/8);
        }
    }
}

// Check's the pot and updates the global brightness
void updatePotBrightness() {
    potValue = analogRead(POT);
    int multiple = 5;
    int tempBright = ((map(potValue, 0, 4095, 10, 100) + multiple/2) / multiple) * multiple; 
    if (tempBright != brightness) {
        ring.setBrightness(tempBright);
        brightness = tempBright;
        Serial.printlnf("Brightness set to %d", brightness);
    }
}


// Sets the ring to a specific color
// 0 - 8 = a nice colour
void setLEDColor(const int& color, const int& delayTime) {
    Serial.printlnf("SetLEDColor called with color = %d", color);

    int r = colorArray[color][0];
    int g = colorArray[color][1];
    int b = colorArray[color][2];
    
    
    //Serial.printlnf("Fading goals = [%d, %d, %d]", r, g, b);
    //Serial.printlnf("before fade.... red = %d  green = %d   blue = %d", red, green, blue);
    while (red != r || green != g || blue != b) {
        red = approachValue(red, r);
        green = approachValue(green, g);
        blue = approachValue(blue, b);
        
        for (int i = 0; i < PIXEL_COUNT; i++) {
            ring.setPixelColor(i, red, green, blue);
        }
        ring.show();
        delay(delayTime);
        
        //Serial.printlnf("iterating... red = %d  green = %d   blue = %d", red, green, blue);
    }
    //Serial.printlnf("AFTER fade.... red = %d  green = %d   blue = %d", red, green, blue);
    
    
    lightOn = true;
    lastOn = Time.second();
    //Serial.printlnf("LastOn set to = %d", lastOn);
    currColor = color;
}

// Given a current value and a target value, make value approach target value in increments of 10
int approachValue(const int& value, const int& target) {
    int skip = 10;  // How much to increment
    
    if (value > target) {
        if (value >= target + skip) {
            return value - skip;
        } else {
            return target;
        }
    } else if (value < target) {
        if (value <= target - skip) {
            return value + skip;
        } else {
            return target;
        }
    }
}



// Light up the ring using a spiral effect
void spiralLight(const int& color, const int& delayTime) {
    Serial.printlnf("spiralLight called with color = %d", color);

    int r = colorArray[color][0];
    int g = colorArray[color][1];
    int b = colorArray[color][2];
    
    for (int i = 0; i < PIXEL_COUNT; i++) {
        ring.setPixelColor(i, r, g, b);
        ring.show();
        delay(delayTime);
    }
    Serial.printlnf("set pixel colors to %d %d %d", r, g, b);
    
    // Set global variables
    red = r;
    green = g;
    blue = b;
    
    lightOn = true;
    lastOn = Time.second();

    currColor = color;
}

// Give a nice rainbow before turning off
void rainbowOff(const int& delayTime) {
    Serial.printlnf("Rainbowing off");
    
    for (int i = 0; i < NUM_COLORS; i++) {
        spiralLight(i, delayTime);
    }
    turnOff(2);
}



// Turn off LED's via fading
void turnOff(const int& delayTime) {
    Serial.printlnf("Turning off");

    while (brightness > 0) {
        brightness--;
        ring.setBrightness(brightness);
        ring.show();
        delay(delayTime);
    }
    
    red = 0;
    green = 0;
    blue = 0;
    lightOn = false;
    currColor = -1;
    
    shutSec = -1;
    currSec = -1;
}




// Publishes private event + gives error message if it fails
void publishEvent(const String& event, const String& message) {
    
    bool success;
    success = Particle.publish(event, message, PRIVATE);
    if (!success) {
        Serial.println("Publishing event [" + event + "] FAILEDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD :(");
    } else {
        Serial.println("Published event [" + event + "] successful!");
    }
            
}