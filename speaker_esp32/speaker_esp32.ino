#include <ESP_I2S.h>
#include <SD.h>
#include <SPI.h>

/* MAX98357A */
#define MAX_BCLK 26
#define MAX_LRC  25
#define MAX_DIN  22

/* SD CARD */
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS 5

/* BUTTON */
#define BTN1 16
#define BTN2 17

I2SClass i2s;
bool pressed_btn1 = false;
bool pressed_btn2 = false;

bool playWavOnce(const char *fileName)
{
  File f;
  f = SD.open(fileName);
  if (!f)
  {
    Serial.println("WAV open failed!");
    SPI.end();
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if(!SD.begin(SD_CS, SPI))
    {
      Serial.println("SD init failed2!");
      return false;
    }
    f = SD.open(fileName);
    if (!f)
    {
      Serial.println("WAV open failed again!");
      return false;
    }
  }

  // WAV HEADER READ
  uint8_t header[44];
  f.read(header, 44);
  uint16_t audioFormat = header[20] | (header[21] << 8);
  uint16_t numChannels = header[22] | (header[23] << 8);
  uint32_t sampleRate  = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
  uint16_t bitsPerSample = header[34] | (header[35] << 8);

  Serial.println("===== WAV Info =====");
  Serial.printf("Audio Format : %u (%s)\n", audioFormat, (audioFormat == 1) ? "PCM" : "Compressed");
  Serial.printf("Channels     : %u (%s)\n", numChannels, (numChannels == 1) ? "Mono" : "Stereo");
  Serial.printf("Sample Rate  : %u Hz\n", sampleRate);
  Serial.printf("Bit Depth    : %u-bit\n", bitsPerSample);
  Serial.println("====================");

  i2s_data_bit_width_t bitWidth;
  switch (bitsPerSample)
  {
    case 16: bitWidth = I2S_DATA_BIT_WIDTH_16BIT; break;
    case 24: bitWidth = I2S_DATA_BIT_WIDTH_24BIT; break;
    case 32: bitWidth = I2S_DATA_BIT_WIDTH_32BIT; break;
    default:
      Serial.printf("Unsupported bit depth: %u\n", bitsPerSample);
      f.close();
      return false;
  }

  i2s.end();
  i2s.setPins(MAX_BCLK, MAX_LRC, MAX_DIN);
  if (!i2s.begin(I2S_MODE_STD, sampleRate, bitWidth,
                 (numChannels == 1) ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO)) {
    Serial.println("I2S init failed!");
    f.close();
    return false;
  }

  static uint8_t buf[4096];
   while (f.available())
  {
    int len = f.read(buf, sizeof(buf));
    if (len > 0) i2s.write(buf, len);
    delay(0);
  }

  f.close();
  return true;
}

void setup()
{
  // UART SETTING
  Serial.begin(115200);

  // SD CARD INIT
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS, SPI)) {
    Serial.println("SD init OK");
  }
  else
  {
    Serial.println("SD init failed!");
  }

  // BUTTON INIT
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
}

void loop() {
  uint8_t btn1_status = digitalRead(BTN1);
  uint8_t btn2_status = digitalRead(BTN2);

  if(btn1_status == 0)
  {
    pressed_btn1 = true;
  }
  if(btn2_status == 0)
  {
    pressed_btn2 = true;
  }

  if(pressed_btn1)
  {
    playWavOnce("/test1.wav");
    pressed_btn1 = false;
  }

  if(pressed_btn2)
  {
    playWavOnce("/test2.wav");
    pressed_btn2 = false;
  }
}
