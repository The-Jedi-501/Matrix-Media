
// HUB75E pinout --- if looking at the pin connctor holes with the clip on the right --- See notes in OneNote for more details
// R1 | G1
// B1 | GND
// R2 | G2
// B2 | E
//  A | B
//  C | D
// CLK| LAT
// OE | GND



/*
Chris Notes:

Its been a while since ive done any sort of ardunio code so there will be alot of comments to kinda catch mysefl back up to speed 
Along side this, this code was intially an example code for the matrix and i build off of this so im going thru to make it more organized 
Goal: have two version of the code one for Notes heavy commented ie the intiall run and then later on ill make a seocnd one where the comments are more strict but still helpful 


Installs: the library headers themselves so they work propperly 
          - for this code i use ESP32 so make sure you have that libary also downloaded 


Format and layout of the code add the newer parts later on so MATRIX >>> LCD >>> tershiary part unless it messes with code 


*/



// --- Pin Configuration for my Matix ---
const int R1 = 32;
const int G1 = 33;   //was initally 15
const int BL1 = 15;  //was intially 33
const int R2 = 25;
const int G2 = 26;  //was 2
const int BL2 = 2;  //was 26
const int CH_A = 27;
const int CH_B = 4;
const int CH_C = 5;
const int CH_D = 16;
const int CH_E = -1;
const int CLK = 12;
const int LAT = 17;
const int OE = 13;
/*Noticed some weird issues with color as in Green and Blue are definitly flipped and id id the wiring so idk 
 Pins here coorelate to the GPIO pins of which whilst GPIO is General Purpose Input Output and since they are outputs as as we just need to send signals to the pannel itself never read anything back 
 also before looking at pins to just plug in consule a wiring diagram casue it doesnt tell whcih is does what 

 attempted using "#defines" use "const int" instead 
*/

// --- PIN CONFIG FOR ROTARY ENCODER (_R_E == _Rotary_Encoder)--- Purpose of encoder was to be able to switch to multiple screens --- REMOVED PART
  //const int CLK_R_E = 35; //GPIO 35 remember thats how we refercne also pins also since GPIO 36, 39, 35, 34 ARE STRICTLY INPUT ONLY OF WHICH THEY ARE 
  //const int DT_R_E = 34;
// --- PIN CONFIG FOR PUSH BUTTONS (since Rotatry Encoders caused all sorts of issues design wise trying alternaiva)
  const int PB_NO_Forward = 35; // Push Button Normally Open (Starts 0) and Forward in event i want to have a seperate button for going forward vs back in terms of screens --- NOTE IN ORDER FOR THIS TO WORK MUST USE EXTERNAL 10K OHM RESISTOR TO PULL GND 


// --- Headers and important key features for the matrix (Width x Height) ---
  #include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>  //library for matrix itslef

// Headers for LCD
  #include <Wire.h>              //This is essenlly how esp knows im using the clock and data pin
  #include <Adafruit_GFX.h>      //This library features some just neat things inherintly such as wrap around text but i will be setting that to "false"
  #include <Adafruit_SSD1306.h>  //For the lcd driver itself

  #include <math.h>               //need for sin and cos added after i used distance formuala could go back adn make sqrt adn pow 



// MATRIX --- Configure for your panel(s) as appropriate!
  #define PANEL_WIDTH 64
  #define PANEL_HEIGHT 32  // Panel height of 64 will required PIN_E to be defined. --- NEEEDED TO CHANGE TO MAKE WORK WITH MINE
  #define PANELS_NUMBER 1  // Number of chained panels, if just a single panel, obviously set to 1
  //#define PIN_E 32 //if i were to have mulitple dispalys then uncomment this, add it to pins later on and then in setup hte mxconfis uncomment the E line thats it nothing else

// MATRIX --- Reason why this is here and look weird is: just accountign for total number of leds in use hence "* PANEL_NUMBER" its essnally the latoyt itself, think if i got another 32x64 how do we code wise tell difference if ther staked ontop or sitting next??? this solves it so i could have it be numbers or this math
  #define PANE_WIDTH PANEL_WIDTH *PANELS_NUMBER
  #define PANE_HEIGHT PANEL_HEIGHT

// LCD --- Setting up the size for mine
  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 64

  #define SERIAL_SIZE_RX  7144 //Purpose is to set gloabl variable in attempt to save the bug of puase play icon showin in cd mode - note should have a value that teh expected (32x64 x3 = 6144 + extra = so lets say 7144 for now idk man)

// MATRIX --- placeholder for the matrix object
  MatrixPanel_I2S_DMA *dma_display = nullptr;  //This creates a placehodler varible name nothin now but later it hold the actula matrix dispaly once setup cratse it

