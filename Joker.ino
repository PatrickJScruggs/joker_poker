/************************************************************************************************************
   Joker Playfield LED controller (c) Patrick J. Scruggs, updated October 2017.

   This program uses Arduino and the ShiftPWM library developed by Elco Jacobs to flash 71 LEDs
   on a pinball playfield in several different patterns.
   https://github.com/elcojacobs/ShiftPWM
************************************************************************************************************/


/************************************************************************************************************
  Global Constants:
************************************************************************************************************/

//each LED is mapped to a constant X and Y coordinate
const int xCoords[72] = {534, 879, 1215, 240, 108, 423, 431, 462, 403, 346, 873, 896, 917, 940, 1103, 960, 1082,
                         1061, 1041, 1020, 1379, 1318, 1635, 1511, 1638, 1512, 1645, 1591, 1540, 1493, 104, 1609,
                         148, 192, 149, 146, 485, 653, 914, 1006, 1192, 98, 110, 94, 150, 388, 225, 626, 397, 942,
                         1040, 1116, 1196, 1273, 1449, 1349, 1494, 1552, 1040, 1196, 1350, 1641, 1525, 1540, 1637,
                         883, 451, 982, 1253, 114, 0, 242
                        };

const int yCoords[72] = {266, 83, 275, 689, 668, 569, 764, 647, 846, 895, 455, 630, 804, 979, 1141, 1157, 963, 793,
                         615, 443, 697, 665, 679, 692, 865, 908, 1181, 1030, 1223, 1398, 1227, 1484, 1327, 1467,
                         1583, 1671, 1595, 1667, 1777, 1966, 1892, 1978, 2143, 2308, 2426, 2539, 2598, 2653, 2675,
                         2709, 2691, 2710, 2687, 2710, 2710, 2683, 2451, 2606, 2898, 2896, 2895, 2140, 1932, 1877,
                         1923, 1333, 2004, 2076, 2278, 852, 0, 897
                        };

const int maxX = 1922;
const int maxY = 3059;
const int centerX = round(maxX / 2.0);
const int centerY = round(maxY / 2.0);

const byte maxBrightness = 100; // how many levels of apparent brightness are there?

const byte minRate = 1; //rate units are coordinates per ms
const byte maxRate = 5;

const byte fadeSlopeFraction = 20; //how quickly does the brightness drop off from the edge of the sweep line?
const float fadeSlope = (-1.0) * ((float)maxBrightness / ((float)maxY / (float)fadeSlopeFraction) );


const float minScrollWidth = (maxY / 20) / 2;
const float maxScrollWidth = (maxY / 4) / 2;


const int minRandomFlashRunTime = 3000; //how much time do we spend randomly flashing the LEDs?
const int maxRandomFlashRunTime = 10000;

const int minRandomFlashFrameTime = 100; //how often do we assign new boolean states to all the LEDs?
const int maxRandomFlashFrameTime = 500;

const int minTotalFlashTime = 1000; //how much time do we spend flashing all the LEDs at once?
const int maxTotalFlashTime = 4000;

const byte minFlashFrameTime = 50; //what is the period of the flashing LEDs?
const byte maxFlashFrameTime = 250;

const byte minWaxSet = 4; // how many times do we wax-on-and wax off?
const byte maxWaxSet = 10;

const byte minCustomFrameTime = 50;  //how long is each frame of the custom animations displayed?
const byte maxCustomFrameTime = 150;
const byte minNumCustomAni = 1;  //how many custom animations play at a time?
const byte maxNumCustomAni = 3;

const byte minNumGreenLoops = 2; //how many times do the green dots loop around in a chase pattern?
const byte maxNumGreenLoops = 4;

const byte minROAATFrameTime = 50; //how long is each frame of the randomOneAtATime() animation?
const byte maxROAATFrameTime = 100;

const byte minNumRandomRainbowFades = 4; //how many times do we fade between colors?
const byte maxNumRandomRainbowFades = 10;

const int minFadeTime = 100; //how long (ms) does it take to fade and crossfade?
const int maxFadeTime = 250;

const int minOuterLoopFrameTime = 250;
const int maxOuterLoopFrameTime = 500;

const byte minNumOuterLoops = 2;
const byte maxNumOuterLoops = 5;

//constants needed for the ShiftPWM library:
const int ShiftPWM_latchPin = 8;
const bool ShiftPWM_invertOutputs = false;
const bool ShiftPWM_balanceLoad = false;
const unsigned char maxPWMBrightness = 255;
const unsigned char pwmFrequency = 75;


/************************************************************************************************************
  Global Variables:
************************************************************************************************************/

byte animation = 1; //which animation is being displayed?
byte diceRoll = 0; //which animation is playing?

byte brightness[72];  //array for the brightness values to be sent to the LEDs
byte previousBrightness[72]; //array for the previous brightness values sent to the LEDs

byte pwmBri[(maxBrightness + 1)]; //create an array for converting brightness values so that brightness
//appears to scale linearly

boolean invertOutputs = false; //should the brightness values of each LED be flipped before they are assigned?

unsigned long currentTime = 0;
unsigned long animationStartTime = 0;
unsigned long deltaT = 0; // how long has it been since the current animation started?

byte previousDiceRoll = 0; //the dice roll that determines which animation is played is tracked to prevent the
//same animation from playing twice consecutively.
byte previousDiceRoll2 = 0;

float rate = 3.0; //the current animation speed in coordinate units per ms.

byte remainingWaxes = 0; //how many wax-off animations are remaining?

float scrollWidth = 0; // how wide is the band of scrolling LEDs?

