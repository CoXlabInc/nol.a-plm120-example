#include <cox.h>

extern "C" int digitalReadInternal(int8_t);

Timer timerHello;

void setup() {
  Serial.begin(115200);
  printf("\n*** [PLM120] SX1280 Tx ***\n");

  error_t err = SX1280.begin();
  printf("* Initialization result:%d\n", err);

  SX1280.SetRegulatorMode(SX1280.USE_DCDC);
  SX1280.SetStandby(SX1280.STDBY_RC);
  printf("* Current state:0x%02X\n", SX1280.GetOpMode());
  printf("* SX1280 firmware version:0x%04X\n", SX1280.GetFirmwareVersion());
}