// MATRIX --- Prepping final essential varibales before the loop --- true as were starting her e
  String incomingTag = "";  // this is where im building the "ART" as they arrive so A R T
  bool readingTag = true;   // of which needs to be truee as intially were lookign for that first song piece think im in reading a tag vs reading pixels

  uint8_t imageBuffer[6144];  // unsigned 8 bit integer is - 32 x 32 = 1024 times 3 bytes value for R G B so 1024 pixels x 3 bytes each = 3072 total bytes - testing right side also so 63x32x3 is 6144
  int bytesReceived = 0;      // these are both counters to track hwo many bytes collected so far toward teh current image
  int expectedBytes = 0;      // tracking how mnay im suppose to collect in total before its complete where this one gets the 3072 the moment we see ART tag???



// LCD --- Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);  //if i wanted anther lcd call it display_2

// LCD --- this section is used for if the text is too big and needs to scroll essenlly
  int16_t boundX, boundY;  //Both "int"
  uint16_t title_w, title_h;
  uint16_t artist_w, artist_h;

  int xPos_Title = 34;
  int xPos_Artist = 46;

  // LCD --- TIME SECTION --- its used for the millis loop at the very end so rather than a delay that fully stops code millis allows us to only manipulate a chunk of code over time
  unsigned long lastScrollTime = 0;
  const int scrollInterval = 25;  // adjust this number to control scroll speed — smaller = faster

  unsigned long HoldStart_Title = 0;  // new variable, when did we last reset this string
  unsigned long HoldStart_Artist = 0;

  const unsigned long holdDuration = 1000;  // 1 second pause before scrolling starts

  bool IsPausing_Title = false;
  bool IsPausing_Artist = false;

// LCD --- false since this is a later step in the overall code
  String incomingText = "";
  bool readingPixels = false;  // dont want to read the pixels for letters for text right away
  bool readingText = false;

  String currentTitle = "";
  String currentArtist = "";

  bool lcdReady = false;

  float avgCharWidth_Title;  //Varibale is later used in the void loop to constantly update how long the string is to make sure it get fully played
  float avgCharWidth_Artist;

  String incomingPosition = "";
  bool reading_POS = false;

  String current_POS = "";
  String current_END_TIME = "";

  String current_Status = "";

  int posMinutes = 0;
  int posSeconds = 0;
  int endMinutes = 0;
  int endSeconds = 0;

  int barFill = 0;
  int Status_Num = 0;

  int Status_Color_Dynamic_Test_R = 0;
  int Status_Color_Dynamic_Test_G = 0;
  int Status_Color_Dynamic_Test_B = 0;

// LCD --- Stuff from website to get custom designs
  static const unsigned char PROGMEM image_music_bits[] = { 0x00, 0x7c, 0x0f, 0x84, 0x08, 0x04, 0x08, 0x7c, 0x0f, 0xc4, 0x08, 0x04, 0x08, 0x04, 0x08, 0x04, 0x08, 0x04, 0x08, 0x04, 0x08, 0x38, 0x70, 0x44, 0x88, 0x44, 0x88, 0x38, 0x70, 0x00 };

  static const unsigned char PROGMEM image_usb_cable_connected_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0xe0, 0x04, 0xc0, 0x08, 0x04, 0xc8, 0x06, 0xff, 0xff, 0xc2, 0x06, 0x02, 0x04, 0x01, 0x30, 0x00, 0xf8, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00 };

// LCD --- MATH CLASS --- Purpsoe is to essnlly be able to caluclate before hand or for now declare some essentlls
  int X_Center = 16;  // using 64/2=32 sicne im usign the full matrix rn but 32 i but 16 is the left center but sicne its indexed at 0 therfore minus 1 ???
  int Y_Center = 16;

  int Hole_Ring_Radius = 2;
  int Inner_Ring_Radius = 4;
  int Art_Radius = 13;  // Radius is Distance/2 or half
  int Outer_Ring_Radius = 15; //The outer white ring essenlly was 16 adn art 15


// ROTARY ENCODER VARIABLES NEEDED -  note this section and all encoder stuff will be based of of ex code i built
  int Current_Screen_Value = 1; //before i had a bool now the idea is hey the math says were on screen one so if else our way to find the correct screen 
  int Clock_State; 
  int Last_Clock_State;
  int Counter_Encoder = 0; //This will be used to track the current value of the encoder - Where 0 is us saying hey were starting at that value 


// PUSH BUTTON VARIABLES - as well as the time aspect involved 
  int PB_NO_Forward_rawReading;            //Actual reading basic i know right 
  int PB_NO_Forward_lastRaw = HIGH;        // last raw reading seen (starts HIGH since idle = pulled up)
  int PB_NO_Forward_debounced = HIGH;      // the "trusted" value
  int PB_NO_Forward_lastDebounced = HIGH;  // previous trusted value, for edge detection

  unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 30; // millis the main idea here is to 

  int Counter_PB_NO_Screen_Value = 1; // Main idea is i can say hey let 1 be the main album art screeen and then from here im jsut increaming and later decremenating when i have anohter button 
  //int Counter_PB_NO = 0; //Not refereing to them as FORWARD as i need this to work with teh potentail future second button that allows me to go in reveres 

  int Max_Screens_Made = 2; // This is going to be a way i just check the max screen i have and then from here ill build maths --- also not sure whihc variable group it shoudl go with 