int customFrameTime = 0; //how many ms is each frame of the custom animations displayed?
boolean customLEDsOn[10]; //array for the state of the 10 LEDs animated during custom animations

int fadeTime = 0; //how long does it take to fade and cross-fade?

//included libraries:
#include <ShiftPWM.h>
#include <math.h>
#include <TrueRandom.h>

void setup()
{
  //Serial.begin(9600);  //This line is commented out when not debugging

  //pin 8 = latch, pin 51 = DATA, pin 52 = CLOCK
  pinMode(8, OUTPUT);
  pinMode(51, OUTPUT);
  pinMode(52, OUTPUT);

  //initialize the ShiftPWM library and set all LEDs off
  ShiftPWM.SetAmountOfRegisters(9);
  ShiftPWM.SetPinGrouping(1);
  ShiftPWM.Start(pwmFrequency, maxPWMBrightness);
  ShiftPWM.SetAll(0);

  //The randomSeed library is used to generate a true hardware random number seed for the Arduino's pseudo RNG.
  randomSeed(TrueRandom.random());

  //clear the global arrays
  memset(brightness, 0, 72);
  memset(previousBrightness, 0, 72);

  //populate the pwmBri array
  for (int appBri = 0; appBri <= maxBrightness; appBri++)
  {
    pwmBri[appBri] = round(pow (2.0, ((float)appBri / ((float)maxBrightness * ( (log10(2.0)) /
                                      (log10((float)maxPWMBrightness)) )))));
  }

  //force apparent brightness 0 to PWM brightness 0 (it will otherwise round to 1) and force max apparent
  //brightness to max PWMBrightness
  pwmBri[0] = 0;
  pwmBri[maxBrightness] = maxPWMBrightness;

}


//the void loop selects which animation will play
void loop()
{
  const byte numberOfAnimations = 10;

  //after an animation plays, the next two animations must be different
  previousDiceRoll2 = previousDiceRoll;
  previousDiceRoll = diceRoll;

  do
  {
    diceRoll = random(numberOfAnimations) + 1;
  } while (diceRoll  ==  previousDiceRoll || diceRoll == previousDiceRoll2);

  clearLEDs();
  resetTime();


  switch (diceRoll)
  {
    default:
      waxOnWaxOff();

    case 1:
      waxOnWaxOff();
      break;

    case 2:
      scroll();
      break;

    case 3:
      randomFlash();
      break;

    case 4:
      flashAll();
      break;

    case 5:
      customAnimations();
      break;

    case 6:
      randomOneAtATime();
      break;

    case 7:
      randomColorCrossFade();
      break;

    case 8:
      randomColorFlash();
      break;

    case 9:
      greenDotAnimations();
      break;

    case 10:
      outerLoop();
      break;
  }
}


/************************************************************************************************************
   Utility Functions:

   These functions are called by many of the animations.
************************************************************************************************************/

//randomly selects a sweep rate
void selectRate()
{
  rate = random(minRate, (maxRate) ) + ( random(10) * 0.1) + ( random(11) * 0.01);
}


//updates the global timekeeping variables when called
void updateTime()
{
  currentTime = millis(); //update currentTime
  deltaT = abs(currentTime - animationStartTime);
}


//resets the time-keeping variables
void resetTime()
{
  currentTime = millis();
  animationStartTime = currentTime;
  deltaT = 0;
}


//turns off all LEDs and clears the brightness and previousBrightness arrays
void clearLEDs()
{
  invertOutputs = false;
  memset(brightness, 0, 72);
  applyBV();
  memset(previousBrightness, 0, 72);
  ShiftPWM.SetAll(0);
}


//short for "assignBrightnessValue," this function  converts apparent brightness values to actual brightness values
//and stores LED brightness values in the brightness[] array.
void assignBV(byte n, int z)
{
  //fix any out-of-bounds z-values
  if (z > maxBrightness)
  {
    z = maxBrightness;
  }
  else if (z < 0)
  {
    z = 0;
  }

  //convert apparent brightness to actual brightness and store it in brightness array
  brightness[n] =  pwmBri[z];
}


//sends the brightness values in the brightness[] array to the LEDs via the shift registers
void applyBV()
{
  for (byte i = 0; i < 72; i++)
  {
    // only push new brightness values to LEDs whose brightness values have changed since the last time applyBV()
    // was called.
    if (brightness[i] != previousBrightness[i])
    {
      if (invertOutputs == true)
      {
        ShiftPWM.SetOne(i, (maxPWMBrightness - brightness[i]) );
      }

      else
      {
        ShiftPWM.SetOne(i, brightness[i]);
      }
      previousBrightness[i] = brightness[i];
    }
  }
}


/************************************************************************************************************
  Animations:
************************************************************************************************************/


//Alternatley sweeps the LEDs on and off with several diffrent patterns.
void waxOnWaxOff()
{
  remainingWaxes = random(minWaxSet, (maxWaxSet + 1) );
  selectRate();

  while (remainingWaxes > 0)
  {
    byte waxAnimation = random(10) + 1;

    resetTime();

    switch (waxAnimation)
    {
      default:
        sweepUp();
        break;

      case 1:
        sweepUp();
        break;

      case 2:
        sweepDown();
        break;

      case 3:
        sweepRight();
        break;

      case 4:
        sweepLeft();
        break;

      case 5:
        diagSweep1();
        break;

      case 6:
        diagSweep2();
        break;

      case 7:
        diagSweep3();
        break;

      case 8:
        diagSweep4();
        break;

      case 9:
        irisOut();
        break;

      case 10:
        irisIn();
        break;
    }

    //remaining waxes only decrements at the end of an inverted "wax off" animation
    if (invertOutputs == true)
    {
      remainingWaxes--;
      ShiftPWM.SetAll(0);
    }

    else
    {
      ShiftPWM.SetAll(maxPWMBrightness);
    }

    invertOutputs = !invertOutputs;
  }
}


