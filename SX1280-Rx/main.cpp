#include <cox.h>

extern "C" int digitalReadInternal(int8_t);

void setup() {
  Serial.begin(115200);
  printf("\n*** [PLM120] SX1280 Rx ***\n");

  error_t err = SX1280.begin();
  printf("* Initialization result:%d\n", err);

  SX1280.setChannel(2400000000ul);
  SX1280.setLoRaMode(LoRa2GHzFrame::SF12, LoRa2GHzFrame::BW_400kHz, LoRa2GHzFrame::CR_4_5);

  SX1280.onRxStarted([](void *) {
    printf("* Rx started\n");
  }, nullptr);

  SX1280.onRxDone([](void *) {

    while (!SX1280.bufferIsEmpty()) {
      LoRa2GHzFrame frame(255);
      if (SX1280.readFrame(&frame) != ERROR_SUCCESS) {
        printf("* readFrame error\n");
      }

      static uint16_t success = 0;

      printf("Rx is done!: RSSI:%d dB, SNR:%d, CRC:%s, Length:%u, (",
             frame.power,
             frame.snr,
             frame.result == RadioPacket::SUCCESS ? "OK" : "FAIL",
             frame.len);
      uint16_t i;
      for (i = 0; i < frame.len; i++)
        printf("%02X ", frame.buf[i]);
      printf("\b), # of Rx:%u\n", (frame.result == RadioPacket::SUCCESS) ? ++success : success);
    }
  }, nullptr);
}
