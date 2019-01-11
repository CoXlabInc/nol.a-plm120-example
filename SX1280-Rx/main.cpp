#include <cox.h>

// #define TEST_LORA
#define TEST_FLRC

struct timeval tRxStarted;

static void printLoRaFrameInfo(LoRa2GHzFrame *frame) {
  printf(
    "* LoRa RSSI:%d dB, SNR:%d",
    frame->power,
    frame->snr
  );
}

static void printFLRCFrameInfo(FLRCFrame *frame) {
  printf(
    "* FLRC RSSI:%d dB",
    frame->power
  );
}

static void taskMeasureRssi(void *) {
  int16_t rssi = SX1280.getRssi();
  if (rssi > -77) {
    printf("* RSSI detected: %d\n", rssi);
  }
  postTask(taskMeasureRssi);
}

void setup() {
  Serial.begin(115200);
  pinMode(GPIO3, OUTPUT);
  digitalWrite(GPIO3, LOW);

  printf("\n*** [PLM120] SX1280 Rx ***\n");
  printf("* Reset: 0x%08lx\n", System.getResetReason());
  if (System.getResetReason() & (1ul << 1)) {
    const McuNRF51::StackDump *last = System.getLastStackDump();
    printf("* Watchdog reset. Check the last stack dump:\n");
    printf(" - R0: 0x%08lx, R1: 0x%08lx, R2: 0x%08lx, R3: 0x%08lx\n", last->r0, last->r1, last->r2, last->r3);
    printf(" - R12: 0x%08lx, LR: 0x%08lx, PC: 0x%08lx, PSR: 0x%08lx\n", last->r12, last->lr, last->pc, last->psr);
  }

  error_t err = SX1280.begin();
  printf("* Initialization result:%d\n", err);

  SX1280.setChannel(2400000000ul);
#ifdef TEST_LORA
  printf("* Test LoRa mode\n");
  SX1280.setLoRaMode(LoRa2GHzFrame::SF12, LoRa2GHzFrame::BW_400kHz, LoRa2GHzFrame::CR_4_5);
#elif defined TEST_FLRC
  printf("* Test FLRC mode\n");
  SX1280.setFLRCMode(
    FLRCFrame::BR_1300_BW_1200, //bitrate
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

  SX1280.onRxStarted = [](void *, GPIOInterruptInfo_t *intrInfo) {
    digitalWrite(GPIO3, HIGH);
    struct timeval now, tDiff;
    gettimeofday(&now, nullptr);
    timersub(&now, &intrInfo->timeEnteredISR, &tDiff);
    tRxStarted = intrInfo->timeEnteredISR;

    printf(
      "* [%lu.%06lu] Rx started at %lu.%06lu (d:%lu.%06lu)\n",
      now.tv_sec, now.tv_usec,
      intrInfo->timeEnteredISR.tv_sec, intrInfo->timeEnteredISR.tv_usec,
      tDiff.tv_sec, tDiff.tv_usec
    );
  };

  SX1280.onRxDone = [](void *, GPIOInterruptInfo_t *intrInfo) {
    digitalWrite(GPIO3, LOW);
    struct timeval now, tDiff, tDuration;
    gettimeofday(&now, nullptr);
    timersub(&now, &intrInfo->timeEnteredISR, &tDiff);
    timersub(&intrInfo->timeEnteredISR, &tRxStarted, &tDuration);

    printf(
      "* [%lu.%06lu] Rx done at %lu.%06lu (d:%lu.%06lu) => duration:%lu.%06lu\n",
      now.tv_sec, now.tv_usec,
      intrInfo->timeEnteredISR.tv_sec, intrInfo->timeEnteredISR.tv_usec,
      tDiff.tv_sec, tDiff.tv_usec, tDuration.tv_sec, tDuration.tv_usec
    );

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

        default:
        printf("* unknown modulation:%u\n", mod);
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
  };

  postTask(taskMeasureRssi);
}
