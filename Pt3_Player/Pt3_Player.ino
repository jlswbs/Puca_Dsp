// PÚCA_DSP - ZX-Spectrum Pt3 player //

#include "driver/i2s.h"
#include "driver/gpio.h"
#include "WM8978.h"
#include "ay_emu.h"
#include "acidgoa.h"

#define AY_CLOCK      1773400
#define FRAME_RATE    50
#define SAMPLE_RATE   44100
#define DMA_BUF_LEN   32
#define DMA_NUM_BUF   2

static short out_buf[DMA_BUF_LEN * 2];
volatile int16_t buff[2];

int interruptCnt;

struct PT3_Channel_Parameters
{
  unsigned short Address_In_Pattern, OrnamentPointer, SamplePointer, Ton;
  unsigned char Loop_Ornament_Position, Ornament_Length, Position_In_Ornament, Loop_Sample_Position, Sample_Length, Position_In_Sample, Volume, Number_Of_Notes_To_Skip, Note, Slide_To_Note, Amplitude;
  bool Envelope_Enabled, Enabled, SimpleGliss;
  short Current_Amplitude_Sliding, Current_Noise_Sliding, Current_Envelope_Sliding, Ton_Slide_Count, Current_OnOff, OnOff_Delay, OffOn_Delay, Ton_Slide_Delay, Current_Ton_Sliding, Ton_Accumulator, Ton_Slide_Step, Ton_Delta;
  signed char Note_Skip_Counter;
};

struct PT3_Parameters
{
  unsigned char Env_Base_lo;
  unsigned char Env_Base_hi;
  short Cur_Env_Slide, Env_Slide_Add;
  signed char Cur_Env_Delay, Env_Delay;
  unsigned char Noise_Base, Delay, AddToNoise, DelayCounter, CurrentPosition;
  int Version;
};

struct PT3_SongInfo
{
  PT3_Parameters PT3;
  PT3_Channel_Parameters PT3_A, PT3_B, PT3_C;
};

struct AYSongInfo
{
  unsigned char* module;
  unsigned char* module1;
  int module_len;
  PT3_SongInfo data;
  PT3_SongInfo data1;
  bool is_ts;

  AYChipStruct chip0;
  AYChipStruct chip1;
};

struct AYSongInfo AYInfo;

void ay_resetay(AYSongInfo* info, int chipnum)
{
  if (!chipnum) ay_init(&info->chip0); else ay_init(&info->chip1);
}

void ay_writeay(AYSongInfo* info, int reg, int val, int chipnum)
{
  if (!chipnum) ay_out(&info->chip0, reg, val); else ay_out(&info->chip1, reg, val);
}

#include "PT3Play.h"

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

    uint32_t out_l, out_r;
        
    for (int i=0; i < DMA_BUF_LEN; i++) {

      if (interruptCnt++ >= (SAMPLE_RATE / FRAME_RATE)) {

        PT3_Play_Chip(AYInfo, 0);
        interruptCnt = 0;

      }

      ay_tick(&AYInfo.chip0, (AY_CLOCK / SAMPLE_RATE / 8));

      out_l = AYInfo.chip0.out[0] + AYInfo.chip0.out[1] / 2;
      out_r = AYInfo.chip0.out[2] + AYInfo.chip0.out[1] / 2;

      if (AYInfo.is_ts){

        ay_tick(&AYInfo.chip1, (AY_CLOCK / SAMPLE_RATE / 8));
        out_l += AYInfo.chip0.out[0] + AYInfo.chip0.out[1] / 2;
        out_r += AYInfo.chip0.out[2] + AYInfo.chip0.out[1] / 2;

      }

      if (out_l > 32767) out_l = 32767;
      if (out_r > 32767) out_r = 32767;
      out_buf[i*2] = out_l;
      out_buf[i*2+1] = out_r;

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

  memset(&AYInfo, 0, sizeof(AYInfo));

  ay_init(&AYInfo.chip0);
  ay_init(&AYInfo.chip1);

  AYInfo.module = music_data;
  AYInfo.module_len = music_data_size;

  PT3_Init(AYInfo);

  xTaskCreate(audio_task, "audio", 1024, NULL, configMAX_PRIORITIES - 1, NULL);

}

void loop() {

}