//OG VERSION USED 
  //bool CD_MODE = true; //Main idea let future component jsut flip this bool exptession --- REMOVE/UNCOMMNET FOR NOW JSUT TO GET IT WORK ING 


// MATH EXPLAINED --- Called once per frame, BEFORE the pixel loop starts - faster smoother without speeding it up, you need to cut both numbers, together, by the same amount. So instead of 1000ms + 10 degrees, try something like 500ms + 5 degrees
  float spinAngle = 0.0;         // persists across frames — this is your "currentAngle"
  const float spinIncrement = 0.0436; //0.0872; //0.1745; // ~10 degrees in radians, tune this once you know your FPS

//Varibales for the spinning 
  unsigned long lastSpinTime = 0;
  int spinInterval = 250; //500; //1000; // 1 second 
        
// Attempting to fix possible bytes being lost ---
  unsigned long lastByteTime = 0;
  const unsigned long serialTimeout = 5000;  //this will be my if i dont new bytes in 5seconds

// USED ONLY IN TERMINAL --- This mini section is used to just track the animation and track its frame rate nothing special we wwould only see this in terminal
  uint16_t time_counter = 0, cycles = 0, fps = 0;
  unsigned long fps_timer;

//FUTURE ME HEY UR NOT USING A SINGLE STRUCT???? DO THAT MESS AROUD CAUSE VARIBALES ARE A MESS RN 



