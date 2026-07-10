#include <SPI.h>
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
#include <HMC5883L_Simple.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//top 8 px rows are yellow, 2 dead px rows, then 54 px rows of blue
// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C

//button functionality
const byte  buttonPin = 2;    // the pin that the pushbutton is attached to for checking direction orientation and changing the multicolor LED
const byte  buttonPin2 = 3;    // the pin that the pushbutton is attached to for playing the d6 game
//Defining multicolor LED variable and the GPIO pin on Arduino ******assign once you choose them!!******
int redPin= 5;
int greenPin = 6;
int  bluePin = 7;

long rolldie;

// Variables will change:
boolean buttonState = 0;         // current state of the button
boolean lastButtonState = 0;     // previous state of the button
boolean buttonState2 = 0;         // current state of the button for d6 game
boolean lastButtonState2 = 0;     // previous state of the button

DHT dht(4, DHT11);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create a compass
HMC5883L_Simple Compass;

void setup() {
  // put your setup code here, to run once:

 Serial.begin(9600);
  // initialize the OLED object
 if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
   Serial.println(F("SSD1306 allocation failed"));
   for(;;); // Don't proceed, loop forever
 }
 //for randomizing
   randomSeed(analogRead(0));
   // initialize the button pin as a input:
   pinMode(buttonPin, INPUT_PULLUP);
	 pinMode(buttonPin2, INPUT_PULLUP);
   
  //Defining the multicolor LED pins as OUTPUT
	pinMode(redPin,  OUTPUT);              
	pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
	  Wire.begin();
    //***************Compass Callibration********************
  // Magnetic Declination is the correction applied according to your present location
  // in order to get True North from Magnetic North, it varies from place to place.
  // 
  // The declination for your area can be obtained from http://www.magnetic-declination.com/
  // Take the "Magnetic Declination" line that it gives you in the information, 
  //
  // Examples:
  //   Christchurch, 23° 35' EAST
  //   Wellington  , 22° 14' EAST
  //   Dunedin     , 25° 8'  EAST
  //   Auckland    , 19° 30' EAST
  //		Oregon		,	15° EAST   ish -- we aren't going to care too much
	//
  Compass.SetDeclination(15, 0, 'E');  
  
  // The device can operate in SINGLE (default) or CONTINUOUS mode
  //   SINGLE simply means that it takes a reading when you request one
  //   CONTINUOUS means that it is always taking readings
  // for most purposes, SINGLE is what you want.
	// -- continuous would be good for magnet detection?
  Compass.SetSamplingMode(COMPASS_SINGLE);
  
  // The scale can be adjusted to one of several levels, you can probably leave it at the default.
  // Essentially this controls how sensitive the device is.
  //   Options are 088, 130 (default), 190, 250, 400, 470, 560, 810
  // Specify the option as COMPASS_SCALE_xxx
  // Lower values are more sensitive, higher values are less sensitive.
  // The default is probably just fine, it works for me.  If it seems very noisy
  // (jumping around), incrase the scale to a higher one.
  Compass.SetScale(COMPASS_SCALE_190);
  
  // The compass has 3 axes, but two of them must be close to parallel to the earth's surface to read it, 
  // (we do not compensate for tilt, that's a complicated thing) - just like a real compass has a floating 
  // needle you can imagine the digital compass does too.
  //
  // To allow you to mount the compass in different ways you can specify the orientation:
  //   COMPASS_HORIZONTAL_X_NORTH (default), the compass is oriented horizontally, top-side up. when pointing North the X silkscreen arrow will point North
  //   COMPASS_HORIZONTAL_Y_NORTH, top-side up, Y is the needle,when pointing North the Y silkscreen arrow will point North
  //   COMPASS_VERTICAL_X_EAST,    vertically mounted (tall) looking at the top side, when facing North the X silkscreen arrow will point East
  //   COMPASS_VERTICAL_Y_WEST,    vertically mounted (wide) looking at the top side, when facing North the Y silkscreen arrow will point West  
  Compass.SetOrientation(COMPASS_HORIZONTAL_X_NORTH); //I placed the module to point at a 90 to the right from one of the screens, so EAST
}