//moves a band of LEDs across the playfield in a predetermined sequence, but with a random speed and band width
void scroll()
{
  selectRate();

  invertOutputs = random(2);

  scrollWidth = random(minScrollWidth, maxScrollWidth) + ( random(10) * 0.1) + ( random(11) * 0.01) ;

  resetTime();
  scrollDown();

  resetTime();
  scrollUp();

  resetTime();
  scrollLeft();

  resetTime();
  scrollRight();

  resetTime();
  diagScroll1();

  resetTime();
  diagScroll3();

  resetTime();
  diagScroll2();

  resetTime();
  diagScroll4();
}


//randomly assign each LED to be on or off every {randomFlashFrameTime} miliseconds
void randomFlash()
{
  //choose how long (ms) the randomFlash() animation will last and how long (ms) each frame will be
  unsigned long randomFlashRunTime = currentTime + random(minRandomFlashRunTime, (maxRandomFlashRunTime + 1));
  unsigned long randomFlashFrameTime =  random(minRandomFlashFrameTime, (maxRandomFlashFrameTime + 1));

  while (currentTime <= randomFlashRunTime)
  {
    for (byte k = 0; k < 72 ; k++)
    {

      //randomly assign each light to be off or on
      boolean cointoss = random(2);

      switch (cointoss)
      {
        default:
          assignBV(k, 0);
          break;

        case 0:
          assignBV(k, 0);
          break;

        case 1:
          assignBV(k, maxBrightness);
          break;
      }
    }
    applyBV();

    //keep light assignments until the frame is over
    delay(randomFlashFrameTime);
    updateTime();
  }
}


//rapidly flash all LEDs on and off
void flashAll()
{
  resetTime();

  //randomly choose how long the flashAll() animation will play, and the frequency of the flashes
  unsigned long totalFlashTime = currentTime + random(minTotalFlashTime, (maxTotalFlashTime + 1));
  unsigned int flashFrameTime = random(minFlashFrameTime, (maxFlashFrameTime + 1) );

  boolean lightsOn = false;

  resetTime();

  while (currentTime <= totalFlashTime)
  {

    updateTime();

    if (deltaT > flashFrameTime)
    {

      if (lightsOn == false)
      {
        ShiftPWM.SetAll(maxPWMBrightness);
      }

      else
      {
        ShiftPWM.SetAll(0);
      }

      lightsOn = !lightsOn;
      resetTime;
    }
  }
}

//animate the lower central 10 lEDs with several diffrent hand-animated patterns.
void customAnimations()
{
  //The CWLoop() or CCWLoop() loop animations are only allowed to be called once per customAnimations().
  //Once one of them has played, neither of them can play again until the next time customAnimations() is called.
  boolean CWorCWWplayed = false;

  //turn on the custom animation "partner" lights
  const byte customAniPartners[8] = {3, 4, 22, 23, 44, 45, 47, 57};

  for (byte i = 0; i < sizeof(customAniPartners); i++)
  {
    assignBV(customAniPartners[i], maxBrightness);
  }
  applyBV();

  //randomly choose how many animations will play, and how long each frame of animation lasts
  byte numCustomAni = random(minNumCustomAni, (maxNumCustomAni + 1) );
  customFrameTime = random (minCustomFrameTime, (maxCustomFrameTime + 1));

  byte customDiceRoll;

  while (numCustomAni > 0)
  {
    //randomly select which animation will play
    if (CWorCWWplayed == false)
    {
      customDiceRoll = random(6) + 1;
    }

    else
    {
      customDiceRoll = random(3, 7);
    }

    switch (customDiceRoll)
    {
      default:
        CWorCWWplayed = true;
        CWLoop();
        break;

      case 1:
        CWorCWWplayed = true;
        CWLoop();
        break;

      case 2:
        CWorCWWplayed = true;
        CCWLoop();
        break;

      case 3:
        flipUpAndDownPinnedLeft();
        break;

      case 4:
        flipUpAndDownPinnedRight();
        break;

      case 5:
        twoJoinUpThenDown();
        break;

      case 6:
        twoJoinDownThenUp();
        break;

    }//end switch case

    //display a blank frame after each animation
    for (int i = 0; i < 10 ; i++)
    {
      assignBV( (i + 10), 0);
    }

    applyBV();

    delay(customFrameTime);

    numCustomAni--;
  }
}


//turns on one LED at a time in a random order, until all LEDs are on
void randomOneAtATime()
{
  invertOutputs = false;
  int frameTime = random(minROAATFrameTime, (maxROAATFrameTime + 1));

  byte listArray[72];
  byte storageBin = 0;
  byte switchIndex = 0;

  //create an ordered array from 0 to 71
  for (byte i = 0; i < 72; i++)
  {
    listArray[i] = i;
  }

  //randomize array with a Fisher-Yates shuffle algorithm
  for (byte i = 71; i > 0; i--)
  {
    switchIndex = random(i);
    storageBin = listArray[switchIndex];
    listArray[switchIndex] = listArray[i];
    listArray[i] = storageBin;
  }


  for (byte i = 0; i < 72 ; i++ )
  {
    assignBV(listArray[i], maxBrightness);
    applyBV();

    delay(frameTime);

  }
}


