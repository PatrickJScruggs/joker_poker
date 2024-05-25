# joker_poker
This is a custom program I wrote for the ArduinoMEGA back in 2017 which drives several LED animations for an old Joker Poker (Gottlieb, 1978) pinball playfield. The playfield was too damaged to be used for pinball anymore and so it was repurposed into wall art. The program drives several dynamic LED animations using the XY coordinates of the LEDs. This was back before I had ever touched Python let alone OpenCV, so I assigned the coordinates by simply taking a picture of the playfield, opening it in GIMP, and manually adding the pixel coordinates of each light to a spreadsheet. 


## Features

- Utilizes the ShiftPWM library developed by Elco Jacobs to control 71 LEDs on the playfield
- Assigns constant X and Y coordinates to each LED based on their pixel coordinates in a playfield photo
- Implements various animations, including:
  - Wax-on, wax-off animations with different sweep patterns
  - Scrolling animations with random speed and band width
  - Random flashing animations
  - Custom hand-animated patterns for the lower central 10 LEDs
  - Cross-fading between LEDs of different colors in a random order
  - Animating the large, green LEDs in a pre-defined pattern
- Provides utility functions for LED brightness control and timekeeping
- Uses the TrueRandom library to generate true hardware random number seeds for the Arduino's pseudo-random number generator

## Requirements
- [A vintage 1978 Joker Poker pinball playfield.](https://www.ipdb.org/machine.cgi?gid=1306)
- Arduino MEGA
- [ShiftPWM library by Elco Jacobs](https://github.com/elcojacobs/ShiftPWM)
- [TrueRandom library](https://github.com/sirleech/TrueRandom)

## Installation

1. Install the required libraries:
   - ShiftPWM library: https://github.com/elcojacobs/ShiftPWM
   - TrueRandom library: https://github.com/sirleech/TrueRandom
2. Connect the LED shift registers to the appropriate pins on the Arduino MEGA:
   - Latch pin: 8
   - Data pin: 51
   - Clock pin: 52
3. Upload the code to your Arduino MEGA

## Usage

Once the code is uploaded to the Arduino MEGA, the program will automatically start running the LED animations on the connected pinball playfield.
