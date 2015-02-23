
#include "platform_config.h"
#include <stdio.h>

SDCardFAT fat;

static void setup();
static void spi_setup();
static void sdcard_setup();
static void loop();

int main(void) {
  setup();
  while (1) {
    loop();
  }
  return 0;
}

static void setup() {
  time_setup();
  debug_setup();
  spi_setup();
  sdcard_setup();
  printf("setup complete!\n");
}

static void spi_setup() {
#ifdef SPI1_ENABLE
  SPI_InitParams spiInit;

  SPI_initParamsInit(&spiInit);
  spiInit.halSpiInitParams.instance = SPI1;
  spiInit.halSpiInitParams.cpol = SPI_CPOL_low;
  spiInit.halSpiInitParams.cpha = SPI_CPHA_2Edge;
  SPI_init(&spiInit);
  printf("setup SPI1 complete\n");
#endif
}

static void sdcard_setup() {
  SDCard_initParamsInit(&fat.sdcard);
  fat.sdcard.initializeSpi = true;
  fat.sdcard.spiInstance = SDCARD_SPI;
  fat.sdcard.csPort = SDCARD_CS_PORT;
  fat.sdcard.csPin = SDCARD_CS_PIN;
  SDCard_init(&fat.sdcard);
  printf("setup sdcard complete\n");
}

static void loop() {

}

void assert_failed(uint8_t *file, uint32_t line) {
  while (1) {
    printf("assert_failed: %s:%lu\n", file, line);
  }
}