void loop() {
 dht.begin();
 display.clearDisplay();
 dht.read();
 //Compass Readings

 	 float heading = Compass.GetHeadingDegrees();
   
   
//Temp and Humidity sensor code
	//calculate temp into F
	float temp = dht.readTemperature();
	float humidity = dht.readHumidity();
	float headingt = heading;
	temp = (temp * 9/5) + 32;
	
	//fixing temp and humidity for display
  char hum1 [6]; 
  char temp1 [7];
	char head1 [8];
	char dir = 'N ';
  dtostrf(humidity, 2, 1, hum1); //converts to a string with only one decimal point (no longer a value!!)
  dtostrf(temp, 2, 1, temp1); //convers to a string with only one decimal point (no longer a value!!)
	dtostrf(headingt, 2, 1, head1); //same for heading


//serial monitor messages
	Serial.println();
	Serial.println();
	Serial.print("Temp: ");
	Serial.print(temp);
	Serial.print(" F");
	Serial.println();
	
	Serial.print("Hum is: ");
	Serial.print(humidity);
	Serial.print("%");
  Serial.println();
	Serial.print("Heading: ");
	Serial.print(head1);
	
	Serial.print(" degrees ");
 
//top infobar layout on .96 OLED screen
 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(0, 8);

 display.print(head1);
 display.print(" "); //placeholder for compass direction
 display.print("   ");
 display.print(temp1); // placeholder for compass bearing
 display.print("F");
 display.print("  ");
 display.print(hum1);
 display.print("%");

 display.display();


//Multicolor LED colors
	//sets the color back to light blue after 6 seconds
	static unsigned long timer1 = 0;
  unsigned long interval1 = 6000;
  if (millis() - timer1 >= interval1)
  	{
  	timer1 = millis();
	//  setColor(255, 0, 0); // Red Color
	//  setColor(0,  255, 0); // Green Color
	//  setColor(0,  255, 55); // Green blue Color?
	  setColor(0, 5, 130); // Blue Color
		//setColor(0, 50, 155); // Blue Color
	//  setColor(255, 255, 255); // White Color
	//  setColor(170, 0, 255); // Purple Color
	  //setColor(127, 127,  127); // Light Blue
		//setColor(200, 0, 150); //Magenta 3 red...
		//setColor(255, 204, 18); //Mustard
		//setColor(255, 255, 0); //Yellow
		//setColor(255, 212, 0); //Gold
		//setColor(74, 227, 13); //Nerf Green 29/89/5
		//setColor(13, 191, 232); //Diamond Blue 5/75/91
		}


// read the state of the pushbutton value:
 static unsigned long timer = 0;
   unsigned long interval = 20;
   if (millis() - timer >= interval)
   {
      timer = millis();
      
      // read the pushbuttons input pin:
      buttonState = digitalRead(buttonPin);
			buttonState2 = digitalRead(buttonPin2);
      // compare the buttonState to its previous state
      if (buttonState != lastButtonState)
      {
         // if the state has changed, increment the counter
         if (buttonState == LOW)
         {
            	//draw lines down the screen animation
							int ll = 0;
							int rex = 0;
							int rey = 10;
							for (ll = 0; ll < 15; ll++)
							{
								
								display.drawRect(rex, rey, 128, 2, WHITE); //x, y, w, h, color
								display.display();
								rey = rey + 5;
							}
							rex = 2;
							rey = 0;
							display.drawRect(69, 0, 2, 64, WHITE); //x, y, w, h, color -- draw goal line
							display.display();
							for (ll > 12; ll < 45; ll++)
							{
								if (rex < heading)
								{
								display.drawTriangle(rex - 20, 30, rex - 20, 60, rex, 45, WHITE); //x, y, w, h, color -- draw animation verical lines
								display.display();
								rex = rex + 5;
								}
								else{
									display.drawRect(heading, 0, 2, 64, WHITE); // draw bearing line
									delay(300);
								}
							}
							display.clearDisplay(); //end of lines animation
							
							display.drawRect(2, 20, 120, 44, WHITE);
							display.display();
						// if the current state is LOW 
						if (heading < 70.00 && heading > 68.99)
						{
							setColor(0,  255, 0); // Green Color
						  Serial.println("");
							Serial.println("SUCCESS!");
							
							display.setTextSize(2);
 							display.setTextColor(WHITE);
 							display.setCursor(10, 35);

 							display.println("SUCCESS!!");
							display.display();
							delay(5000);
						}
						else
						{

							setColor(255, 0, 0); // Red Color
							Serial.println("");
							Serial.println("UNALIGNED");

							display.setTextSize(2);
 							display.setTextColor(WHITE);
 							display.setCursor(10, 35);

 							display.println("UNALIGNED");
							display.display();
							delay(5000);
							display.clearDisplay();
						}
         }
         else {}
          // save the current state as the last state, for next time through the loop
          lastButtonState = buttonState;
      }
			else{}

			if (buttonState2 != lastButtonState2)
      {
				if (buttonState2 == LOW)
         {
					    //draw expanding rectangles animation
							int ll = 0;
							int rex = 58;
							int rey = 34;
							int rew = 10;
							int reh = 2;
							for (ll = 0; ll < 15; ll++)
							{
								
								display.drawRect(rex, rey, rew, reh, WHITE); //x, y, w, h, color
								display.display();
								rex = rex - 5;
								rey = rey - 5;
								rew = rew + 10;
								reh = reh + 10;


							}
							display.clearDisplay(); //clear at end of animation

					//play game! - random integer 1-6, if tree for the result
									display.drawRect(2, 20, 124, 44, WHITE);
									display.display();
							int rolldie;
							rolldie = random(1, 6);
							Serial.println("");
							Serial.println("d6 Result: ");					
							Serial.print(rolldie);
							Serial.println("");
							if (rolldie == 1)
								{
								Serial.println("Person to your RIGHT drinks!");
								Serial.println("");
								setColor(155,  55, 55); // magenta Color?

								display.setTextSize(1);
 								display.setTextColor(WHITE);
 								display.setCursor(15, 30);

 								display.println("Person to your");
								display.setCursor(15, 40);
								display.println("RIGHT drinks!");
								display.display();
								}
							else if (rolldie == 2)
								{
								Serial.println("Person to your LEFT drinks!");
								Serial.println("");
								setColor(0,  255, 55); // Green blue Color?
								
								display.setTextSize(1);
 								display.setTextColor(WHITE);
 								display.setCursor(15, 30);

 								display.println("Person to your");
								display.setCursor(15, 40);
								display.println("LEFT drinks!");
								display.display();
								}
							else if (rolldie == 3)
									{
								Serial.println("YOU drink!");
								Serial.println("");
								setColor(0, 0, 55); // Blue Color
																
								display.setTextSize(1);
 								display.setTextColor(WHITE);
 								display.setCursor(15, 30);

 								display.println("YOU Drink!");
								display.display();
							
									}
						else if (rolldie == 4)
							{
								Serial.println("CHOOSE the Person who drinks!");
								Serial.println("");
								setColor(170, 0, 255); // Purple Color

								display.setTextSize(1);
 								display.setTextColor(WHITE);
 								display.setCursor(15, 30);

 								display.println("CHOOSE");
								display.setCursor(15, 40);
								display.println("who drinks!");
								display.display();
							}
						else if (rolldie == 5)
							{
								Serial.println("Everyone Drinks!");
								Serial.println("");
								setColor(255,  255, 0); // yellow Color?
								
								display.setTextSize(1);
 								display.setTextColor(WHITE);
 								display.setCursor(15, 30);

 								display.println("EVERYONE");
								display.setCursor(15, 40);
								display.println("drinks!");
								display.display();
							}
						else if (rolldie == 6)
							{
								Serial.println("Person to your LEFT AND RIGHT drink!");
								Serial.println("");
								setColor(0,  155, 155); // Green blue Color?
								
								display.setTextSize(1);
 								display.setTextColor(WHITE);
 								display.setCursor(15, 30);

 								display.println("Person to your LEFT");
								display.setCursor(15, 40);
								display.println("AND RIGHT drink!");
								display.display();
						}
							delay(6000);
							display.clearDisplay();
				 }
				lastButtonState2 = buttonState2;
			}

  }
	else{}

//graphical representation of readings for funsies	
	//draw circle
	display.setCursor(10, 30);
	//display.println("Circle");
	display.drawCircle(64, 37, 20, WHITE);
	display.display();

	//draw triangle for humidity and temp
	// x0 is temp offset to bar, y0, x1 is humidity offset to bar, y1, x2 is nothing yet, y2, color
	//keep between 0-128x and 8-64 y
	display.drawTriangle(temp + 20, 25, 40, 40, humidity + 20, 60, WHITE);
	display.drawRect(20, 15, 1, 60, WHITE);

	//convert heading to a dot on the circle
	float dotx = sin(heading);
	float doty = cos(heading);
	dotx = dotx*10;
	doty = doty*10;
	dotx = dotx + 64;
	doty = doty + 40;
	display.fillCircle(dotx, doty, 5, WHITE);

	display.display();
 
 delay(500);
 

}

void setColor(int redValue, int greenValue,  int blueValue) {
  analogWrite(redPin, redValue);
  analogWrite(greenPin,  greenValue);
  analogWrite(bluePin, blueValue);
}

