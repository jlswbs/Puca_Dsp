// PÚCA_DSP - M16 DSP Frequency Modulation + moog filter + reverb //
// https://github.com/algomusic/M16 //

#include "driver/i2s.h"
#include "driver/gpio.h"
#include "WM8978.h"
#include "M16.h"
#include "Osc.h"
#include "Bob.h"
#include "FX.h"

#define SAMPLE_RATE   48000
#define DMA_BUF_LEN   32
#define DMA_NUM_BUF   2
#define BPM           120

float randomf(float minf, float maxf) {return minf + (esp_random()%(1UL << 31))*(maxf - minf) / (1UL << 31);}

static short out_buf[DMA_BUF_LEN * 2];

int16_t sineTable[TABLE_SIZE]; // empty wavetable

Osc aOsc1(sineTable);
Osc aOsc2(sineTable);
Bob lpf;
FX reverb;

float modIndex, ratio;

// Define 12S pins
#define I2S_BCLK       23   // Bit clock //
#define I2S_LRC        25   // Left Right / WS Clock
#define I2S_DOUT       26  
#define I2S_DIN        27
#define I2S_MCLKPIN     0   // MCLK

WM8978 wm8978;

void I2S_Init() {

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_NUM_BUF,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = true
  };
    
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_DIN                                        
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);  // install the driver 
  i2s_set_pin(I2S_NUM_0, &pin_config);  // port & pin config 
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1); // MCLK on GPIO0
  REG_WRITE(PIN_CTRL, 0xFFFFFFF0);

  i2s_set_sample_rates(I2S_NUM_0, SAMPLE_RATE); // set sample rate used for I2S TX and RX

}

static void audio_task(void *userData) {

  while(1) {

    for (int i=0; i < DMA_BUF_LEN; i++) {

      int16_t left, right;
      int16_t sample = lpf.next(aOsc1.phMod(aOsc2.next(), modIndex));
      reverb.reverbStereo(sample, sample, left, right);
      out_buf[i*2] = left;
      out_buf[i*2+1] = right;

    }

    size_t i2s_bytes_write = 0;
    i2s_write(I2S_NUM_0, out_buf, sizeof(out_buf), &i2s_bytes_write, portMAX_DELAY);

  }

}

void setup() {
  
  srand(time(NULL));
  
  I2S_Init();
  
  wm8978.init();            // WM8978 codec initialisation
  wm8978.addaCfg(1,1);      // enable the adc and the dac
  wm8978.sampleRate(1);     // set the sample rate of the codec 
  wm8978.inputCfg(0,0,0);   // input config
  wm8978.outputCfg(1,0);    // output config
  wm8978.spkVolSet(50);     // speaker volume
  wm8978.hpVolSet(25,25);   // headphone volume
  wm8978.i2sCfg(2,0);       // I2S format MSB, 16Bit

  i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);

  Osc::sinGen(sineTable);
  reverb.setReverbSize(16);
  reverb.setReverbLength(0.9f);

  ratio = 0.5f;

  xTaskCreate(audio_task, "audio", 1024, NULL, configMAX_PRIORITIES - 1, NULL);

}

void loop() {

  float pitch = 24 + rand(48);
  aOsc1.setPitch(pitch);
  aOsc2.setFreq(mtof(pitch) * ratio);
  modIndex = 1.0f + chaosRand(24.0f);
  lpf.setFreq(100 + rand(1400));
  lpf.setRes(chaosRand(1.0f));

  int tempo = 60000 / BPM;
  delay(tempo / 4);

}