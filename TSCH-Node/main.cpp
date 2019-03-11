#include <cox.h>
#include <TSCHMac.hpp>

TSCHMac *tsch = nullptr;
static uint32_t seq;
uint16_t myPAN = 0xFFFF;

void setup() {
  Serial.begin(115200);

  printf("\n*** [PLM120] IEEE 802.15.4 TSCH Node ***\n");
  printf("- Reset: 0x%08lx\n", System.getResetReason());
  if (System.getResetReason() & (1ul << 1)) {
    const McuNRF51::StackDump *last = System.getLastStackDump();
    printf("  Watchdog reset. Check the last stack dump:\n");
    printf("  R0: 0x%08lx, R1: 0x%08lx, R2: 0x%08lx, R3: 0x%08lx\n", last->r0, last->r1, last->r2, last->r3);
    printf("  R12: 0x%08lx, LR: 0x%08lx, PC: 0x%08lx, PSR: 0x%08lx\n", last->r12, last->lr, last->pc, last->psr);
  }

  tsch = TSCHMac::Create();
  if (!tsch) {
    printf("- TSCHMac creation failed.\n");
    while(1);
  }

  error_t err = SX1280.begin();
  printf("- SX1280 initialization result:%d\n", err);

  SX1280.setChannel(2450000000ul);
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

  tsch->begin(SX1280, 0x0001, 0x0002);
  // tsch->setHoppingSequence(
  //   0, //macHoppingSequenceID
  //   sizeof(tsch->HoppingSequenceList2450MHz) / sizeof(uint16_t),
  //   tsch->HoppingSequenceList2450MHz,
  //   tsch->ConvertChToFreq2450MHz
  // );

  tsch->onSendDone([](TSCHMac &, IEEE802_15_4Frame *frame) {
    uint32_t sentSeq = (
      (uint32_t) frame->getPayloadAt(0)
      | ((uint32_t) frame->getPayloadAt(1) << 8)
      | ((uint32_t) frame->getPayloadAt(2) << 16)
      | ((uint32_t) frame->getPayloadAt(3) << 24)
    );
    printf(
      "- Send result:%d, seq:0x%08lX, # of Tx:%u, RSSI:%d\n",
      frame->result, sentSeq, frame->txCount, frame->power
    );
    delete frame;
  });

  tsch->onReceive([](TSCHMac &, const IEEE802_15_4Frame *frame) {
  });

  tsch->onTimesynced = [](TSCHMac &, IEEE802_15_4Address source) {
    return true;
  };

  tsch->run();
  printf("- TSCH is running!!\n");

  static Timer timerSend;
  timerSend.onFired([](void *) {
    if (tsch->isWorking()) {
      IEEE802_15_4Frame *f = new IEEE802_15_4Frame(4);
      if (!f) {
        printf("- Not enough memory\n");
        return;
      }

      IEEE802_15_4Address dstAddr(0x0001, myPAN);
      f->setDstAddr(dstAddr);

      f->setPayloadAt(0, (uint8_t) (seq >> 0));
      f->setPayloadAt(1, (uint8_t) (seq >> 8));
      f->setPayloadAt(2, (uint8_t) (seq >> 16));
      f->setPayloadAt(3, (uint8_t) (seq >> 24));
      // f->setPayloadLength(4);
      error_t err = tsch->send(f);
      if (err != ERROR_SUCCESS) {
        printf("- Send data fail: %d\n", err);
        delete f;
      } else {
        printf("- Send data: 0x%08lX\n", seq);
        seq++;
      }
    } else {
      error_t err = tsch->sendEnhancedBeaconRequest();
      printf("- Send EBR! (%d)\n", err);
    }
  }, nullptr);
  timerSend.startPeriodic(5000);

  tsch->onJoin = [](TSCHMac &, uint16_t panId) {
    printf("- Joined PAN 0x%04X\n", panId);
    myPAN = panId;
    return true;
  };
}