void setup() {

  Serial.begin(115200);  //value relates to baud rate which is just how fast we can transfer info this value was the preset for ex code, NOTE the value we see on the esp32 ie 921600 is a differnt thing relating to esptool speed vs teh value here is what value her eis for communication

  Serial.setRxBufferSize(SERIAL_SIZE_RX); // SUPPOSED FIX FOR PAUSE PLAY BUG IN CD MODE - this being a buffer size issue and solving a related issue for losing and misplacing due to byte loss - 
  //Explainb buffer: how much how much room exists to hold bytes that have arrived but your code hasn't gotten around to reading yet. It doesn't know or care about "expected" anything; it's just a holding tank.
  //Explaiin byte loss: We are rasining the roof of the container for bytes as "bytes get lost: your code takes too long to come back and read them. Here's the actual timeline. Bytes arrive on the wire continuously, at a rate set by your baud rate — they don't wait for your code to be ready, they just keep coming. Meanwhile, your ESP32"
  //cont.d by us rasiing the ceiling were preventiong the overflow so with all this extra room i dont need to worry about losing anythign adn there fore IN CD MODE I SHOULDNT SEE ANY PAUSE PLAY WEIRD GHOST PROJECTION BS STUFF ANYMORE
  //"make the gap between drops and buffer capacity big enough that your actual, real-world busy-periods never come close to filling it"
  //UPDATE COULDNT GET THIS WORKING AT ALL SO THE EXTRA HEY UR PAUSING OR APLYING ART MADE IN PY WHEN IN CDMODE WILL ALSWYS BE AN ISSUE UTS FINE 

  Serial.println(F("*****************************************************"));
  Serial.println(F("*        ESP32-HUB75-Matrix Media Project           *"));
  Serial.println(F("*****************************************************"));

  /*
    THIS CODE CHUNK IF FOR MULTI MATRIX --- This large commented section aslo is just general docuamtention so applies to single matrix alteranative approahc form the _pins we see below 

    The configuration for MatrixPanel_I2S_DMA object is held in HUB75_I2S_CFG structure,
    pls refer to the lib header file for full details.
    All options has it's predefined default values. So we can create a new structure and redefine only the options we need 

    // those are the defaults
    mxconfig.mx_width = 64;                   // physical width of a single matrix panel module (in pixels, usually it is always 64 ;) )
    mxconfig.mx_height = 32;                  // physical height of a single matrix panel module (in pixels, usually almost always it is either 32 or 64)
    mxconfig.chain_length = 1;                // number of chained panels regardless of the topology, default 1 - a single matrix module
    mxconfig.gpio.r1 = R1;                    // pin mappings
    mxconfig.gpio.g1 = G1;
    mxconfig.gpio.b1 = B1;                    // etc
    mxconfig.driver = HUB75_I2S_CFG::SHIFT;   // shift reg driver, default is plain shift register
    mxconfig.double_buff = false;             // use double buffer (twice amount of RAM required)
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;// I2S clock speed, better leave as-is unless you want to experiment
  */

  /*
    For example we have two 64x64 panels chained, so we need to customize our setup like this

  */
  // HUB75_I2S_CFG mxconfig;
  // mxconfig.mx_height = PANEL_HEIGHT;      // we have 64 pix heigh panels
  // mxconfig.chain_length = PANELS_NUMBER;  // we have 2 panels chained
  // mxconfig.gpio.e = PIN_E;                // we MUST assign pin e to some free pin on a board to drive 64 pix height panels with 1/32 scan
  //mxconfig.driver = HUB75_I2S_CFG::FM6126A;     // in case that we use panels based on FM6126A chip, we can change that



  // --- How we are actully able to use the matrix via setup ie were using these pins its single(top) or mulitmatrix (bottom commented version) ---
  //This is the code setup for a single matrix
  HUB75_I2S_CFG::i2s_pins _pins = { R1, G1, BL1, R2, G2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK };  //Bundling all the pins were using for the matrix here in the order library expects
  //this next part is calling all teh information on the matrix itslef about my setup
  HUB75_I2S_CFG mxconfig(
    PANEL_WIDTH,
    PANEL_HEIGHT,
    PANELS_NUMBER,
    _pins);

  mxconfig.clkphase = false;  //From the git itslef "If you are facing issues with pixels being 'off' by 1 px to the co-ordinate requested, or experiencing ghosting, then it could be due to the 'clock phase' setting."

  // Below is that same setup but for mulit matrix
  // mxconfig.gpio.e = PIN_E;  -- remove/comment out entirely, not needed for a 32-tall panel

  /*
    //Another way of creating config structure
    //Custom pin mapping for all pins
    HUB75_I2S_CFG::i2s_pins _pins={R1, G1, BL1, R2, G2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
    HUB75_I2S_CFG mxconfig(
                            64,   // width
                            64,   // height
                             4,   // chain length
                         _pins,   // pin mapping
      HUB75_I2S_CFG::FM6126A      // driver chip is found on the back of matrix its all those ic chips they all have same value mine 
    );

  */


  // MATRIX --- Just what we see on startup  ---
  // OK, now we can create our matrix object
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);  //Only interesting part here bc we needed to make it a ptr varible as if it was a value then it could contain garbage due to leftover data in memory location very bad so what happens here is so now we give it a value in terms of all the core parts ie width, height number and pins

  // Brightness
  dma_display->setBrightness8(255);  // range is 0-255, 0 - 0%, 255 - 100%
  dma_display->setLatBlanking(2);    //From the Git where they say "If you are facing issues with image ghosting when pixels has clones with horizontal offset ..." thees alot more info - and for me was needed and was a life saver and took me way too long to realize i needed 


  // Allocate memory and start DMA display - safety check ngl seen this so much in lectures not supprised
  if (not dma_display->begin())
    Serial.println("****** !KABOOM! I2S memory allocation failed ***********");

  // well, hope we are OK, let's draw some colors first :)
  Serial.println("Fill screen: RED");
  dma_display->fillScreenRGB888(255, 0, 0);
  delay(100);

  Serial.println("Fill screen: GREEN");
  dma_display->fillScreenRGB888(0, 255, 0);
  delay(100);

  Serial.println("Fill screen: BLUE");
  dma_display->fillScreenRGB888(0, 0, 255);
  delay(100);

  Serial.println("Fill screen: Neutral White");
  dma_display->fillScreenRGB888(64, 64, 64);
  delay(100);

  Serial.println("Fill screen: black");
  dma_display->fillScreenRGB888(0, 0, 0);
  delay(100);



  // LCD --- Initialize I2C on custom or default pins (21 for SDA, 22 for SCL)
  Wire.begin(21, 22);

  // LCD --- Saftey Check --- Address 0x3C is common; change to 0x3D if 0x3C doesn't work
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed — continuing without LCD"));
    lcdReady = false;
  } else {
    lcdReady = true;
  }

  // LCD --- Just running thru the basic to make sure it works
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);  //on
  display.setCursor(0, 0);      //where we starting ie top left
  display.println(F("Matrix Media Prep"));
  display.display();

  delay(250);

  display.setTextWrap(false);  //This stop the text from wrapping around ot other rows essenllyu this is naturally TRUE as this is apart of the GFX library

  Serial.println("Testing via terminal");


  // ROTARY ENCODER SETUP - REMOVED PART
  // pinMode(CLK_R_E, INPUT);
  // pinMode (DT_R_E, INPUT);

  //Push Button Norm Open SETUP 
  pinMode (PB_NO_Forward, INPUT);


}  // Closeing } for "Void Setup"



//Function that we use in the "void loop" purpose is the seperate the text from artist and song via the "|"
void processTitleArtist( String data ) {
  int separatorIndex = data.indexOf('|');  // find WHERE the | character sits in the string

  currentTitle = data.substring(0, separatorIndex);    // everything before the |
  currentArtist = data.substring(separatorIndex + 1);  // everything after the |


  xPos_Title = 34;
  IsPausing_Title = true;
  HoldStart_Title = millis();

  xPos_Artist = 46;
  IsPausing_Artist = true;
  HoldStart_Artist = millis();
}



void time_converion( int totalSeconds, int &outMinutes, int &outSeconds ) {
  outMinutes = totalSeconds / 60;
  outSeconds = totalSeconds % 60;
}



