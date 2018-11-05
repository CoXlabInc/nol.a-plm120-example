#include <cox.h>

extern "C" int digitalReadInternal(int8_t);

Timer timerSend;
LoRa2GHzFrame *frame = nullptr;
uint32_t txCount = 0;

void setup() {
  Serial.begin(115200);
  printf("\n*** [PLM120] SX1280 Tx ***\n");

  error_t err = SX1280.begin();
  printf("* Initialization result:%d\n", err);


  timerSend.onFired([](void *) {
    if (frame) {
      printf("* Sending in progress...\n");
      return;
    }

    frame = new LoRa2GHzFrame(4);
    if (!frame || !frame->buf) {
      printf("* Not enough memory to make a frame.\n");
      return;
    }

    frame->sf = frame->SF12;
    frame->cr = frame->CR_4_5;
    frame->bw = frame->BW_400kHz;
    frame->setPreambleLength(8, 0);
    frame->useHeader = true;
    frame->useCrc = true;
    frame->invertIQ = false;
    frame->buf[0] = (txCount >> 0) & 0xFF;
    frame->buf[1] = (txCount >> 8) & 0xFF;
    frame->buf[2] = (txCount >> 16) & 0xFF;
    frame->buf[3] = (txCount >> 24) & 0xFF;
    txCount++;

    SX1280.transmit(frame);
  }, nullptr);
  timerSend.startPeriodic(1000);

  SX1280.onTxDone([](void *, bool success) {
    printf("* Tx done: %s\n", (success) ? "success" : "fail");
    delete frame;
    frame = nullptr;
  }, nullptr);
}
