#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "lvgl.h"
#include "../lib/lvgl/examples/lv_examples.h"
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>

//servo definitions
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40); // Default I2C address for PCA9685
#define SERVOMIN 550  // Minimum pulse length (0 degrees)
#define SERVOMAX 2800  // Maximum pulse length (180 degrees)

static int questcounter = 0; // Add a quest counter to track the number of quests completed


int angleToPulse(int ang) {
  int pulse = map(ang, 0, 180, SERVOMIN, SERVOMAX);
  Serial.print("Angle: "); Serial.print(ang);
  Serial.print(" pulse: "); Serial.println(pulse);
  return pulse;
}


//custom I2C pin definitions, change to your pins
#define I2C_SDA 27
#define I2C_SCL 22
#define WIRE Wire

//onboard LED pin definition
#define LED_RED 4
#define LED_GREEN 16
#define LED_BLUE 17


static int value = 0;
static lv_timer_t * Slidetimer = NULL;
static int count = 0;
int headdirection = 0;
int headangle = 90;
int headgo = 1; // This variable will control whether the head should move or not
int mouthangle = 0;
int neckangle = 90;
int neckdirection = 0;

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

enum { SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10 };
static lv_color_t buf [SCREENBUFFER_SIZE_PIXELS];

TFT_eSPI tft = TFT_eSPI( screenWidth, screenHeight ); /* TFT instance */

/*Touch screen config*/
#define XPT2046_IRQ 36 //GPIO driver cảm ứng
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass tsSpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
//Run calib_touch files to get value
//uint16_t touchScreenMinimumX = 15, touchScreenMaximumX = 325, touchScreenMinimumY = 1,touchScreenMaximumY = 230; //Chạy Calibration để lấy giá trị mỗi màn hình mỗi khác
uint16_t touchScreenMinimumX = 480, touchScreenMaximumX = 3700, touchScreenMinimumY = 240,touchScreenMaximumY = 3800;

