// PÚCA_DSP - FM polyphonic player + reverb //

#include "driver/i2s.h"
#include "driver/gpio.h"
#include "WM8978.h"
#include "playtune.h"
#include "wavetable.h"
#include "songdata.h"
#include "tuningwords.h"
#include "envelope.h"

#define SAMPLE_RATE   44100
#define DMA_BUF_LEN   32
#define DMA_NUM_BUF   2

static short out_buf[DMA_BUF_LEN * 2];
volatile int16_t buff[2];

const int FIXED_BITS = 14;
const int FIXED_SCALE = (1 << FIXED_BITS);
const int REVERB_BUFFER_SIZE_L = 0x4000;
const int REVERB_BUFFER_SIZE_R = 0x2000;
const int REVERB_LENGTH_L = (int)(REVERB_BUFFER_SIZE_L * 0.95);
const int REVERB_LENGTH_R = (int)(REVERB_BUFFER_SIZE_R * 0.91);
const int REVERB_DECAY = (int)(FIXED_SCALE * 0.7);

short reverbBufferL[REVERB_BUFFER_SIZE_L];
short reverbBufferR[REVERB_BUFFER_SIZE_R];
int reverbAddrL;
int reverbAddrR;

extern FMchannel ch[NUM_OF_CHANNELS];

  unsigned int timePlay = 0;
  unsigned int timePlayCount = 0;
  unsigned char isPlaying = 1;
  unsigned int songIndex = 0;
  float speed = 1.5f;

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

    isPlaying = 1;
        
    for (int i=0; i < DMA_BUF_LEN; i++) {
      
      int16_t sample = generateFModTask();

      int reverbL = ((int)reverbBufferR[reverbAddrR] * REVERB_DECAY) >> FIXED_BITS;
      int reverbR = ((int)reverbBufferL[reverbAddrL] * REVERB_DECAY) >> FIXED_BITS;
      reverbL += sample;
      reverbR += sample;
      reverbBufferL[reverbAddrL] = reverbL;
      reverbBufferR[reverbAddrR] = reverbR;
      reverbAddrL++;
      if (reverbAddrL > REVERB_LENGTH_L) reverbAddrL = 0;
      reverbAddrR++;
      if (reverbAddrR > REVERB_LENGTH_R) reverbAddrR = 0;
      int16_t outL = sample + (reverbBufferL[reverbAddrL]>>2);
      int16_t outR = sample + (reverbBufferR[reverbAddrR]>>2);

      out_buf[i*2] = outL;
      out_buf[i*2+1] = outR;

    }

    size_t i2s_bytes_write = 0;
    i2s_write(I2S_NUM_0, out_buf, sizeof(out_buf), &i2s_bytes_write, portMAX_DELAY);

    isPlaying = 0;

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

  for (int i = 0; i < NUM_OF_CHANNELS; i++){

    ch[i].setFreqMult_c(4.000f);
    ch[i].setFreqMult_m(1.000f);
    ch[i].setModMultiplier(4096);
    ch[i].setADSR(0.01f, 0.15f, 0.15f, 0.25f);

  }

  xTaskCreate(audio_task, "audio", 1024, NULL, configMAX_PRIORITIES - 1, NULL);

}

void loop() {

  if (isPlaying){

    if (timePlayCount > timePlay){

      timePlayCount = 0;
      updateNote(isPlaying, timePlay, timePlayCount, songIndex, speed);
    
    } else timePlayCount++;

    delay(1);

  }

}