//Function that we use in the "void loop" purpose is the seperate the text from artist and song via the "|"
void process_Time_For_Piece( String data ) {
  int separatorIndex = data.indexOf('|');                 // find WHERE the | character sits in the string
  int seperatorIndex_Status = data.indexOf('~');
                                                          //posMinutes = data.length();
  current_POS = data.substring(0, separatorIndex);        // everything before the |
  current_END_TIME = data.substring(separatorIndex + 1);  // everything after the |

  current_Status =  data.substring(seperatorIndex_Status + 1); //fundamentally its this but how to determine if im reasin left or right | oh just make a diff | so use ~ for ex 

  int Current_POS_Num = current_POS.toInt();
  int Current_END_TIME_Num = current_END_TIME.toInt();

  Status_Num = current_Status.toInt();
  if (Status_Num == 1){
    Status_Color_Dynamic_Test_R = 0;
    Status_Color_Dynamic_Test_G = 255;
    Status_Color_Dynamic_Test_B = 0;
  }
  else if (Status_Num == 0) {
    Status_Color_Dynamic_Test_R = 255;
    Status_Color_Dynamic_Test_G = 0;
    Status_Color_Dynamic_Test_B = 0;
  }

  time_converion(Current_POS_Num, posMinutes, posSeconds);
  time_converion(Current_END_TIME_Num, endMinutes, endSeconds);

  int totalDurationSeconds = (endMinutes * 60) + endSeconds;
  int currentPositionSeconds = (posMinutes * 60) + posSeconds;

  float pixelsPerSecond = 116.0 / totalDurationSeconds;

  if (totalDurationSeconds > 0) {
    barFill = (int)(pixelsPerSecond * currentPositionSeconds);
  } 

  else {
    barFill = 0;
  }

}



void drawScreen_1( void ) {  //Numbers key: (X start, Y from bottom start, Width ,Hight I draw, ON==1 ) See detialed up top --- USE LOPAKA WEBSITE TO GET A VIEW ON THE STUFF BEFORE HAND
                           //Where Y is measure top left orignin

  //rect 1 - Outer Box
  display.drawRect(4, 53, 120, 7, 1);
  // rect 2 - Inner Part that tracks progress
  display.fillRect(6, 55, barFill, 3, 1);  //track is suppose to be emputy and vriabale starts at 0 - barfill is function vs track was real
  // music - Music Icon In the Middle - Dont do anything Yet
  display.drawBitmap(57, 0, image_music_bits, 14, 15, 1);
  // usb_cable_connected - USB upper left doesnt do anythign yet
  display.drawBitmap(3, 0, image_usb_cable_connected_bits, 16, 16, 1);
  // string 6 - Song: line
  display.setTextColor(1);
  display.setCursor(4, 18);  //Note the perfect placement for real song will be at x = 34
  display.print("Song:");
  // string 7 - Artist:
  display.setCursor(4, 28);  //Note the perfect placement for real song will be at x = 46
  display.print("Artist:");
  // string 9 - Later i want this to be the what screen ie vinyl or time so set a varuibak l later on
  display.setCursor(77, 3);
  display.print("Screen: ");
  // string 9 - Chnage this to a variubale later on so its goes trhu 1 2 3 fro media vinyl or time
  display.setCursor(119, 3);
  display.print(Counter_PB_NO_Screen_Value); //Current_Screen_Value USED TO TRACK SCREEN was just "1" should now actully show the value for screen 
  // line 10 - the line that seperate teh yellow from blue ie the change in color ~16 p
  display.drawLine(0, 15, 127, 15, 1);
  // string 10 - this will be changed to varibales in main code to be the current and end but ex rn

  String posSecStr = (posSeconds < 10) ? ("0" + String(posSeconds)) : String(posSeconds);
  String endSecStr = (endSeconds < 10) ? ("0" + String(endSeconds)) : String(endSeconds);

  String timeDisplay = String(posMinutes) + ":" + posSecStr + " / " + String(endMinutes) + ":" + endSecStr;

  display.setCursor(4, 43);
  display.print(timeDisplay);  //NEED TO BE VARIBALES SEE THE WEBSITE TO GET EXACT SPOTS FOR THIS STUFF
}



void Text_Logic(String current_Txt, float &Avg_Char_Wid_Txt, uint16_t &Txt_w,
                int xPos_Txt, int X_Value, int Y_Value,
                int16_t &Bound_X, int16_t &Bound_Y, uint16_t &Txt_H) {

  if (current_Txt.length() > 0) {
    Avg_Char_Wid_Txt = Txt_w / strlen(current_Txt.c_str());
  } 
  else {
    Avg_Char_Wid_Txt = 1;  // safe placeholder, doesn't matter since there's no text to draw yet anyway
  }

  display.setCursor(xPos_Txt, Y_Value);
  display.getTextBounds(current_Txt, X_Value, Y_Value, &Bound_X, &Bound_Y, &Txt_w, &Txt_H);

  if (Txt_w > (128 - X_Value)) {  //Main logic is if the title string name is too big perfrom the marqeuee esq sign else jsut print it
    for (int i = 0; i < current_Txt.length(); i++) {
      int Char_X_Txt = xPos_Txt + (i * Avg_Char_Wid_Txt);  // where this specific character would land

      if (Char_X_Txt >= X_Value && Char_X_Txt <= 128) {  // is it inside the visible window?
        display.setCursor(Char_X_Txt, Y_Value);
        display.print(current_Txt[i]);  // draw JUST this one character
      }
    }
  } 
  else {
    display.setCursor(X_Value, Y_Value);
    display.print(current_Txt);  // fits fine, just draw it plainly, no scroll
  }
}



