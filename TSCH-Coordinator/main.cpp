#include <cox.h>
#include <TSCHMac.hpp>

TSCHMac *tsch = nullptr;

void setup() {
  Serial.begin(115200);

  printf("\n*** [PLM120] IEEE 802.15.4 TSCH Coordinator ***\n");

  tsch = TSCHMac::Create();
  if (!tsch) {
    printf("- TSCHMac creation failed.\n");
    while(1);
  }

  tsch->begin(SX1280, 0x0001, 0x0001, nullptr, true);
  tsch->setHoppingSequence(
    0, //macHoppingSequenceID
    sizeof(tsch->HoppingSequenceList2450MHz),
    tsch->HoppingSequenceList2450MHz,
    tsch->ConvertChToFreq2450MHz
  );

  tsch->onSendDone([](TSCHMac &, IEEE802_15_4Frame *frame) {
  });

  tsch->onReceive([](TSCHMac &, const IEEE802_15_4Frame *frame) {
  });

  error_t err = tsch->setSlotframe(20);
  if (err != ERROR_SUCCESS) {
    printf("- setSlotframe error:%d\n", err);
    while(1);
  }

  /* Common links: [0:beacon, 1:shared Tx] */
 tsch->setLink(0, 0, TSCHMac::LINK_TRANSMIT, IEEE802_15_4Address(0xFFFF), true, true);
 tsch->setLink(1, 0, TSCHMac::LINK_SHARED | TSCHMac::LINK_RECEIVE, IEEE802_15_4Address(0xFFFF), false, true);

  /* Links for node '2': [2:2->1, 3:1->2] */
  tsch->setLink(2, 0, TSCHMac::LINK_RECEIVE, 2);
  tsch->setLink(3, 0, TSCHMac::LINK_TRANSMIT, 2);

  /* Links for node '3': [4:3->1, 5:1->3] */
  tsch->setLink(4, 0, TSCHMac::LINK_RECEIVE, 3);
  tsch->setLink(5, 0, TSCHMac::LINK_TRANSMIT, 3);

  /* Links for node '4': [6:4->1, 7:1->4] */
  tsch->setLink(6, 0, TSCHMac::LINK_RECEIVE, 4);
  tsch->setLink(7, 0, TSCHMac::LINK_TRANSMIT, 4);

  tsch->run();
  printf("- TSCH is running!!\n");

  static Timer timerBeacon;
  timerBeacon.onFired([](void *) {
    error_t err = tsch->sendEnhancedBeacon();
    printf("Send EB! (%d)", err);
  }, nullptr);
  timerBeacon.startPeriodic(1000);
}
