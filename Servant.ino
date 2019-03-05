// Quick-and-Dirty Demo of FastLED Parallel Strip Support on ESP32
//
// Uses FastLED's RMT support on the ESP32 to drive 8 channels in parallel
//
// (c) 2019 Dave Plummer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <FastLED.h>                              // Assumes 3.2 or better
#include <U8g2lib.h>                              // So we can talk to the CUU text 
#include <FastLED.h>                              // FastLED for the LED panels
#include <pixeltypes.h>                           // Handy color and hue stuff
#include <gfxfont.h>                              // Adafruit GFX for the panels 
#include <Fonts/FreeSans9pt7b.h>                  // A nice font for the VFD

#define LED_PIN0  21                              // These work for me on a Heltec Wiki32 board
#define LED_PIN1  22
#define LED_PIN2  19
#define LED_PIN3   5
#define LED_PIN4  17
#define LED_PIN5   0
#define LED_PIN6   2
#define LED_PIN7  12

#define USE_TFT 1                                 // Use the little TFT on the Heltec board to diplay framerate
#define STACK_SIZE      4096				              // Stack size for each new thread
#define ARRAYSIZE(n) (sizeof(n)/sizeof(n[0]))     // Count of elements in array, as opposed to bytecount

#define STRIP_SIZE 144*7                          // Just over 1000 LEDs per segment x 8 segments = 8064 LEDs

CRGB g_rgbData0[STRIP_SIZE];                      // Frame buffers (color bytes) for the 8 LED strip segments
CRGB g_rgbData1[STRIP_SIZE];
CRGB g_rgbData2[STRIP_SIZE];
CRGB g_rgbData3[STRIP_SIZE];
CRGB g_rgbData4[STRIP_SIZE];
CRGB g_rgbData5[STRIP_SIZE];
CRGB g_rgbData6[STRIP_SIZE];
CRGB g_rgbData7[STRIP_SIZE];

// FPS
// 
// Given a millisecond value for when the last frame took place and the current timestamp returns the number of
// frames per second, as low as 0.  Never exceeds 999 so you can make some width assumptions.

#define MS_PER_SECOND 1000

int FPS(unsigned long start, unsigned long end)
{
	unsigned long msDuration = end - start;
	float fpsf = 1.0f / (msDuration / (float)MS_PER_SECOND);
	int FPS = (int)fpsf;
	if (FPS > 999)
		FPS = 999;
	return FPS;
}
volatile int g_FPS;

// Enable the Heltec Wif Kit 32 TFT board.  Undefine this if you are on a different board.

#if USE_TFT
U8G2_SSD1306_128X64_NONAME_F_SW_I2C g_TFT(U8G2_R2, 15, 4, 16);
#endif
TaskHandle_t                        g_taskTFT = nullptr;

// TFTUpdateLoop
//
// Displays statistics on the Heltec's built in TFT board.  If you are using a different board, you would simply get rid of
// this or modify it to fit a screen you do have.  You could also try serial output, as it's on a low-pri thread it shouldn't
// disturb the primary cores, but I haven't tried it myself.

void TFTUpdateLoop(void *)
{
 #if USE_TFT
  g_TFT.clear();
 #endif

	for (;;)
	{ 
		char szBuffer[256];
 #if USE_TFT
    g_TFT.setDisplayRotation(U8G2_R2);
   	g_TFT.clearBuffer();						// clear the internal memory
	  g_TFT.setFont(u8g2_font_profont15_tf);	// choose a suitable font

    g_TFT.setCursor(0,10);
    g_TFT.print("Update Speed: ");
    g_TFT.print(g_FPS);
    g_TFT.sendBuffer();

#endif        
    delay(200);
	}
}

// setup
//
// Invoked once at boot, does initial chip setup and application initial

void setup() 
{
#if USE_TFT
    g_TFT.begin();
    g_TFT.clear();
    g_TFT.clearBuffer();                   // clear the internal memory
    g_TFT.setFont(u8g2_font_profont15_tf); // choose a suitable font
    g_TFT.setCursor(0,10);
    g_TFT.println("TFT Ready");
    g_TFT.sendBuffer();
#endif

    Serial.begin(115200);

    // Spin off the TFT stuff as a separate worker thread
    xTaskCreatePinnedToCore(TFTUpdateLoop, "TFT Loop", STACK_SIZE, nullptr, 0, &g_taskTFT, 0);							// UI stuff not bound to any core and at lower priority

    // Add all 8 segmetns to FastLED
    FastLED.addLeds<WS2812B, LED_PIN0, GRB>(g_rgbData0, STRIP_SIZE);
    FastLED.addLeds<WS2812B, LED_PIN1, GRB>(g_rgbData1, STRIP_SIZE);
    FastLED.addLeds<WS2812B, LED_PIN2, GRB>(g_rgbData2, STRIP_SIZE);
    FastLED.addLeds<WS2812B, LED_PIN3, GRB>(g_rgbData3, STRIP_SIZE);
    FastLED.addLeds<WS2812B, LED_PIN4, GRB>(g_rgbData4, STRIP_SIZE);
    FastLED.addLeds<WS2812B, LED_PIN5, GRB>(g_rgbData5, STRIP_SIZE);
    FastLED.addLeds<WS2812B, LED_PIN6, GRB>(g_rgbData6, STRIP_SIZE);
    FastLED.addLeds<WS2812B, LED_PIN7, GRB>(g_rgbData7, STRIP_SIZE);

    // Check yourself before you wreck yourself, make sure your power supply doesn't burn down your shop
    FastLED.setBrightness(4);
}


void loop()
{
    // Simple walking rainbow offset on the various strips

    static int hue = 0;
    hue+=8;

    fill_rainbow(g_rgbData0, STRIP_SIZE, hue, 5);
    fill_rainbow(g_rgbData1, STRIP_SIZE, hue+32, 5);
    fill_rainbow(g_rgbData2, STRIP_SIZE, hue+64, 5);
    fill_rainbow(g_rgbData3, STRIP_SIZE, hue+96, 5);
    fill_rainbow(g_rgbData4, STRIP_SIZE, hue+128, 5);
    fill_rainbow(g_rgbData5, STRIP_SIZE, hue+160, 5);
    fill_rainbow(g_rgbData6, STRIP_SIZE, hue+192, 5);
    fill_rainbow(g_rgbData7, STRIP_SIZE, hue+224, 5);

    unsigned static long lastTime = 0;
    FastLED.show();
    g_FPS = FPS(lastTime, millis());
    lastTime = millis();
    delay(5); 
}