void Slider_Gaurd(bool &Pause_Txt, unsigned long &Hold_State_Txt, unsigned long Hold_Duration, int &x_Pos_TA, String Current_Piece, float Avg_Char_Width_Part, int Y_Edge) {

  if (Pause_Txt) {
    if (millis() - Hold_State_Txt >= Hold_Duration) {
      Pause_Txt = false;  // hold's over, start sliding
    }
    // else: do nothing, stay put
  } 
  else {
    x_Pos_TA -= 1;  // sliding

    if (x_Pos_TA + (Current_Piece.length() * Avg_Char_Width_Part) <= Y_Edge) {
      x_Pos_TA = 128;  // string has fully cleared the visible window — wrap to off-screen right, keep sliding, NO pause here
    } 
    else if (x_Pos_TA == Y_Edge) {
      Pause_Txt = true;  // arrived back at 34 — pause here
      Hold_State_Txt = millis();
    }
  }
}

void Draw_Art_Framework( ) {

  int i = 0;

  // MATH --- Depending on status ie playing or paused rotate - to get a smoother speed overall shrik spinInterval and spinIncrement proportaon as its Inc/Int
  if( Status_Num == 1 ){ 
    spinAngle += spinIncrement;
    if (spinAngle >= 2 * PI) {
      spinAngle -= 2 * PI;   // wrap, avoids unbounded growth
    }
  }

  float cosA = cos(-spinAngle);  // NEGATIVE — backward mapping, screen -> source, for postivie or negateve remeber the 4 Quadrants postive is All Students Take Calc 
  float sinA = sin(-spinAngle);

  for (int y = 0; y < 32; y++) {     //Note the same nested loop from python
    for (int x = 0; x < 64; x++) {   //once again now 64 was 32 due to cheagnign stuff

    if ( Counter_PB_NO_Screen_Value == 2 ){ //Logic for ifs here are essnlly make the art then when i make the white ring it cant overlap the art so its filling in the rest and the important ring thinkness is the difference form the outer and art radius adn essnlly draw from outside goin out ie middle of disk to outer 
        //yes i could use math library or jsut give the value but its seeing the math here and now 
        // WAS CD_MODE == true is now Current_Screen_Value = 2

      // MATH FOR THE RINGS   
      int dx = x - X_Center;  //Delta x == dxf
      int dy = y - Y_Center;
      int Distance_Squared = (dx * dx) + (dy * dy);  //naturally think distacne fomrula sqrt deltx squared pl;us deltas y sawred

      // MATH FOR ROTATION OF A GIVEN POINT AS I WANT TO ROTATE THE ENTIRE "CD" MATH FORMULA FOUND FROM EXAMPLES 
      // float dx = x - X_Center;
      // float dy = y - Y_Center;

      float srcX = dx * cosA - dy * sinA + X_Center;
      float srcY = dx * sinA + dy * cosA + Y_Center;

      int sampleX = round(srcX);   // nearest-pixel sampling, simplest option
      int sampleY = round(srcY);

      // NEW EQUTION I NEEDED TO "MAKE" SINCE WE HAVE 3 BYTES 
      int sampleIndex = (sampleY * 192) + (sampleX * 3);

      uint8_t r = imageBuffer[sampleIndex];
      uint8_t g = imageBuffer[sampleIndex + 1];
      uint8_t b = imageBuffer[sampleIndex + 2]; 

          if ( Distance_Squared <= (Hole_Ring_Radius * Hole_Ring_Radius) ) {
            dma_display->drawPixelRGB888(x, y, 0, 0, 0); //BLK 
            //dma_display->drawPixelRGB888(x, y, Status_Color_Dynamic_Test_R, Status_Color_Dynamic_Test_G, Status_Color_Dynamic_Test_B); //BLK 
          }

          else if ( Distance_Squared <= (Inner_Ring_Radius * Inner_Ring_Radius) ) {
            dma_display->drawPixelRGB888(x, y, 160, 160, 160);                    //white (255) changing to a grey (192) to see if allows for better visibility otherwise its too bright 
          }

          else if ( Distance_Squared <= (Art_Radius * Art_Radius) ) {   // for eq for radius no +1 get a weird extra light casue its 15.5 with its 3 light so -1 must be no lights right not a crazy thing using -1 to put if left most essnlly 
            dma_display->drawPixelRGB888(x, y, r, g, b);          //hey at this x and y value place these colors essenlly
          }

          else if ( Distance_Squared <= (Outer_Ring_Radius * Outer_Ring_Radius) ) {  //Outer_Ring_Radius = 15; //The outer white ring essenlly neetd to be Distance_Squared between R_ArtOuter² and Radius² → that's the ring band → white  outer < distacne < radi 
            dma_display->drawPixelRGB888(x, y, 160, 160, 160);                    //white (255) changing to a grey (192 or 162 i keep change darker ) to see if allows for better visibility otherwise its too bright 
          }

          else {
            dma_display->drawPixelRGB888(x, y, 0, 0, 0); //BLK 
          }
        }

        else if ( Counter_PB_NO_Screen_Value == 1 ) { // --- STANDARD MODE IE JUST ALBUM COVER AND > OR || ICON --- mode essnlly rn basic media player we made WE ME MADE I MADE --- Current_Screen_Value = 1

          uint8_t r = imageBuffer[i++];  //Logic here we would grab the first i valeu then go thru so first iteration r=0 g=1 and b=2 so were going thru to get the 3 bytes information for color here and postfix to get the next byte info as in color
          uint8_t g = imageBuffer[i++];  //remember unsigbned jsut means the range is from 0 to 255 since 8 bits = 1 byte = the range of colors were using in the form of the color code stuff
          uint8_t b = imageBuffer[i++];

          dma_display->drawPixelRGB888(x, y, r, g, b);  //hey at this x and y value place these colors essenlly
        }
      }
    }
}


