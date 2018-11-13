#include <cox.h>

// #define TEST_LORA
#define TEST_FLRC

static void printLoRaFrameInfo(LoRa2GHzFrame *frame) {
  printf(
    "Rx is done! RSSI:%d dB, SNR:%d",
    frame->power,
    frame->snr
  );
}

static void printFLRCFrameInfo(FLRCFrame *frame) {
  printf(
    "Rx is done! RSSI:%d dB",
    frame->power
  );
}

void setup() {
  Serial.begin(115200);
  printf("\n*** [PLM120] SX1280 Rx ***\n");

  error_t err = SX1280.begin();
  printf("* Initialization result:%d\n", err);

  SX1280.setChannel(2400000000ul);
#ifdef TEST_LORA
  printf("* Test LoRa mode\n");
  SX1280.setLoRaMode(LoRa2GHzFrame::SF12, LoRa2GHzFrame::BW_400kHz, LoRa2GHzFrame::CR_4_5);
#elif defined TEST_FLRC
  printf("* Test FLRC mode\n");
  SX1280.setFLRCMode(
    FLRCFrame::BR_260_BW_300,   //bitrate
    FLRCFrame::CR_1_2,          //coding rate
    FLRCFrame::BT_1_0,          //modulation shaping
    32,                         //preamble length (bits)
    true,                       //use syncword
    FLRCFrame::SYNCWORD_1,      //syncword rx match
    true,                       //use header
    3                           //CRC bytes
  );
  SX1280.setSyncword(1, (const uint8_t[]) { 0xDD, 0xA0, 0x96, 0x69 });
#else
#error "One of test mode should be defined."
#endif

  SX1280.onRxStarted([](void *) {
    printf("* Rx started\n");
  }, nullptr);

  SX1280.onRxDone([](void *) {
    Radio::Modulation_t mod = SX1280.getCurrentModulation();
    RadioPacket *frame = nullptr;

    while (!SX1280.bufferIsEmpty()) {
      switch(mod) {
        case Radio::MOD_LORA2G:
        frame = new LoRa2GHzFrame(255);
        break;

        case Radio::MOD_FLRC:
        frame = new FLRCFrame(255);
        break;

        default:
        printf("* SX1280 in unknown mode(%u)\n", mod);
        return;
      }

      error_t err = SX1280.readFrame(frame);
      if (err != ERROR_SUCCESS) {
        printf("* readFrame error:%d\n", err);
        SX1280.flushBuffer();
        delete frame;
        return;
      }

      static uint16_t success = 0;

      switch(mod) {
        case Radio::MOD_LORA2G:
        printLoRaFrameInfo((LoRa2GHzFrame *) frame);
        break;

        case Radio::MOD_FLRC:
        printFLRCFrameInfo((FLRCFrame *) frame);
        break;
      }

      printf(
        ", CRC:%s, Length:%u, (",
        frame->result == RadioPacket::SUCCESS ? "OK" : "FAIL",
        frame->len
      );

      uint16_t i;
      for (i = 0; i < frame->len; i++)
        printf("%02X ", frame->buf[i]);
      printf("\b), # of Rx:%u\n", (frame->result == RadioPacket::SUCCESS) ? ++success : success);

      delete frame;
    }
  }, nullptr);
}