//cross fade between LEDs of diffrent colors in a random order
void randomColorCrossFade()
{
  //randomly select number of crossfades and fade speed
  byte numFadesRemaining = random(minNumRandomRainbowFades, (maxNumRandomRainbowFades + 1 ) );
  fadeTime = random(minFadeTime, ( maxFadeTime + 1) );
  float fadeSlope = (float)maxBrightness / (float)fadeTime;

  byte color1 = random(5) + 1;
  byte color2 = color1;
  int brightness1 = 0;
  int brightness2 = 0;

  //fade up first color
  resetTime();

  long startFadeTime = currentTime;

  while (deltaT <= fadeTime)
  {
    brightness1 = round( deltaT * fadeSlope);

    setColorBrightness(color1, brightness1);

    updateTime();
  }

  //after fade up, force color 1 to maxBrightness
  setColorBrightness(color1, maxBrightness);

  delay(round((float)fadeTime / 2));

  //begin cross fading
  while (numFadesRemaining > 0)
  {
    //randomly sleect color2, != color1
    do
    {
      color2 = random(5) + 1;
    } while (color2 == color1);

    resetTime();

    startFadeTime = currentTime;

    while (deltaT <= fadeTime)
    {
      brightness1 = round( deltaT *  (-1) * fadeSlope  + maxBrightness);
      brightness2 = round( deltaT * fadeSlope);

      setColorBrightness(color1, brightness1);
      setColorBrightness(color2, brightness2);

      updateTime();
    }

    setColorBrightness(color1, 0);
    setColorBrightness(color2, maxBrightness);

    delay(round((float)fadeTime / 2));
    color1 = color2;
    numFadesRemaining--;
  }

  //fade out last color
  resetTime();
  startFadeTime = currentTime;

  while (deltaT <= fadeTime)
  {
    brightness1 = round( deltaT *  (-1) * fadeSlope  + maxBrightness);
    setColorBrightness(color1, brightness1);
    updateTime();
  }

  setColorBrightness(color1, 0);
  delay(round((float)fadeTime / 2));

}


//cross fade between LEDs of diffrent colors in a random order
void randomColorFlash()
{
  updateTime();

  //Borrow the parameters from randomFlash() to decide how long each frame lasts and how long the animation lasts
  unsigned long randomFlashRunTime = currentTime + random(minRandomFlashRunTime, (maxRandomFlashRunTime + 1));
  unsigned long randomFlashFrameTime =  random(minRandomFlashFrameTime, (maxRandomFlashFrameTime + 1));

  while (currentTime <= randomFlashRunTime)
  {
    byte color = random(5) + 1;
    setColorBrightness(color, maxBrightness);

    delay(randomFlashFrameTime);

    setColorBrightness(color, 0);

    updateTime();
  }

}


//animates the large, green LEDs in a pre-defined pattern
void greenDotAnimations()
{
  //turn on the green dot "partner" lights
  const byte greenDotPartners[9] = {26, 27, 28, 29, 31, 36, 37, 38, 40};

  for (byte i = 0; i < sizeof(greenDotPartners); i++)
  {
    assignBV(greenDotPartners[i], maxBrightness);
  }
  applyBV();

  byte greenDiceRoll = random(3, 6);
  int numAniRemaining = greenDiceRoll;
  fadeTime = random(minFadeTime, ( maxFadeTime + 1) );

  //flash the green dots in a simple pattern
  while ((numAniRemaining) > 0)
  {

    while ((numAniRemaining * 2) > 0)
    {
      assignBV(69, maxBrightness);
      assignBV(71, maxBrightness);
      applyBV();
      delay(50);
      assignBV(69, 0);
      assignBV(71, 0);
      applyBV();
      delay(50);
      numAniRemaining--;
    }

    numAniRemaining = greenDiceRoll;

    while ((numAniRemaining * 2) > 0)
    {
      assignBV(58, maxBrightness);
      assignBV(59, maxBrightness);
      assignBV(60, maxBrightness);
      applyBV();
      delay(50);
      assignBV(58, 0);
      assignBV(59, 0);
      assignBV(60, 0);
      applyBV();
      delay(50);
      numAniRemaining--;
    }

    numAniRemaining = greenDiceRoll;

    while ((numAniRemaining * 2) > 0)
    {
      assignBV(24, maxBrightness);
      assignBV(25, maxBrightness);
      applyBV();
      delay(50);
      assignBV(24, 0);
      assignBV(25, 0);
      applyBV();
      delay(50);
      numAniRemaining--;
    }
    numAniRemaining--;
  }

  greenCrossFadeChaseCW();
}

//flash the LEDs around the perimeter of the playfield in clockwise sequence
void outerLoop()
{

  const byte outerLoopLEDs[26] = { 1, 0, 4, 69, 30, 32, 33, 34, 35, 41, 42, 43, 46, 48, 58, 59, 60, 54, 57, 61,
                                   64, 31, 26, 24, 22, 2
                                 };

  int outerLoopFrameTime = random(minOuterLoopFrameTime, (maxOuterLoopFrameTime + 1));
  int loopsRemaining = random(minNumOuterLoops, (maxNumOuterLoops + 1));

  //turn on this central LED, and keep it on during the animation.
  assignBV(65, maxBrightness);
  applyBV();

  while (loopsRemaining > 0)
  {

    assignBV(outerLoopLEDs[0], maxBrightness);
    applyBV();

    delay(100);



    for (byte i = 0; i < 25 ; i++)
    {
      assignBV(outerLoopLEDs[i], 0);
      assignBV(outerLoopLEDs[(i + 1)], maxBrightness);
      applyBV();

      delay(100);

    }

    assignBV(outerLoopLEDs[25], 0);

    delay(100);


    loopsRemaining--;
  }

}