// UNUSED AS THE PUSH BUTTON DOESNT NEED ANY REAL MAJOR MATH FUNC RN 
// void Math_to_Check_Screen ( int Counter_PB_NO ) {// as a function i need to say hey we have the max of 0~39 where 40 would be a full 360 degree so math wise its tak ethat vale adn if in range change scren 

//   if ( Counter_PB_NO == 1  ) { 
//     Counter_PB_NO_Screen_Value = 1; // Let 1 be the default album art scren 
//   }



// }





// --- THE MEAT OF THE CODE ---
void loop() {                       //Ardunio alwasy has this its like final section
  while (Serial.available() > 0) {  //Serial.available tells us how many bytes curretnlu sitting in incomimng buffer SO if i get 50 bytes we go thru all 50 here and now
    //Reading purely all the inputs ie bytes
    if (readingTag) {
      char c = Serial.read();

      lastByteTime = millis();

      if (c == ':') {
        // Tag is complete — decide what happens next based on it
        if (incomingTag == "ART") {
          expectedBytes = 6144;  //was 3072 changing for right side check
          bytesReceived = 0;

          //amemset(imageBuffer, 0, sizeof(imageBuffer)); // Attempting to fix ghost pause/play --- This buffer clearing being non-free, and non-free operations mattering when synchronous timing with an external, faster process is on the line

          readingTag = false;  // switch modes: now collect pixel bytes instead of tag characters
          readingPixels = true;  // entering pixel-counting mode
        }
        // (later: else if incomingTag == "PLAY" ... etc)
        // ART explained so were reading the incomeing stream of data filling the string ART so one byte = 1 letter when its fill confimred hey we have the actual album cover art/thumbnail art so the folowing 3072 bytes coorelate to that so atp we draw that art then flip back to reading for tag

        // LCD --- essentilly looking thru the text for the "" and eading all within that we cannot go via "i have x amount of bytes == what i mush be tracking" as first were ont dealing with string its in terms of bytes and "WOO" vs "YIPPEE" will be two differnt bytes not same as the preset one for images
        else if (incomingTag == "TXT") {
          incomingText = "";
          readingTag = false;  //Comment out cause this is like yo finnished this area and we keep adding stuff so keep pushign this down
          readingText = true;  // entering terminator-reading mode
        }

        else if (incomingTag == "POS") {  //testing pos
          incomingPosition = "";
          readingTag = false;
          reading_POS = true;  // entering terminator-reading mode
        }

        incomingTag = "";  // reset for next time so essernlly clearing that string itslef
      }

      else {
        incomingTag += c;  //hey if its not a : therefore it must be apart of the regular tag/what im looking for so essenlly build up ART so we weould go from A > AR > ART then do the if above
      }

    }

    else if (readingText) {
      char c = Serial.read();
      lastByteTime = millis();

      if (c == '\n') {
        // Terminator found — message is complete
        processTitleArtist(incomingText);  // split on '|', display it --- a func call out side of all the code itslef
        incomingText = "";                 // declared once, near the top — creates the variable, starts empty and we fill with every +c in eht else
        readingText = false;               // done with text mode
        readingTag = true;                 // go back to watching for the next tag

      }

      else {
        incomingText += c;  // keep building the string, one character at a time
      }
    }

    else if (reading_POS) {  //testing pos
      char c = Serial.read();
      lastByteTime = millis();

      if (c == '\n') {
        // Terminator found — message is complete
        process_Time_For_Piece(incomingPosition);  // split on '|', display it --- a func call out side of all the code itslef --- change fucntion fundamentally yes it works but i need to convert and get the minutes >>> hr >>> whatever i want --- also must be dynamic liek the / in middle for logn time ie hrs
        incomingPosition = "";                     // declared once, near the top — creates the variable, starts empty and we fill with every +c in eht else
        reading_POS = false;                       // done with POS mode
        readingTag = true;                         // go back to watching for the next tag
      }

      else {
        incomingPosition += c;  // keep building the string, one character at a time
      }

    }

    else {
      // We're in "collecting pixel bytes" mode --- so when we say reading pixles true that was esenlly use saying hey were done uop there we can move unto this
      imageBuffer[bytesReceived] = Serial.read();  //reads one byte of the image itself storing into an array of which were collecintng by also byu the size of bytesReceived
      lastByteTime = millis();
      bytesReceived++;  //now just write to next slot

      if (bytesReceived == expectedBytes) {
      // Full image received — draw it
        Draw_Art_Framework(  ); // calling first time to essnlly be like hey im now doing this but really the if statment i have later that uses millis is the real version wher we get spinnin and needs to be its own thing else spinning wont be anywhere close to how i want as were neseted in ifs and elese  

        readingTag = true;  // switch back: go look for the next tag
      }                     // Closeing } for "if == " drawing phase
    }                       // Closeing } for "Else loop "
  }                         // Closeing } for "While Serial.Available"


// Safety net for not having THE VALUE OF BYTES in a given time of 5 Seconds i Believe need to check varibale values 
  if ((readingTag == false || readingPixels == true || readingText == true || reading_POS == true || incomingTag != "") && millis() - lastByteTime > serialTimeout) {

    Serial.println("Serial stall detected — resetting parser state");

    // reset counters
    bytesReceived = 0;

    // reset mode flags back to a known-good "waiting for a tag" state
    readingTag = true;
    readingPixels = false;
    readingText = false;
    reading_POS = false;  //testing pos

    // clear any partial junk sitting in the string buffers
    incomingTag = "";
    incomingText = "";
    incomingPosition = "";  //testing pos
  }


// LCD Screen Logic --- Including scrolls, text 
  if (lcdReady) {
    // Marquee redraw — runs every loop() pass, completely independent of serial data
    if (millis() - lastScrollTime >= scrollInterval) {
      lastScrollTime = millis();

      // LCD --- Drawing the text
      display.clearDisplay();
      display.setTextColor(WHITE);

      drawScreen_1();  //Not passing anythihng yet proff of concept with jsut migrating codebases - WILL NEED to pass the unit conversion for pixels and time for the slider later on

      //Func call does the printing in a both marquee style and static design depending on string length
      Text_Logic(currentTitle, avgCharWidth_Title, title_w, xPos_Title, 34, 18, boundX, boundY, title_h);  //charX_Title --- was last thign sent bu tdont need since declared in function - passing a value to later be by refe only use & in the fucnio n header not call
      Text_Logic(currentArtist, avgCharWidth_Artist, artist_w, xPos_Artist, 46, 28, boundX, boundY, artist_h);

      display.display();

      //Gaurd time - goal is essnlly depending on where we are code wise do we scroll and ho that works
      Slider_Gaurd(IsPausing_Title, HoldStart_Title, holdDuration, xPos_Title, currentTitle, avgCharWidth_Title, 34);
      Slider_Gaurd(IsPausing_Artist, HoldStart_Artist, holdDuration, xPos_Artist, currentArtist, avgCharWidth_Artist, 46);

    }  //Closing } for "millis if"
  }    //Closing } for "lcd ready"


// This is if actull spining part to the cd so main idea is to have a seperate function to have millis and all the work involded as if its in teh main code block above hence the same func call that one is bound to use bytes and serial overall its nested too deep 
  if (millis() - lastSpinTime >= spinInterval) { //Purpose for function is so rather than it beign called once i have a way where it just keeps going and spinning 
    lastSpinTime = millis();

    Draw_Art_Framework( ); //Turned into a function just so it easier to work with as it was getting crazy and function made easier to diagnose 
  } //Closer for "If lastSpinTime"



//SW on the rotaty attepnt 
  PB_NO_Forward_rawReading = digitalRead(PB_NO_Forward);

  // Triggered when actula pbno is changted --- if raw reading changed at all (even mid-bounce), reset the clock
  if (PB_NO_Forward_rawReading != PB_NO_Forward_lastRaw) {
    lastDebounceTime = millis();
  }

  //Has the time between not press and pressed actully real and can i use it --- Wher 30ms passed since the last time anything changed --- only trust the raw reading once it's held steady past the window
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (PB_NO_Forward_rawReading != PB_NO_Forward_debounced) {
      PB_NO_Forward_debounced = PB_NO_Forward_rawReading;

      // Hey when i rpess button move forward thru screens and if i go over a value thats reprenst tha amount of screns i have then reset back to what the roginal screen woudl ben 
      if (PB_NO_Forward_debounced == HIGH) { 

      Counter_PB_NO_Screen_Value++; // mew var 

      if (Counter_PB_NO_Screen_Value > Max_Screens_Made){
        Counter_PB_NO_Screen_Value = 1; 
      }//else woudl nautall be if low then dont move but the real else if i could add later is the second push button goes back on that same one 

      //Math_to_Check_Screen ( Counter_PB_NO_Screen_Value ); // i dont need math involved here at all really 

      }

    } 
    
  }
  
  PB_NO_Forward_lastRaw = PB_NO_Forward_rawReading;

}  // Closeing } for "Void Loop"
