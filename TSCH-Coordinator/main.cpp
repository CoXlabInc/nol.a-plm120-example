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

  tsch->begin(SX1280, 0x0001, 0x0001, nullptr, true);
  // tsch->setHoppingSequence(
  //   0, //macHoppingSequenceID
  //   sizeof(tsch->HoppingSequenceList2450MHz),
  //   tsch->HoppingSequenceList2450MHz,
  //   tsch->ConvertChToFreq2450MHz
  // );

  tsch->onSendDone([](TSCHMac &, IEEE802_15_4Frame *frame) {
  });

  tsch->onReceive([](TSCHMac &, const IEEE802_15_4Frame *frame) {
    printf("- Received a frame(%p) from ", frame);
    IEEE802_15_4Address src = frame->getSrcAddr();
    if (src.len == 2) {
      printf("0x%04X\n", src.id.s16);
    } else if (src.len == 8) {
      printf(
        "%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
        src.id.s64[0], src.id.s64[1], src.id.s64[2], src.id.s64[3],
        src.id.s64[4], src.id.s64[5], src.id.s64[6], src.id.s64[7]
      );
    } else {
      printf("(unknown address length:%u)\n", src.len);
    }
    printf(" ");
    for (uint16_t i = 0; i < frame->getPayloadLength(); i++) {
      printf(" %02X", frame->getPayloadAt(i));
    }
    printf(" (%u byte)\n", frame->getPayloadLength());
    printf("  RSSI:%d\n", frame->power);
  });

  err = tsch->setSlotframe(20);
  if (err != ERROR_SUCCESS) {
    printf("- setSlotframe error:%d\n", err);
    while(1);
  }

  /* Common links: [0:beacon, 1:shared Tx] */
  tsch->setLink(0, 0, TSCHMac::LINK_TRANSMIT, IEEE802_15_4Address(0xFFFF), true);
  tsch->setLink(1, 0, TSCHMac::LINK_SHARED | TSCHMac::LINK_RECEIVE, IEEE802_15_4Address(0xFFFF), false);

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
    printf("Send EB! (%d)\n", err);
  }, nullptr);
  timerBeacon.startPeriodic(10000);

  tsch->onEBRReceive = [](
      TSCHMac &,
      IEEE802_15_4Address addr,
      IEEE802_15_4Frame::EnhancedBeaconRequestInfo_t &info
    ) {
    printf("- EBR received from ");
    if (addr.len == 2) {
      printf("0x%04X\n", addr.id.s16);
    } else if (addr.len == 8) {
      printf(
        "%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
        addr.id.s64[0], addr.id.s64[1], addr.id.s64[2], addr.id.s64[3],
        addr.id.s64[4], addr.id.s64[5], addr.id.s64[6], addr.id.s64[7]
      );
    }
    printf(
      "  permitJoiningOn:%d, linkQualityFilter:%d, percentFilter:%d\n",
      info.permitJoiningOn, info.linkQualityFilter, info.percentFilter
    );

    if (info.permitJoiningOn && (info.attributeIDs & IEEE802_15_4Frame::ATTRIBUTE_ID_TSCH_ENABLED)) {
      tsch->sendEnhancedBeacon(addr, true);
    }
  };
}