/************************************************************************************************************
  Wax-on, wax-off animations:

  These are the individual animations used by the waxOnWaxOff()
************************************************************************************************************/


//Sweeps from the bottom of the playfield to the top
void sweepUp()
{
  float y = 0; // start imaginary line at zero

  while (y < maxY)
  {
    //for each light, check to see if its y-coordinate is under the imaginary line
    for (byte j = 0; j < 72 ; j++)
    {
      if (yCoords[j] < y)
      {
        assignBV(j, maxBrightness);
      }

      else
      {
        int z = round(fadeSlope * abs(y - yCoords[j]) ) + maxBrightness;
        assignBV(j, z);
      }
    }//end zone-check for-loop

    applyBV();
    updateTime();
    y = rate * (float)deltaT; //calculate new position of the imaginary line
  }
}


//Sweeps from the top of the playfield to the bottom
void sweepDown()
{
  float y = maxY; // start imaginary line at zero

  while (y > 0)
  {
    //for each light, check to see if its y-coordinate is above the imaginary line
    for (byte j = 0; j < 72 ; j++)
    {
      if (yCoords[j] > y)
      {
        assignBV(j, maxBrightness);
      }
      else
      {
        int z = round(fadeSlope * abs(y - yCoords[j]) ) + maxBrightness;
        assignBV(j, z);
      }
    }//end zone-check for-loop

    applyBV();
    updateTime();
    y = maxY - (rate * (float)deltaT); //calculate new position of the imaginary line
  }
}


