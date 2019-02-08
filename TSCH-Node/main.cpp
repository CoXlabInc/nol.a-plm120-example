#include <cox.h>
#include <TSCHMac.hpp>

TSCHMac *tsch = nullptr;

void setup() {
  Serial.begin(115200);

  printf("\n*** [PLM120] IEEE 802.15.4 TSCH Node ***\n");

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
  //   sizeof(tsch->HoppingSequenceList2450MHz),
  //   tsch->HoppingSequenceList2450MHz,
  //   tsch->ConvertChToFreq2450MHz
  // );

  tsch->onSendDone([](TSCHMac &, IEEE802_15_4Frame *frame) {
  });

  tsch->onReceive([](TSCHMac &, const IEEE802_15_4Frame *frame) {
  });

  tsch->onTimesynced = [](TSCHMac &, IEEE802_15_4Address source) {
    return true;
  };

  tsch->run();
  printf("- TSCH is running!!\n");

  static Timer timerBeaconRequest;
  timerBeaconRequest.onFired([](void *) {
    if (!tsch->isWorking()) {
      error_t err = tsch->sendEnhancedBeaconRequest();
      printf("Send EBR! (%d)", err);
    }
  }, nullptr);
  timerBeaconRequest.startPeriodic(5000);

  tsch->onJoin = [](TSCHMac &, uint16_t panId) {
    printf("- Joined PAN 0x%04X\n",panId);
    return true;
  };


}
