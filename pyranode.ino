//-*-C++-*-
// Photodiode or phototransistor from 19 to 18.

#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <FastCRC_tables.h>

#include "ADC.h"

ADC *adc = new ADC(); // adc object
FastCRC32 CRC32;
uint32_t CRC_BUF_BYTES = 3 * 4;

const uint32_t ADC_RESOLUTION = 12;
const uint32_t ADC_RANGE = 1 << ADC_RESOLUTION;
const uint32_t ADC_MAX = ADC_RANGE - 1;
const uint32_t ADC_MEDIAN_SAMPLES = 254;
const uint32_t ADC_INPUT_PIN = 16;
const uint32_t ADC_MEAN_SAMPLES = 4;

const uint32_t DIE_TEMPERATURE_PIN = 38;

// Perform nsamples calls to analogRead().
void getHistogram(int pin, uint32_t h[ADC_RANGE], uint32_t nsamples) {
  for (uint32_t i = 0; i < ADC_RANGE; ++i) {
    h[i] = 0;
  }
  for (uint32_t i = 0; i < nsamples; ++i) {
    ++h[adc->analogRead(pin, ADC_1)];
  }
}

// Array h is a histogram of values ranging from 0 to ADC_RANGE
// inclusive.
// Return the value at or just above the middle index in the sorted
// range of samples. Results are exact for odd sample sizes.
//
// N.B. Result has slight bias when nsamples is even because there's
// no code to average the values on either side of the non-integral
// midpoint of the range.
uint32_t getHistogramMedian(uint32_t h[ADC_RANGE], uint32_t nsamples) {
  uint32_t i = 0;
  uint32_t count = 0;
  uint32_t target = (nsamples + 1) / 2;
  while (count <= target && i < ADC_RANGE) {
    count += h[i];
    ++i;
  }
  return i;
}

void enablePullDown(int pin) {
  *portConfigRegister(pin) |= PORT_PCR_PE; //pull enable
  *portConfigRegister(pin) &= ~PORT_PCR_PS; //pull down
}

void setup() {
  Serial.begin(9600);
  pinMode(ADC_INPUT_PIN, INPUT);
  enablePullDown(ADC_INPUT_PIN);
  pinMode(17, OUTPUT);
  digitalWriteFast(17, 1); // Give the phototransistor something to modulate.

  adc->setReference(ADC_REF_3V3, ADC_0);
  adc->setReference(ADC_REF_3V3, ADC_1);
  adc->setConversionSpeed(ADC_ADACK_2_4);
  adc->setAveraging(1); // set number of averages
  adc->setResolution(ADC_RESOLUTION); // set bits of resolution

  // ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
  // see the documentation for more information
  adc->setConversionSpeed(ADC_VERY_LOW_SPEED ); // change the conversion speed
  // ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
  adc->setSamplingSpeed(ADC_VERY_LOW_SPEED); // change the sampling speed
  // with 16 averages, 12 bits resolution and ADC_HIGH_SPEED conversion and sampling
  // it takes about 32.5 us for a conversion

  //  adc->enablePGA(64, ADC_0);
  //  adc->enablePGA(64, ADC_1);
  delay(500);
}

const char* dstr =
  "________________________________________________________________";
uint32_t elapsed = 0;
const int sensePin = 17;
const uint32_t TX_MILLISECONDS = 1 * 1000;
elapsedMillis sinceTx(TX_MILLISECONDS);
void loop() {
  if (TX_MILLISECONDS <= sinceTx) {
    sinceTx = 0;
    
    uint32_t h[ADC_MAX];
    getHistogram(ADC_INPUT_PIN, h, ADC_MEDIAN_SAMPLES);
    uint32_t light = getHistogramMedian(h, ADC_MEDIAN_SAMPLES);
    for (uint32_t k = 0; k < ADC_MEAN_SAMPLES - 1; ++k) {
      getHistogram(ADC_INPUT_PIN, h, ADC_MEDIAN_SAMPLES);
      light += getHistogramMedian(h, ADC_MEDIAN_SAMPLES);
    }
    light /= ADC_MEAN_SAMPLES;
    getHistogram(DIE_TEMPERATURE_PIN, h, ADC_MEDIAN_SAMPLES);
    
    uint32_t temperature = getHistogramMedian(h, ADC_MEDIAN_SAMPLES);

    // Note: CRC_BUF_BYTES must align with the code below to fill crcbuf.
    uint8_t crcbuf[CRC_BUF_BYTES];

    uint32_t m = 1;
    crcbuf[CRC_BUF_BYTES - m++] = (elapsed & (0xFF << 24)) >> 24;
    crcbuf[CRC_BUF_BYTES - m++] = (elapsed & (0xFF << 16)) >> 16;
    crcbuf[CRC_BUF_BYTES - m++] = (elapsed & (0xFF <<  8)) >>  8;
    crcbuf[CRC_BUF_BYTES - m++] = (elapsed & (0xFF <<  0)) >>  0;

    crcbuf[CRC_BUF_BYTES - m++] = (light & (0xFF << 24)) >> 24;
    crcbuf[CRC_BUF_BYTES - m++] = (light & (0xFF << 16)) >> 16;
    crcbuf[CRC_BUF_BYTES - m++] = (light & (0xFF <<  8)) >>  8;
    crcbuf[CRC_BUF_BYTES - m++] = (light & (0xFF <<  0)) >>  0;

    crcbuf[CRC_BUF_BYTES - m++] = (temperature & (0xFF << 24)) >> 24;
    crcbuf[CRC_BUF_BYTES - m++] = (temperature & (0xFF << 16)) >> 16;
    crcbuf[CRC_BUF_BYTES - m++] = (temperature & (0xFF <<  8)) >>  8;
    crcbuf[CRC_BUF_BYTES - m++] = (temperature & (0xFF <<  0)) >>  0;

    uint32_t ck = CRC32.crc32(crcbuf, CRC_BUF_BYTES);

    Serial.printf("S\t%lu\t"
                  "L\t%lu\t"
                  "T\t%lu\t"
                  "C\t%lu\r\n", elapsed, light, temperature, ck);

    elapsed += TX_MILLISECONDS;
  }
}

// The Enphase PV data has 12 samples per hour or one sample every
// five minutes.

// A median filter on a five minute window results in significantly
// smoother graphs than Enphase.