//Sweeps from the left side of the playfield to the right side
void sweepRight()
{
  float x = 0; // start imaginary line at zero

  while (x < maxX)
  {
    //for each light, check to see if its x-coordinate is to the left of the imaginary line
    for (byte i = 0; i < 72 ; i++)
    {
      if (xCoords[i] < x)
      {
        assignBV(i, maxBrightness);
      }
      else
      {
        int z = round(fadeSlope * abs(x - xCoords[i]) ) + maxBrightness;
        assignBV(i, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    x = rate * (float)deltaT; //calculate new position of the imaginary line
  }
}


//Sweeps from the right side of the playfield to the left side
void sweepLeft()
{
  float x = maxX; // start imaginary line at maxX

  while (x > 0)
  {
    //for each light, check to see if its x-coordinate is to the right of the imaginary line
    for (byte i = 0; i < 72 ; i++)
    {
      if (xCoords[i] > x)
      {
        assignBV(i, maxBrightness);
      }
      else
      {
        int z = round(fadeSlope * abs(x - xCoords[i]) ) + maxBrightness;
        assignBV(i, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    x = maxX - (rate * (float)deltaT); //calculate new position of the imaginary line
  }
}



void diagSweep1() //sweeps from lower-left corner to upper-right corner
{
  float c = 0.0;

  while ( c > ((maxX + maxY) * (-1)) )
  {
    //for each light, check to see if it's below the imaginary line
    for (byte k = 0; k < 72 ; k++)
    {
      if ( (xCoords[k] + yCoords[k] + c) < 0)
      {
        assignBV(k, maxBrightness);
      }
      else
      {
        float distance = abs( xCoords[k] + yCoords[k] + c) / sqrt(2) ;
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    c = 0 - (rate * (float) deltaT); //calculate new position of the imaginary line
  }
}


void diagSweep2() //sweeps from lower-right corner to upper-left corner
{
  float c = maxX;

  while (c > ((-1) * maxY))
  {
    //for each light, check to see if its below the imaginary line
    for (byte k = 0; k < 72 ; k++)
    {
      if ( ( ((-1) * xCoords[k]) + yCoords[k] + c) < 0)
      {
        assignBV(k, maxBrightness);
      }
      else
      {
        float distance = abs(  (-1 * xCoords[k]) +  yCoords[k] + c) / sqrt(2);
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    c = maxX - (rate * (float)deltaT); //calculate new position of the imaginary line
  }
}

void diagSweep3() //sweeps from upper-right corner to lower-left corner
{
  float c = (maxX + maxY) * (-1);
  const float initialC = c;

  while (c < 0)
  {
    //for each light, check to see if it's below the imaginary line
    for (byte k = 0; k < 72 ; k++)
    {
      if ( (xCoords[k] + yCoords[k] + c) > 0)
      {
        assignBV(k, maxBrightness);
      }

      else
      {
        float distance = abs(xCoords[k] +  yCoords[k] + c) / sqrt(2);
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }//end zone-check for-loop

    updateTime();
    c = initialC + (rate * (float)deltaT); //calculate new position of the imaginary line
    applyBV();
  }
}


void diagSweep4() //sweeps from upper-left corner to lower-right corner
{
  float c = (maxY) * (-1);
  const float initialC = c;

  while (c < maxX)
  {
    //for each light, check to see if it's below the imaginary line
    for (byte k = 0; k < 72 ; k++)
    {
      if ( ( ( (-1) * xCoords[k]) + yCoords[k] + c) > 0)
      {
        assignBV(k, maxBrightness);
      }

      else
      {
        float distance = abs((-1 * xCoords[k]) + yCoords[k] + c) / sqrt(2);
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }

    applyBV();

    updateTime();
    c = initialC + (rate * (float)deltaT); //calculate new position of the imaginary line
  }
}


void irisOut()
{
  float r = 0.0;

  while ( r < maxY)
  {
    for (byte k = 0; k < 72 ; k++) //for each light, check to see if its inside he circle
    {
      float distance = sqrt(pow( (centerX - xCoords[k]), 2) + pow( (centerY - yCoords[k]), 2));

      if (distance <= r)
      {
        assignBV(k, maxBrightness);
      }

      else
      {
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }

    applyBV();

    updateTime();
    r = 0 + (rate * (float)deltaT);
  }
}


void irisIn()
{
  float r = maxY;

  while ( r > 0)
  {
    for (byte k = 0; k < 72 ; k++) //for each light, check to see if its outside he circle
    {
      float distance = sqrt(pow((centerX - xCoords[k]), 2) + pow((centerY - yCoords[k]), 2));

      if (distance >= r)
      {
        assignBV(k, maxBrightness);
      }

      else
      {
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }
    applyBV();

    updateTime();
    r = maxY - (rate * (float) deltaT);
  }
}


/************************************************************************************************************
  Scroll animations:

  These are the individual animations used by the scroll() function
************************************************************************************************************/

//move a horizontal band from the bottom to the top of the playfield
void scrollUp()
{
  // start imaginary line far enough below zero that no LEDs are lit
  float y = 0 - (scrollWidth + ((float)maxX / fadeSlopeFraction));
  const float initalY = y;

  while (y < (maxY + (scrollWidth + ((float)maxX / fadeSlopeFraction))))
  {

    //for each light, check to see if its y-coordinate is under the imaginary line
    for (byte j = 0; j < 72 ; j++)
    {
      if (yCoords[j] < (y + scrollWidth) && yCoords[j] > (y - scrollWidth))
      {
        assignBV(j, maxBrightness);
      }
      else
      {
        int z;

        if (yCoords[j] < y)
        {
          z = round(fadeSlope * abs((y - scrollWidth) - yCoords[j])) + maxBrightness;
        }

        else
        {
          z = round(fadeSlope * abs((y + scrollWidth) - yCoords[j])) + maxBrightness;
        }

        assignBV(j, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    y = initalY + (rate * (float)deltaT); //calculate new position of the imaginary line
  }
} //end scrollUp

//Scrolls from the top of the playfield to the bottom
void scrollDown()
{
  float y = maxY + (scrollWidth + (maxX / fadeSlopeFraction)); // start imaginary line at zero
  const float initalY = y;

  while (y > (0 - (scrollWidth + (maxX / fadeSlopeFraction))))
  {
    //for each light, check to see if its y-coordinate is above the imaginary line
    for (byte j = 0; j < 72 ; j++)
    {
      if (yCoords[j] < (y + scrollWidth) && yCoords[j] > (y - scrollWidth))
      {
        assignBV(j, maxBrightness);
      }
      else
      {
        int z;

        if (yCoords[j] < y)
        {
          z = round(fadeSlope * abs((y - scrollWidth) - yCoords[j])) + maxBrightness;
        }

        else
        {
          z = round(fadeSlope * abs((y + scrollWidth) - yCoords[j])) + maxBrightness;
        }
        assignBV(j, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    y = initalY - (rate * (float) deltaT); //calculate new position of the imaginary line

  }//end while loop
} //end scrollDown()


//Scrolls from the left side of the playfield to the right side
void scrollRight()
{
  float x = 0 - (scrollWidth + (maxX / fadeSlopeFraction)); // start imaginary line at zero
  const float initalX = x;

  while (x < (maxX + (scrollWidth + (maxX / fadeSlopeFraction))))
  {
    //for each light, check to see if its x-coordinate is to the left of the imaginary line
    for (byte i = 0; i < 72 ; i++)
    {
      if (xCoords[i] < (x + scrollWidth) && xCoords[i] > (x - scrollWidth))
      {
        assignBV(i, maxBrightness);
      }

      else
      {
        int z;

        if (xCoords[i] < x)
        {
          z = round(fadeSlope * abs((x - scrollWidth) - xCoords[i])) + maxBrightness;
        }

        else
        {
          z = round(fadeSlope * abs((x + scrollWidth) - xCoords[i])) + maxBrightness;
        }

        assignBV(i, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    x = initalX + (rate * (float)deltaT); //calculate new position of the imaginary line

  }//end while loop
} //end sweepRight


//Sweeps from the right side of the playfield to the left side
void scrollLeft()
{
  float x = maxX + (scrollWidth + (maxX / fadeSlopeFraction)); // start imaginary line at zero
  const float initalX = x;

  while (x > (0 - (scrollWidth + (maxX / fadeSlopeFraction))))
  {
    //for each light, check to see if its x-coordinate is close the imaginary line
    for (byte i = 0; i < 72 ; i++)
    {
      if (xCoords[i] < (x + scrollWidth) && xCoords[i] > (x - scrollWidth))
      {
        assignBV(i, maxBrightness);
      }

      else
      {
        int z;

        if (xCoords[i] < x)
        {
          z = round(fadeSlope * abs((x - scrollWidth) - xCoords[i])) + maxBrightness;
        }

        else
        {
          z = round(fadeSlope * abs((x + scrollWidth) - xCoords[i])) + maxBrightness;
        }
        assignBV(i, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    x = initalX - (rate * (float)deltaT); //calculate new position of the imaginary line

  }//end while loop
} //end sweepLeft

void diagScroll1() //sweeps from lower-left corner to upper-right corner
{
  float c = 0.0;

  while (c > ( (maxX + maxY) * (-1) ))
  {
    //for each light, check to see if its close to the imaginary line
    for (byte k = 0; k < 72 ; k++)
    {
      float distance = abs( xCoords[k] + yCoords[k] + c) / sqrt(2);

      if (distance < scrollWidth)
      {
        assignBV(k, maxBrightness);
      }

      else
      {
        distance = distance - scrollWidth;
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    c = 0.0 - (rate * (float) deltaT); //calculate new position of the imaginary line

  }//end while loop
}//end diagScroll1()


void diagScroll2() //scrolls from lower-right corner to upper-left corner
{
  float c = maxX;

  while (c > ((-1) * maxY))
  {
    for (byte k = 0; k < 72 ; k++)
      //for each light, check to see if its close the imaginary line
    {
      float distance = abs( (-1 * xCoords[k]) + yCoords[k] + c) / sqrt(2);

      if (distance < scrollWidth)
      {
        assignBV(k, maxBrightness);
      }
      else
      {
        distance = distance - scrollWidth;
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    c = maxX - (rate * (float)deltaT); //calculate new position of the imaginary line

  }//end while loop
}//end diagScroll2

void diagScroll3() //scrolls from upper-right corner to lower-left corner
{
  float c = (maxX + maxY) * (-1);
  const float initalC = c;

  while (c < 0)
  {
    //for each light, check to see if its close to the imaginary line
    for (byte k = 0; k < 72 ; k++)
    {

      float distance = abs(xCoords[k] +  yCoords[k] + c) / sqrt(2);

      if (distance < scrollWidth)
      {
        assignBV(k, maxBrightness);
      }
      else
      {
        distance = distance - scrollWidth;
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }//end zone-check for-loop

    applyBV();

    updateTime();
    c = initalC + (rate * (float) deltaT); //calculate new position of the imaginary line

  }//end while loop
}//end diagScroll3()


void diagScroll4() //scrolls from upper-left corner to lower-right corner
{
  float c = (maxY) * (-1);
  const float initalC = c;


  while (c < maxX)
  {
    //for each light, check to see if its close to the imaginary line
    for (byte k = 0; k < 72 ; k++)
    {
      float distance = abs( (-1 * xCoords[k]) + yCoords[k] + c) / sqrt(2);

      if (distance < scrollWidth)
      {
        assignBV(k, maxBrightness);
      }
      else
      {
        distance = distance - scrollWidth;
        int z = round(fadeSlope * distance) + maxBrightness;
        assignBV(k, z);
      }
    }

    applyBV();

    updateTime();
    c = initalC + (rate * (float)deltaT); //calculate new position of the imaginary line

  }//end while loop
}//end diagScroll4()

/************************************************************************************************************
  Custom Animation Functions:

  These are the individual animations and functions used by the customAnimations() function
************************************************************************************************************/


//cycles through each frame of the selected custom animation, and assigns and applies brightness values to
// the ten LEDs for each frame
void playAnimation(boolean inputArray[][10], int sizeOfArray)
{
  for (int i = 0; i < sizeOfArray; i++)
  {
    for (byte j = 0; j < 10; j++)
    {
      customLEDsOn[j] = inputArray[i][j];
    }

    //The values of customLEDsOn[4] and customLEDsOn[5] need to be swapped to correct an error in the shift
    //register circuit board.
    boolean storageBin = customLEDsOn[4];
    customLEDsOn[4] = customLEDsOn[5];
    customLEDsOn[5] = storageBin;

    byte z = 0;

    for (int i = 0; i < 10 ; i++)
    {
      if (customLEDsOn[i] == true)
      {
        z = maxBrightness;
      }

      else
      {
        z = 0;
      }

      assignBV( (i + 10), z);

    }

    applyBV();

    delay(customFrameTime);
  }
}

void CWLoop()
{
  const boolean animation[][10] =
  {
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},

    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},

    {1, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1},

    {1, 0, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1},

    {1, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1},

    {1, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1},

    {1, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1},

    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1},

    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},

    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1},

    {1, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1},

    {1, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1},

    {1, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1},

    {1, 0, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1},

    {1, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1},

    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},

    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1}
  };

  playAnimation(animation, 179);
}

void CCWLoop()
{
  const boolean animation[][10] =
  {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},

    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 1},

    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 1, 1},

    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 1, 1, 1, 1},

    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 1, 1, 1, 1, 1},

    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 1, 1, 1, 1, 1},

    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1},

    {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1},

    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1},

    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1},

    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 1, 1, 1, 1, 1},

    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 1, 1, 1, 1, 1},

    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 1, 1, 1, 1},

    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 1, 1},

    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 1},

    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };

  playAnimation(animation, 179);

}

void flipUpAndDownPinnedLeft()
{
  boolean animation[][10] =
  {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 1, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1}
  };

  playAnimation(animation, 41);
}


void flipUpAndDownPinnedRight()
{
  boolean animation[][10] =
  {
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 1, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };

  playAnimation(animation, 41);
}


void twoJoinUpThenDown()
{
  boolean animation[][10] =
  {
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},

    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
    {0, 1, 0, 0, 1, 1, 0, 0, 1, 0},
    {0, 0, 1, 0, 1, 1, 0, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},

    {1, 0, 0, 1, 1, 1, 1, 0, 0, 1},
    {0, 1, 0, 1, 1, 1, 1, 0, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},

    {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0},

    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},

    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 0, 1, 1, 1, 1, 0, 1, 0},
    {1, 0, 0, 1, 1, 1, 1, 0, 0, 1},

    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 1, 0, 1, 1, 0, 1, 0, 0},
    {0, 1, 0, 0, 1, 1, 0, 0, 1, 0},
    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1},

    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}
  };

  playAnimation(animation, 30);
}


void twoJoinDownThenUp()
{
  boolean animation[][10] =
  {
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},


    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
    {1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},

    {1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
    {1, 1, 0, 1, 0, 0, 1, 0, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 1, 1, 1},

    {1, 1, 1, 0, 1, 1, 0, 0, 1, 1},
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1},

    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 1, 1, 0, 1, 1, 1},

    {1, 1, 1, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 0, 1, 0, 0, 1, 0, 1, 1},
    {1, 1, 0, 0, 1, 1, 0, 0, 1, 1},

    {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 1},
    {1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 0, 0, 0, 1},


    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 0}

  };

  playAnimation(animation, 30);
}


