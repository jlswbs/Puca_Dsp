// PÚCA_DSP - Faust dsp 2op fm synth //

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fmsyn.h"
#include "WM8978.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/timer.h"

float randomf(float minf, float maxf) {return minf + (esp_random()%(1UL << 31))*(maxf - minf) / (1UL << 31);}

#define SAMPLE_RATE 48000
#define NO_SAMPLES  32

WM8978 wm8978;

TaskHandle_t update_parameters_task;
SemaphoreHandle_t xSemaphore = NULL;
SemaphoreHandle_t ySemaphore = NULL;

fmsyn* DSP = new fmsyn(SAMPLE_RATE, NO_SAMPLES);

float freq, feed, shape, lfo, gain, gate, enva, envd, envs, envr;

void update_parameters (void *pvParameters) {
  for (;;) {
    for (uint32_t i = 0; i < NO_SAMPLES; i++) {
      DSP->setParamValue("freq", freq);
      DSP->setParamValue("feed", feed);
      DSP->setParamValue("shape", shape);
      DSP->setParamValue("lfo", lfo);
      DSP->setParamValue("enva", enva);
      DSP->setParamValue("envd", envd);
      DSP->setParamValue("envs", envs);
      DSP->setParamValue("envr", envr);
      DSP->setParamValue("gain", gain);
      DSP->setParamValue("gate", gate);
      vTaskDelay(pdMS_TO_TICKS(10));

      TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
      TIMERG0.wdt_feed=1;
      TIMERG0.wdt_wprotect=0;
    } 
  }
 
  if( xSemaphore != NULL && ySemaphore != NULL) {
    if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) && xSemaphoreTake( ySemaphore, portMAX_DELAY)) {
      
      TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
      TIMERG0.wdt_feed=1;
      TIMERG0.wdt_wprotect=0;
    
      xSemaphoreGive( xSemaphore );
      xSemaphoreGive( ySemaphore );

    } else {}
  }
}

void setup() {

  srand(time(NULL));

  wm8978.init();
  wm8978.addaCfg(1,1); 
  wm8978.inputCfg(0,0,0);     
  wm8978.outputCfg(1,0); 
  wm8978.sampleRate(0); 
  wm8978.micGain(30);
  wm8978.auxGain(0);
  wm8978.lineinGain(0);
  wm8978.spkVolSet(50);
  wm8978.hpVolSet(50,50);
  wm8978.i2sCfg(2,0);
  
  vTaskDelay(pdMS_TO_TICKS(100));
  xTaskCreatePinnedToCore(update_parameters, "update_parameters_task", 2048, NULL, 6, &update_parameters_task, 0); 
  
  DSP->start(); 
  vTaskDelay(pdMS_TO_TICKS(50));

  lfo = 0.1f;
  feed = 0.1f;
  enva = 0.01f;
  envd = 0.5f;
  envs = 0.5f;
  envr = 1.0f;

}

void loop() {

  gain = randomf(0.01f, 0.3f);
  shape = randomf(0.25f, 1.25f);
  freq = 50 + esp_random() % 2000;
  gate = 1.0f;

  vTaskDelay(pdMS_TO_TICKS(120));

  gate = 0.0f;
  
  vTaskDelay(pdMS_TO_TICKS(120));

}