/* Display flushing */
void my_disp_flush (lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    if (LV_COLOR_16_SWAP) {
        size_t len = lv_area_get_size( area );
        lv_draw_sw_rgb565_swap( pixelmap, len );
    }

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( (uint16_t*) pixelmap, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/*========== Read the touch input ==========*/
void my_touch_read (lv_indev_t *indev_drv, lv_indev_data_t * data)
{
    if(ts.touched())
    {
        TS_Point p = ts.getPoint();
        //Some very basic auto calibration so it doesn't go out of range
        if(p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
        if(p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
        if(p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
        if(p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
        //Map this to the pixel position
        data->point.x = map(p.x,touchScreenMinimumX,touchScreenMaximumX,1,screenWidth); /* Touchscreen X calibration */
        data->point.y = map(p.y,touchScreenMinimumY,touchScreenMaximumY,1,screenHeight); /* Touchscreen Y calibration */
        data->state = LV_INDEV_STATE_PR;

         Serial.print( "Touch x " );
         Serial.print( data->point.x );
         Serial.print( " y " );
         Serial.println( data->point.y );
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}


static uint32_t my_tick_get_cb (void) { return millis(); }

// Hàm callback cho timer
static void slow_turn_head(lv_timer_t * t) {

    if (headgo == 1) {

//set_value_task
    //move the first servo slowly to 0 degrees
    if (headangle > 0 && headdirection == 0) {
        headangle-=10;
        board1.setPWM(0, 0, angleToPulse(headangle));

  // Turn LED Red (Red ON, Green/Blue OFF)
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

    } else if(headangle <= 0) {
        headdirection = 1;
    }else{
        //do nothing
    }

     if (headangle < 120 && headdirection == 1) {
        headangle+=10;
        board1.setPWM(0, 0, angleToPulse(headangle));

          // Turn LED Green 
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);

    } else if(headangle >= 180) {
        headdirection = 0;
    }
    else {
        //        board1.setPWM(0, 0, angleToPulse(90));
        // do nothing
        }

    if(neckangle > 0 && neckdirection == 0) {
        neckangle-=10;
        board1.setPWM(2, 0, angleToPulse(neckangle));
    } else if (neckangle <= 0) {
        neckdirection = 1;
    } else {
        // do nothing
    }

     if(neckangle < 110 && neckdirection == 1) {
        neckangle+=10;
        board1.setPWM(2, 0, angleToPulse(neckangle));
    } else if (neckangle >= 110) {
        neckdirection = 0;
    } else {
        // do nothing
    }



            Slidetimer = NULL;

        lv_tick_set_cb(my_tick_get_cb);
    }
    else {
        //this is the headgo checker - stop head while other head movements are happening
        // do nothing
    }
}

static void mouthopen (lv_timer_t * t3) {
    board1.setPWM(1, 0, angleToPulse(45)); // Open the mouth to 45 degrees
    mouthangle = 45;
    lv_delay_ms(500); // Wait for 500 milliseconds
    lv_timer_delete(t3); // Delete the timer after it has executed

}

static void mouthclose (lv_timer_t * t4) {
    board1.setPWM(1, 0, angleToPulse(0)); // Open the mouth to 45 degrees
    mouthangle = 0;
    lv_delay_ms(500); // Wait for 500 milliseconds
    lv_timer_delete(t4); // Delete the timer after it has executed
}

static void talk(lv_timer_t * t5) {
    //open and close mouth 3 times - talking animation
    board1.setPWM(1, 0, angleToPulse(45)); // Open the mouth to 45 degrees
    mouthangle = 45;
    lv_delay_ms(500); // Wait for 500 milliseconds
    board1.setPWM(1, 0, angleToPulse(0)); // Close the mouth
    mouthangle = 0;
    lv_delay_ms(500); // Wait for 500 milliseconds

        board1.setPWM(1, 0, angleToPulse(45)); // Open the mouth to 45 degrees
    mouthangle = 45;
    lv_delay_ms(500); // Wait for 500 milliseconds
    board1.setPWM(1, 0, angleToPulse(0)); // Close the mouth
    mouthangle = 0;
    lv_delay_ms(500); // Wait for 500 milliseconds

        board1.setPWM(1, 0, angleToPulse(45)); // Open the mouth to 45 degrees
    mouthangle = 45;
    lv_delay_ms(500); // Wait for 500 milliseconds
    board1.setPWM(1, 0, angleToPulse(0)); // Close the mouth
    mouthangle = 0;
    lv_delay_ms(500); // Wait for 500 milliseconds

    lv_timer_delete(t5); // Delete the timer after it has executed
}


    //call to look straight ahead
static void headcenter (lv_timer_t * t2) {

headgo = 0; // Stop the head from moving while we reset it to the center position
// set the first servo to 90 degrees
board1.setPWM(0, 0, angleToPulse(90));
headdirection = 0;
headangle = 90;
neckangle = 90;
neckdirection = 0;

  // Turn LED Purple/Magenta (Red and Blue ON)
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, LOW);

lv_delay_ms(3000); // Wait for 1 second to ensure the servo has time to move to the center position

headgo = 1; // Allow the head to start moving again after it has been reset to the center position

lv_timer_delete(t2); // Delete the timer after it has executed

}


// PASTING THE CODE FROM THE LVGL EXAMPLE INTO THE MAIN FILE, MAKE SURE TO ADJUST THE FUNCTION NAMES AND CALLBACKS AS NEEDED

static void btn_event_reset(lv_event_t * e)
{   
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED) {
ESP.restart();

    } else {
        // do nothing  
    }

  
}

//1.4 - complete a quest placeholder and reset - 
/*
-add a number input, check against array of quest numbers
-if correct, goto name entry, if not, try again?
-name entry - keyboard, 10 characters? store in memory and display in scoreboard.
-make a scoreboard with names and quest numbers


*/
static void btn_event_complete(lv_event_t * e)
{   
   lv_tick_set_cb(my_tick_get_cb);

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        const char * txt = lv_label_get_text(label);
//delete the old window
            lv_obj_t* obj = lv_event_get_target_obj(e);
                        //select the bluebox parent container and delete it, which will delete the whole window and all its children
            lv_obj_t* parent_bluebox = lv_obj_get_parent(obj);
            lv_obj_del_async(parent_bluebox);

           /*Create a style and set its background color*/
    static lv_style_t style;
    lv_style_init(&style);
    //lv_style_set_bg_color(&style, lv_color_hex(0x00ff00)); // Set background color to green
    //lv_style_set_bg_color(&style, lv_color_hex(0x8B0000)); // Set background color to dark red
    lv_style_set_bg_color(&style, lv_color_hex(0x00008B));  // Set background color to dark blue
    lv_style_set_bg_opa(&style, LV_OPA_COVER);


    /*Create an object with the new style*/
    lv_obj_t * bluebox = lv_obj_create(lv_screen_active());
    lv_obj_add_style(bluebox, &style, 0);
    lv_obj_set_size(bluebox, 310, 230);
    lv_obj_align(bluebox, LV_ALIGN_CENTER, 0, 0);

        if(strcmp(txt, "Yes") == 0) {

            // create a new textbox with a new dialogue

    /*Create a black label, set its text and align it to the center*/
    lv_obj_t * textbox = lv_label_create(bluebox);
    lv_label_set_text(textbox, "Quest Complete Placeholder");
    lv_obj_set_style_text_color(textbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(textbox, LV_ALIGN_CENTER, 0, -40);
//set text to bold and size to 20
    lv_obj_set_style_text_font(textbox, &lv_font_montserrat_20, LV_PART_MAIN);

            lv_obj_t * okbtn = lv_button_create(bluebox);     /*Add a button the current screen*/
    lv_obj_set_pos(okbtn, 180, 150);                            /*Set its position*/
    lv_obj_set_size(okbtn, 80, 50);                          /*Set its size*/
    lv_obj_add_event_cb(okbtn, btn_event_reset, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * okbtnlabel = lv_label_create(okbtn);          /*Add a label to the button*/
    lv_label_set_text(okbtnlabel, "Okay");                     /*Set the labels text*/
    lv_obj_center(okbtnlabel);

        } else if(strcmp(txt, "No") == 0) {
//1.5 - no quest, no complete - bye! 
    
    lv_tick_set_cb(my_tick_get_cb);
    
/*Create a black label, set its text and align it to the center*/
    lv_obj_t * textbox = lv_label_create(bluebox);
    lv_label_set_text(textbox, "Thank you \nfor stopping by!");
    lv_obj_set_style_text_color(textbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(textbox, LV_ALIGN_CENTER, 0, -40);
//set text to bold and size to 20
    lv_obj_set_style_text_font(textbox, &lv_font_montserrat_20, LV_PART_MAIN);

                lv_obj_t* obj = lv_event_get_target_obj(e);
                        //select the bluebox parent container and delete it, which will delete the whole window and all its children
            lv_obj_t* parent_bluebox = lv_obj_get_parent(obj);
            lv_obj_del_async(parent_bluebox);

    lv_obj_t * okbtn = lv_button_create(bluebox);     /*Add a button the current screen*/
    lv_obj_set_pos(okbtn, 180, 150);                            /*Set its position*/
    lv_obj_set_size(okbtn, 80, 50);                          /*Set its size*/
    lv_obj_add_event_cb(okbtn, btn_event_reset, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * okbtnlabel = lv_label_create(okbtn);          /*Add a label to the button*/
    lv_label_set_text(okbtnlabel, "Okay");                     /*Set the labels text*/
    lv_obj_center(okbtnlabel);

        } else {
            // do nothing  
        }

    }

  
}



// 1.2 - quest assignment and reset

static void btn_event_cb(lv_event_t * e)
{   
    lv_tick_set_cb(my_tick_get_cb);

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        const char * txt = lv_label_get_text(label);


//delete the old window
            lv_obj_t* obj = lv_event_get_target_obj(e);
         //select the bluebox parent container and delete it, which will delete the whole window and all its children
            lv_obj_t* parent_bluebox = lv_obj_get_parent(obj);
            lv_obj_del_async(parent_bluebox);

           /*Create a style and set its background color*/
    static lv_style_t style;
    lv_style_init(&style);
    //lv_style_set_bg_color(&style, lv_color_hex(0x00ff00)); // Set background color to green
    //lv_style_set_bg_color(&style, lv_color_hex(0x8B0000)); // Set background color to dark red
    lv_style_set_bg_color(&style, lv_color_hex(0x00008B));  // Set background color to dark blue
    lv_style_set_bg_opa(&style, LV_OPA_COVER);


    /*Create an object with the new style*/
    lv_obj_t * bluebox = lv_obj_create(lv_screen_active());
    lv_obj_add_style(bluebox, &style, 0);
    lv_obj_set_size(bluebox, 310, 230);
    lv_obj_align(bluebox, LV_ALIGN_CENTER, 0, 0);

        if(strcmp(txt, "Yes") == 0) {
            
            questcounter++; // Increment the quest counter when a quest is completed
            

int quest_num = random(1, 15); // Generate a random number between 1 and 14
const char* quest_text;
            // create a new textbox with a new dialogue
if(quest_num == 1) {
    quest_text = "Beautify the main camp \narea in some way";
} else if(quest_num == 2) {
    quest_text = "Beautify your camp in \nsome way";
} else if(quest_num == 3) {
    quest_text = "Design a symbol and decorate \nyour gear with it";
} else if(quest_num == 4) {
    quest_text = "Help an adventurer get a missing \npiece of camping gear that they need";
} else if(quest_num == 5) {
    quest_text = "Never split the party!\nAccompany someone to the restroom";
} else if(quest_num == 6) {
    quest_text = "Refill another travelers \ncanteen";
} else if(quest_num == 7) {
    quest_text = "Provide/Arrange transport \nto a traveler in need";
} else if(quest_num == 8) {
    quest_text = "Exchange quest information \nwith a fellow adventurer";
} else if(quest_num == 9) {
    quest_text = "Carry a vital message for \nan adventurer";
} else if(quest_num == 10) {
    quest_text = "Provide assistance to a \ntraveler in need";
} else if(quest_num == 11) {
    quest_text = "Fetch an important \nitem for someone";
} else if(quest_num == 12) {
    quest_text = "Provide freelance \ncourier services";
} else if(quest_num == 13) {
    quest_text = "Provide freelance \nmessenger services";
} else {
    quest_text = "Go on an \nadventure!";
}


    /*Create a black label, set its text and align it to the center*/
    lv_obj_t * textbox = lv_label_create(bluebox);
    lv_label_set_text(textbox, quest_text);
    lv_obj_set_style_text_color(textbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(textbox, LV_ALIGN_CENTER, 0, -40);


    /*Create a black label, set its text and align it to the center*/
    lv_obj_t * textbox2 = lv_label_create(bluebox);
    lv_label_set_text(textbox2, "Your quest Number is: ");
    lv_obj_set_style_text_color(textbox2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(textbox2, LV_ALIGN_CENTER, 0, 20);
//set text to bold and size to 20
    lv_obj_set_style_text_font(textbox2, &lv_font_montserrat_16, LV_PART_MAIN);

        lv_obj_t * textbox3 = lv_label_create(bluebox);
    char questcounterChar[10];
    sprintf(questcounterChar, "%d", questcounter); // Convert the quest counter to  a string

    lv_label_set_text(textbox3, questcounterChar);
    lv_obj_set_style_text_color(textbox3, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(textbox3, LV_ALIGN_CENTER, 0, 40);
//set text to bold and size to 20
    lv_obj_set_style_text_font(textbox3, &lv_font_montserrat_16, LV_PART_MAIN);


    lv_obj_t * okbtn = lv_button_create(bluebox);     /*Add a button the current screen*/
    lv_obj_set_pos(okbtn, 180, 150);                            /*Set its position*/
    lv_obj_set_size(okbtn, 80, 50);                          /*Set its size*/
    lv_obj_add_event_cb(okbtn, btn_event_reset, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * okbtnlabel = lv_label_create(okbtn);          /*Add a label to the button*/
    lv_label_set_text(okbtnlabel, "Okay");                     /*Set the labels text*/
    lv_obj_center(okbtnlabel);

    lv_timer_create(headcenter, 1000, NULL); // Call the headcenter function after 1 second to reset the servo position
      
    lv_timer_create(mouthopen, 1500, NULL); // Call the talk function after 2 seconds to start the talking animation
 
} else if(strcmp(txt, "No") == 0) {

    //1.3 - complete a quest?
lv_tick_set_cb(my_tick_get_cb);

   /*Create a black label, set its text and align it to the center*/
    lv_obj_t * textbox = lv_label_create(bluebox);
    lv_label_set_text(textbox, "Would you like to \ncomplete a quest?");
    lv_obj_set_style_text_color(textbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(textbox, LV_ALIGN_CENTER, 0, -40);
//set text to bold and size to 20
    lv_obj_set_style_text_font(textbox, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t * yesbtn = lv_button_create(bluebox);     /*Add a button the current screen*/
    lv_obj_set_pos(yesbtn, 10, 130);                            /*Set its position*/
    lv_obj_set_size(yesbtn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(yesbtn, btn_event_complete, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * yesbtnlabel = lv_label_create(yesbtn);          /*Add a label to the button*/
    lv_label_set_text(yesbtnlabel, "Yes");                     /*Set the labels text*/
    lv_obj_center(yesbtnlabel);

    lv_obj_t * nobtn = lv_button_create(bluebox);     /*Add a button the current screen*/
    lv_obj_set_pos(nobtn, 150, 130);                            /*Set its position*/
    lv_obj_set_size(nobtn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(nobtn, btn_event_complete, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * nobtnlabel = lv_label_create(nobtn);          /*Add a label to the button*/
    lv_label_set_text(nobtnlabel, "No");                     /*Set the labels text*/
    lv_obj_center(nobtnlabel);
            


        } else {
            // do nothing  
        }

    }

  
}





// 1.1 - first message
void lv_quest_get_started_2(void)
{
    
    //lv_async_call(headcenter, NULL); // Call the headcenter function to set the servo positions at the start of the program
    
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x87ceeb), LV_PART_MAIN);    //Set background color to sky blue

          /*Create a style and set its background color*/
    static lv_style_t style;
    lv_style_init(&style);
    //lv_style_set_bg_color(&style, lv_color_hex(0x00ff00)); // Set background color to green
    //lv_style_set_bg_color(&style, lv_color_hex(0x8B0000)); // Set background color to dark red
    lv_style_set_bg_color(&style, lv_color_hex(0x00008B));  // Set background color to dark blue
    lv_style_set_bg_opa(&style, LV_OPA_COVER);


    /*Create an object with the new style*/
    lv_obj_t * bluebox = lv_obj_create(lv_screen_active());
    lv_obj_add_style(bluebox, &style, 0);
    lv_obj_set_size(bluebox, 310, 230);
    lv_obj_align(bluebox, LV_ALIGN_CENTER, 0, 0);

    /*Create a black label, set its text and align it to the center*/
    lv_obj_t * textbox = lv_label_create(bluebox);
    lv_label_set_text(textbox, "WELCOME!\nWould you like to \nreceive a quest?");
    lv_obj_set_style_text_color(textbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(textbox, LV_ALIGN_CENTER, 0, -40);
//set text to bold and size to 20
    lv_obj_set_style_text_font(textbox, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t * yesbtn = lv_button_create(bluebox);     /*Add a button the current screen*/
    lv_obj_set_pos(yesbtn, 10, 130);                            /*Set its position*/
    lv_obj_set_size(yesbtn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(yesbtn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * yesbtnlabel = lv_label_create(yesbtn);          /*Add a label to the button*/
    lv_label_set_text(yesbtnlabel, "Yes");                     /*Set the labels text*/
    lv_obj_center(yesbtnlabel);

    lv_obj_t * nobtn = lv_button_create(bluebox);     /*Add a button the current screen*/
    lv_obj_set_pos(nobtn, 150, 130);                            /*Set its position*/
    lv_obj_set_size(nobtn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(nobtn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * nobtnlabel = lv_label_create(nobtn);          /*Add a label to the button*/
    lv_label_set_text(nobtnlabel, "No");                     /*Set the labels text*/
    lv_obj_center(nobtnlabel);

    Slidetimer = lv_timer_create(slow_turn_head, 500, NULL);
    lv_tick_set_cb(my_tick_get_cb);
    lv_timer_create(mouthclose, 2000, NULL); // Call the talk function after 2 seconds to start the talking animation

}


// END OF LVGL EXAMPLE CODE



void setup (){

    Serial.begin( 115200 );

    //CUSTOM PIN LOCATIONS FOR I2C, CHANGE TO YOUR PINS
    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin();
    Serial.println("I2C initialized with custom pins!");

    lv_init();

    //initialize I2C for servo control, custom pin locations
    

    //servo test code
      Serial.println("16 channel Servo test!");
  board1.begin();
  board1.setOscillatorFrequency(23000000);
  board1.setPWMFreq(50); // Servos operate at ~50 Hz


     //Initialise the touchscreen
    tsSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); /* Start second SPI bus for touchscreen */
    ts.begin(tsSpi);      /* Touchscreen init */
    ts.setRotation(3);   /* Inverted landscape orientation to match screen */

    tft.begin();         /* TFT init */
    tft.setRotation(3); /* Landscape orientation, flipped */

    // Initialize LVGL display
    static lv_disp_t* disp;
    disp = lv_display_create( screenWidth, screenHeight );
    lv_display_set_buffers( disp, buf, NULL, SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL );
    lv_display_set_flush_cb( disp, my_disp_flush );

        //Initialize the Rotary Encoder input device. For LVGL version 9+ only
    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, my_touch_read);

    //setup onboard LED of timing tests
      // Set all LED pins to OUTPUT
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  // Start with the LED off (HIGH)
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);


    lv_quest_get_started_2(); // Example 1: Basic usage

        Slidetimer = lv_timer_create(slow_turn_head, 500, NULL);
    //lv_timer_set_repeat_count(Slidetimer, -1); // Repeat indefinitely
        
     Serial.println( "Setup done" );
}

void loop () {

    //black: 0x000000, white: 0xFFFFFF, red: 0xFF0000, green: 0x00FF00, blue: 0x0000FF
// light yellow: 0xFFFF00, light cyan: 0x00FFFF, light magenta: 0xFF00FF
// light gray: 0xD3D3D3, dark gray: 0xA9A9A9, white: 0xFFFFFF, black: 0x000000
// light pink: 0xFFB6C1, light salmon: 0xFFA07A, light coral: 0xF08080
// beige: 0xF5F5DC, light goldenrod: 0xFAFAD2, light sky blue: 0x87CEFA

/*
// Set all servos to 0 degrees
  for(int i=0; i<8; i++) {
    board1.setPWM(i, 0, angleToPulse(0));
  }

 /*
  //set the first servo to 90 degrees
  board1.setPWM(1, 0, angleToPulse(90));


    //set the first servo to 90 degrees
  board1.setPWM(0, 0, angleToPulse(90));


  //set the first servo to 45 degrees
    board1.setPWM(1, 0, angleToPulse(45));

*/
    board1.writeMicroseconds(0, angleToPulse(headangle));
    board1.writeMicroseconds(1, angleToPulse(mouthangle));
    board1.writeMicroseconds(2, angleToPulse(neckangle));
  
    // LVGL task handler - this is essential!
    lv_timer_handler();
    //Serial.println( "Looping..." ); //checking how frequent loop runs

    lv_delay_ms(5); // Add a small delay to prevent overwhelming the CPU and to allow other tasks to run

}