/************************************************************************************************************
   Color Animation Function:

  This functions is used by randomColorCrossFade() and randomColorFlash() to assign brightness
  values to LEDs of a particular color.
************************************************************************************************************/


// set every led that is color 'c' to brightness 'z'
void setColorBrightness(byte c, int z)
{
  switch (c)
  {
    default:
      {
        const byte redLEDs[14] = {1, 6, 22, 26, 30, 36, 47, 48, 53, 54, 57, 61, 63};
        for (byte i = 0; i < 14; i++)
        {
          assignBV(redLEDs[i], z);
        }
      }
      break;

    case 1:
      {
        const byte redLEDs[12] = {1, 6, 22, 26, 30, 36, 47, 48, 53, 57, 61, 63};
        for (byte i = 0; i < 12; i++)
        {
          assignBV(redLEDs[i], z);
        }
      }
      break;

    case 2:
      {
        const byte yellowLEDs[19] = {0, 7, 9, 14, 16, 17, 18, 19, 27, 31, 32, 39, 41, 43, 46, 50, 54, 56, 62};

        for (byte i = 0; i < 19; i++)
        {
          assignBV(yellowLEDs[i], z);
        }
      }
      break;

    case 3:
      {
        const byte greenLEDs[20] = {2, 5, 8, 21, 24, 25, 29, 33, 35, 37, 40, 44, 51, 58, 59, 60, 64, 67, 69, 71};

        for (byte i = 0; i < 20; i++)
        {
          assignBV(greenLEDs[i], z);
        }
      }
      break;

    case 4:
      {
        const byte blueLEDs[12] = {4, 10, 11, 12, 13, 15, 23, 28, 38, 42, 49, 55};

        for (byte i = 0; i < 12; i++)
        {
          assignBV(blueLEDs[i], z);
        }
      }
      break;

    case 5:
      {
        const byte purpleLEDs[8] = {3, 20, 34, 45, 52, 65, 66, 68};

        for (byte i = 0; i < 8; i++)
        {
          assignBV(purpleLEDs[i], z);
        }
      }
      break;
  }

  applyBV();
}


