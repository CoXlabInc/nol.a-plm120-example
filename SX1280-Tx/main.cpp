#include <cox.h>

// #define TEST_LORA
#define TEST_FLRC

Timer timerSend;
RadioPacket *frame = nullptr;
uint32_t txCount = 0;
struct timeval tTxStart, tTxStarted;

static void prepareLoRa2GHzFrame(LoRa2GHzFrame *f) {
#if 0
  f->sf = LoRa2GHzFrame::SF_UNSPECIFIED; //To use current settings.
#else
  f->sf = LoRa2GHzFrame::SF12;
  f->cr = LoRa2GHzFrame::CR_4_5;
  f->bw = LoRa2GHzFrame::BW_400kHz;
  f->setPreambleLength(3, 0);
  f->useHeader = true;
  f->useCrc = true;
  f->invertIQ = false;
#endif
}

static void prepareFLRCFrame(FLRCFrame *f) {
#if 1
  f->br = FLRCFrame::BR_UNSPECIFIED; //To use current settings.
#else
  f->br = FLRCFrame::BR_260_BW_300;
  f->cr = FLRCFrame::CR_1_2;
  f->ms = FLRCFrame::BT_1_0;
  f->preambleLengthBits = 32;
  f->useSyncword = true;
  f->syncwordRxMatch = FLRCFrame::SYNCWORD_1;
  f->useHeader = true;
  f->crcBytes = 3;
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(GPIO3, OUTPUT);
  digitalWrite(GPIO3, LOW);

  printf("\n*** [PLM120] SX1280 Tx ***\n");

  error_t err = SX1280.begin();
  printf("* Initialization result:%d\n", err);

  SX1280.setChannel(2400000000ul);
#ifdef TEST_LORA
  SX1280.setLoRaMode(LoRa2GHzFrame::SF12, LoRa2GHzFrame::BW_400kHz, LoRa2GHzFrame::CR_4_5);
#elif defined TEST_FLRC
  SX1280.setFLRCMode(
    FLRCFrame::BR_1300_BW_1200, //bitrate
    FLRCFrame::CR_1_2,          //coding rate
    FLRCFrame::BT_1_0,          //modulation shaping
    32,                         //preamble length (byte)
    true,                       //use syncword
    FLRCFrame::SYNCWORD_1,      //syncword rx match
    true,                       //use header
    3                           //CRC bytes
  );
  SX1280.setSyncword(1, (const uint8_t[]) { 0xDD, 0xA0, 0x96, 0x69 });
#endif

  timerSend.onFired([](void *) {
    if (frame) {
      printf("* Sending in progress...\n");
      return;
    }

    // if (!frame || !frame->buf) {
    //   printf("* Not enough memory to make a frame.\n");
    //   return;
    // }
    //
    // printf("* len:%u\n", frame->len);

#if 0
    frame = new RadioPacket(6);
#else

#ifdef TEST_LORA
    frame = new LoRa2GHzFrame(6);
    prepareLoRa2GHzFrame((LoRa2GHzFrame *) frame);
#elif defined TEST_FLRC
    frame = new FLRCFrame(127);
    prepareFLRCFrame((FLRCFrame *) frame);
#endif

#endif
    frame->power = 13;
    frame->buf[0] = (txCount >> 0) & 0xFF;
    frame->buf[1] = (txCount >> 8) & 0xFF;
    frame->buf[2] = (txCount >> 16) & 0xFF;
    frame->buf[3] = (txCount >> 24) & 0xFF;
    frame->buf[4] = 0;
    frame->buf[5] = 0;
    txCount++;

    gettimeofday(&tTxStart, nullptr);
    error_t err = SX1280.transmit(frame);
    gettimeofday(&tTxStarted, nullptr);

    if (err != ERROR_SUCCESS) {
      printf("* Error:%d\n", err);
      delete frame;
      frame = nullptr;
    } else {
      digitalWrite(GPIO3, HIGH);

      struct timeval tDiff;
      timersub(&tTxStarted, &tTxStart, &tDiff);

      printf(
        "[%lu.%06lu] Tx started (d:%lu.%06lu)\n",
        (uint32_t) tTxStarted.tv_sec, tTxStarted.tv_usec, (uint32_t) tDiff.tv_sec, tDiff.tv_usec
      );
    }
  }, nullptr);
  timerSend.startPeriodic(1000);

  SX1280.onTxDone = [](void *, bool success, GPIOInterruptInfo_t *intrInfo) {
    digitalWrite(GPIO3, LOW);

    struct timeval tDiff;
    timersub(&intrInfo->timeEnteredISR, &tTxStarted, &tDiff);

    printf(
      "[%lu.%06lu] Tx done: %s (duration: %lu.%06lu)\n",
      (uint32_t) intrInfo->timeEnteredISR.tv_sec, intrInfo->timeEnteredISR.tv_usec,
      (success) ? "success" : "fail",
      (uint32_t) tDiff.tv_sec, tDiff.tv_usec
    );

    delete frame;
    frame = nullptr;
  };
}