/************************************************************************************************************
  Green Dot Animation Functions:

  These functions are used by the greenDotAnimations() animation to fade and cross-fade individual LEDs.
************************************************************************************************************/

//cross-fade two individual LEDs
void crossFade(byte LED1, byte LED2, int fadeTime)
{
  float fadeSlope = (float)maxBrightness / (float)fadeTime;

  assignBV(LED1, maxBrightness);
  assignBV(LED2, 0);
  applyBV();

  resetTime();
  long  startFadeTime = currentTime;

  while (deltaT <= fadeTime)
  {
    int brightness1 = round( abs(currentTime - startFadeTime) * ( (-1) * (fadeSlope) ) + maxBrightness);
    int brightness2 = round( abs(currentTime - startFadeTime) * (fadeSlope));

    assignBV(LED1, brightness1);
    assignBV(LED2, brightness2);
    applyBV();
    updateTime();
  }
  assignBV(LED1, 0);
  assignBV(LED2, maxBrightness);
}


//fade up a single LED
void fadeUp(byte LED1, int fadeTime)
{
  float fadeSlope = (float)maxBrightness / (float)fadeTime;

  assignBV(LED1, 0);
  applyBV();

  resetTime();
  long startFadeTime = currentTime;

  while (deltaT <= fadeTime)
  {
    int brightness1 = round( abs(currentTime - startFadeTime) * (fadeSlope));
    assignBV(LED1, brightness1);

    applyBV();
    updateTime();
  }
  assignBV(LED1, maxBrightness);
}

//fade out a single LED
void fadeOut(byte LED1, int fadeTime)
{
  float fadeSlope = (float)maxBrightness / (float)fadeTime;

  assignBV(LED1, maxBrightness);
  applyBV();

  resetTime();
  long startFadeTime = currentTime;

  while (deltaT <= fadeTime)
  {
    int brightness1 = round( abs(currentTime - startFadeTime) * ( (-1) * (fadeSlope) ) + maxBrightness);
    assignBV(LED1, brightness1);

    applyBV();
    updateTime();
  }
  assignBV(LED1, 0);
}

//cross-fade between the large,green LEDs in a clockwise pattern
void greenCrossFadeChaseCW()
{

  int remainingLoops = random(1, 4);

  fadeUp(71, fadeTime);

  while (remainingLoops > 0)
  {
    crossFade(71, 69, fadeTime);
    crossFade(69, 58, fadeTime);
    crossFade(58, 59, fadeTime);
    crossFade(59, 60, fadeTime);
    crossFade(60, 24, fadeTime);
    crossFade(24, 25, fadeTime);
    crossFade(25, 71, fadeTime);
    remainingLoops--;
  }
  fadeOut(71, fadeTime);
}


void greenCrossFadeChaseCCW()
{

  int remainingLoops = random(1, 4);


  fadeUp(25, fadeTime);

  while (remainingLoops > 0)
  {
    crossFade(25, 24, fadeTime);
    crossFade(24, 60, fadeTime);
    crossFade(60, 59, fadeTime);
    crossFade(59, 58, fadeTime);
    crossFade(58, 69, fadeTime);
    crossFade(69, 71, fadeTime);
    crossFade(71, 25, fadeTime);
    remainingLoops--;
  }
  fadeOut(25, fadeTime);
}
