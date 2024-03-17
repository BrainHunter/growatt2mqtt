#include "growattInterface.h"

growattIF::growattIF(int _PinMAX485_RE_NEG, int _PinMAX485_DE, int _PinMAX485_RX, int _PinMAX485_TX) {
  PinMAX485_RE_NEG = _PinMAX485_RE_NEG;
  PinMAX485_DE = _PinMAX485_DE;
  PinMAX485_RX = _PinMAX485_RX;
  PinMAX485_TX = _PinMAX485_TX;

  // Init outputs, RS485 in receive mode
  pinMode(PinMAX485_RE_NEG, OUTPUT);
  pinMode(PinMAX485_DE, OUTPUT);
  digitalWrite(PinMAX485_RE_NEG, 0);
  digitalWrite(PinMAX485_DE, 0);
}

void growattIF::initGrowatt() {
  #if defined(ESP8266)
    serial = new SoftwareSerial (PinMAX485_RX, PinMAX485_TX, false); //RX, TX
    serial->begin(MODBUS_RATE);
  #elif defined(ESP32)
    serial = &Serial2;
    serial->begin(MODBUS_RATE, SERIAL_8N1 , PinMAX485_RX ,PinMAX485_TX ); // RX , TX
  #endif
  
  
  growattInterface.begin(SLAVE_ID , *serial);

  static growattIF* obj = this;                               //pointer to the object
  // Callbacks allow us to configure the RS485 transceiver correctly
  growattInterface.preTransmission ([]() {                   //Set function pointer via anonymous Lambda function
    obj->preTransmission();
  });

  growattInterface.postTransmission([]() {                   //Set function pointer via anonymous Lambda function
    obj->postTransmission();
  });
}

uint8_t growattIF::writeRegister(uint16_t reg, uint16_t message) {
  return growattInterface.writeSingleRegister(reg, message);
}

uint16_t growattIF::readRegister(uint16_t reg) {
  growattInterface.readHoldingRegisters(reg, 1);
  return growattInterface.getResponseBuffer(0);				// returns 16bit
}

void growattIF::preTransmission() {
  digitalWrite(PinMAX485_RE_NEG, 1);
  digitalWrite(PinMAX485_DE, 1);
}

void growattIF::postTransmission() {
  digitalWrite(PinMAX485_RE_NEG, 0);
  digitalWrite(PinMAX485_DE, 0);
}

uint8_t growattIF::ReadInputRegisters(char* json) {
  uint8_t result;
#if defined(ESP8266)
  ESP.wdtDisable();     // diabling the watchdog is not nice. 
  result = growattInterface.readInputRegisters(setcounter * 64, 64);
  ESP.wdtEnable(1);
#else
  result = growattInterface.readInputRegisters(setcounter * 64, 64);
#endif
  if (result == growattInterface.ku8MBSuccess)   {
    if (setcounter == 0) {    //register 0-63
      // Status and PV data
      modbusdata.status = growattInterface.getResponseBuffer(0);
      modbusdata.solarpower = ((growattInterface.getResponseBuffer(1) << 16) | growattInterface.getResponseBuffer(2)) * 0.1;

      modbusdata.pv1voltage = growattInterface.getResponseBuffer(3) * 0.1;
      modbusdata.pv1current = growattInterface.getResponseBuffer(4) * 0.1;
      modbusdata.pv1power = ((growattInterface.getResponseBuffer(5) << 16) | growattInterface.getResponseBuffer(6)) * 0.1;

      modbusdata.pv2voltage = growattInterface.getResponseBuffer(7) * 0.1;
      modbusdata.pv2current = growattInterface.getResponseBuffer(8) * 0.1;
      modbusdata.pv2power = ((growattInterface.getResponseBuffer(9) << 16) | growattInterface.getResponseBuffer(10)) * 0.1;

      // Output
      modbusdata.outputpower = ((growattInterface.getResponseBuffer(35) << 16) | growattInterface.getResponseBuffer(36)) * 0.1;
      modbusdata.gridfrequency = growattInterface.getResponseBuffer(37) * 0.01;
      modbusdata.gridvoltage = growattInterface.getResponseBuffer(38) * 0.1;

      // Energy
      modbusdata.energytoday = ((growattInterface.getResponseBuffer(53) << 16) | growattInterface.getResponseBuffer(54)) * 0.1;
      modbusdata.energytotal = ((growattInterface.getResponseBuffer(55) << 16) | growattInterface.getResponseBuffer(56)) * 0.1;
      modbusdata.totalworktime = ((growattInterface.getResponseBuffer(57) << 16) | growattInterface.getResponseBuffer(58)) * 0.5;

      modbusdata.pv1energytoday = ((growattInterface.getResponseBuffer(59) << 16) | growattInterface.getResponseBuffer(60)) * 0.1;
      modbusdata.pv1energytotal = ((growattInterface.getResponseBuffer(61) << 16) | growattInterface.getResponseBuffer(62)) * 0.1;
      overflow = growattInterface.getResponseBuffer(63);
      setcounter ++;
      return Continue;
    }

    if (setcounter == 1) {    //register 64 -127
      modbusdata.pv2energytoday = ((overflow << 16) | growattInterface.getResponseBuffer(64 - 64)) * 0.1;
      modbusdata.pv2energytotal = ((growattInterface.getResponseBuffer(65 - 64) << 16) | growattInterface.getResponseBuffer(66 - 64)) * 0.1;

      // Temperatures
      modbusdata.tempinverter = growattInterface.getResponseBuffer(93 - 64) * 0.1;
      modbusdata.tempipm = growattInterface.getResponseBuffer(94 - 64) * 0.1;
      modbusdata.tempboost = growattInterface.getResponseBuffer(95 - 64) * 0.1;

      // Diag data
      modbusdata.ipf = growattInterface.getResponseBuffer(100 - 64);
      modbusdata.realoppercent = growattInterface.getResponseBuffer(101 - 64);
      modbusdata.opfullpower = ((growattInterface.getResponseBuffer(102 - 64) << 16) | growattInterface.getResponseBuffer(103 - 64)) * 0.1;
      modbusdata.deratingmode = growattInterface.getResponseBuffer(103 - 64);
      //  0:no derate;
      //  1:PV;
      //  2:*;
      //  3:Vac;
      //  4:Fac;
      //  5:Tboost;
      //  6:Tinv;
      //  7:Control;
      //  8:*;
      //  9:*OverBack
      //  ByTime;

      modbusdata.faultcode = growattInterface.getResponseBuffer(105 - 64);
      //  1~23 " Error: 99+x
      //  24 "Auto Test
      //  25 "No AC
      //  26 "PV Isolation Low",
      //  27 " Residual I
      //  28 " Output High
      //  29 " PV Voltage
      //  30 " AC V Outrange
      //  31 " AC F Outrange
      //  32 " Module Hot


      modbusdata.faultbitcode = ((growattInterface.getResponseBuffer(105 - 64) << 16) | growattInterface.getResponseBuffer(106 - 64));
      //  0x00000001 \
      //  0x00000002 Communication error
      //  0x00000004 \
      //  0x00000008 StrReverse or StrShort fault
      //  0x00000010 Model Init fault
      //  0x00000020 Grid Volt Sample diffirent
      //  0x00000040 ISO Sample diffirent
      //  0x00000080 GFCI Sample diffirent
      //  0x00000100 \
      //  0x00000200 \
      //  0x00000400 \
      //  0x00000800 \
      //  0x00001000 AFCI Fault
      //  0x00002000 \
      //  0x00004000 AFCI Module fault
      //  0x00008000 \
      //  0x00010000 \
      //  0x00020000 Relay check fault
      //  0x00040000 \
      //  0x00080000 \
      //  0x00100000 \
      //  0x00200000 Communication error
      //  0x00400000 Bus Voltage error
      //  0x00800000 AutoTest fail
      //  0x01000000 No Utility
      //  0x02000000 PV Isolation Low
      //  0x04000000 Residual I High
      //  0x08000000 Output High DCI
      //  0x10000000 PV Voltage high
      //  0x20000000 AC V Outrange
      //  0x40000000 AC F Outrange
      //  0x80000000 TempratureHigh

      modbusdata.warningbitcode = ((growattInterface.getResponseBuffer(110 - 64) << 16) | growattInterface.getResponseBuffer(111 - 64));
      //  0x0001 Fan warning
      //  0x0002 String communication abnormal
      //  0x0004 StrPIDconfig Warning
      //  0x0008 \
      //  0x0010 DSP and COM firmware unmatch
      //  0x0020 \
      //  0x0040 SPD abnormal
      //  0x0080 GND and N connect abnormal
      //  0x0100 PV1 or PV2 circuit short
      //  0x0200 PV1 or PV2 boost driver broken
      //  0x0400 \
      //  0x0800 \
      //  0x1000 \
      //  0x2000 \
      //  0x4000 \
      //  0x8000
      setcounter = 0;
    }
  } else {
    return result;
  }
  // Generate the modbus MQTT message
  sprintf(json, "{");
  sprintf(json + strlen(json), " \"status\":%d,",  modbusdata.status);
  sprintf(json + strlen(json), " \"solarpower\":%.1f,", modbusdata.solarpower);
  sprintf(json + strlen(json), " \"pv1voltage\":%.1f,", modbusdata.pv1voltage);
  sprintf(json + strlen(json), " \"pv1current\":%.1f,", modbusdata.pv1current);
  sprintf(json + strlen(json), " \"pv1power\":%.1f,", modbusdata.pv1power);
  sprintf(json + strlen(json), " \"pv2voltage\":%.1f,", modbusdata.pv2voltage);
  sprintf(json + strlen(json), " \"pv2current\":%.1f,", modbusdata.pv2current);
  sprintf(json + strlen(json), " \"pv2power\":%.1f,", modbusdata.pv2power);

  sprintf(json + strlen(json), " \"outputpower\":%.1f,", modbusdata.outputpower);
  sprintf(json + strlen(json), " \"gridfrequency\":%.2f,", modbusdata.gridfrequency);
  sprintf(json + strlen(json), " \"gridvoltage\":%.1f,", modbusdata.gridvoltage);

  sprintf(json + strlen(json), " \"energytoday\":%.1f,", modbusdata.energytoday);
  sprintf(json + strlen(json), " \"energytotal\":%.1f,",  modbusdata.energytotal);
  sprintf(json + strlen(json), " \"totalworktime\":%.1f,",  modbusdata.totalworktime);
  sprintf(json + strlen(json), " \"pv1energytoday\":%.1f,",  modbusdata.pv1energytoday);
  sprintf(json + strlen(json), " \"pv1energytotal\":%.1f,",  modbusdata.pv1energytotal);
  sprintf(json + strlen(json), " \"pv2energytoday\":%.1f,",  modbusdata.pv2energytoday);
  sprintf(json + strlen(json), " \"pv2energytotal\":%.1f,",  modbusdata.pv2energytotal);
  sprintf(json + strlen(json), " \"opfullpower\":%.1f,",  modbusdata.opfullpower);

  sprintf(json + strlen(json), " \"tempinverter\":%.1f,", modbusdata.tempinverter);
  sprintf(json + strlen(json), " \"tempipm\":%.1f,", modbusdata.tempipm);
  sprintf(json + strlen(json), " \"tempboost\":%.1f,", modbusdata.tempboost);

  sprintf(json + strlen(json), " \"ipf\":%d,", modbusdata.ipf);
  sprintf(json + strlen(json), " \"realoppercent\":%d,", modbusdata.realoppercent);
  sprintf(json + strlen(json), " \"deratingmode\":%d,", modbusdata.deratingmode);
  sprintf(json + strlen(json), " \"faultcode\":%d,", modbusdata.faultcode);
  sprintf(json + strlen(json), " \"faultbitcode\":%d,", modbusdata.faultbitcode);
  sprintf(json + strlen(json), " \"warningbitcode\":%d }", modbusdata.warningbitcode);
  return result;
}

uint8_t growattIF::ReadHoldingRegisters(char* json) {
  uint8_t result;

#if defined(ESP8266)
  ESP.wdtDisable();     // diabling the watchdog is not nice. 
  result = growattInterface.readHoldingRegisters(setcounter * 64, 64);
  ESP.wdtEnable(1);
#else
  result = growattInterface.readHoldingRegisters(setcounter * 64, 64);
#endif

  if (result == growattInterface.ku8MBSuccess)   {
    if (setcounter == 0) {      //register 0-63
      modbussettings.enable = growattInterface.getResponseBuffer(0);
      modbussettings.safetyfuncen = growattInterface.getResponseBuffer(1); // Safety Function Enabled
      //  Bit0: SPI enable
      //  Bit1: AutoTestStart
      //  Bit2: LVFRT enable
      //  Bit3: FreqDerating Enable
      //  Bit4: Softstart enable
      //  Bit5: DRMS enable
      //  Bit6: Power Volt Func Enable
      //  Bit7: HVFRT enable
      //  Bit8: ROCOF enable
      //  Bit9: Recover FreqDerating Mode Enable
      //  Bit10~15: Reserved
      modbussettings.maxoutputactivepp = growattInterface.getResponseBuffer(3); // Inverter M ax output active power percent  0-100: %, 255: not limited
      modbussettings.maxoutputreactivepp = growattInterface.getResponseBuffer(4); // Inverter M ax output reactive power percent  0-100: %, 255: not limited
      modbussettings.maxpower = ((growattInterface.getResponseBuffer(6) << 16) | growattInterface.getResponseBuffer(7)) * 0.1;
      modbussettings.voltnormal = growattInterface.getResponseBuffer(8) * 0.1;
      strncpy(modbussettings.firmware, "      ", 6);
      modbussettings.firmware[0] = growattInterface.getResponseBuffer(9) >> 8;
      modbussettings.firmware[1] = growattInterface.getResponseBuffer(9) & 0xff;
      modbussettings.firmware[2] = growattInterface.getResponseBuffer(10) >> 8;
      modbussettings.firmware[3] = growattInterface.getResponseBuffer(10) & 0xff;
      modbussettings.firmware[4] = growattInterface.getResponseBuffer(11) >> 8;
      modbussettings.firmware[5] = growattInterface.getResponseBuffer(11) & 0xff;

      strncpy(modbussettings.controlfirmware, "      ", 6);
      modbussettings.controlfirmware[0] = growattInterface.getResponseBuffer(12) >> 8;
      modbussettings.controlfirmware[1] = growattInterface.getResponseBuffer(12) & 0xff;
      modbussettings.controlfirmware[2] = growattInterface.getResponseBuffer(13) >> 8;
      modbussettings.controlfirmware[3] = growattInterface.getResponseBuffer(13) & 0xff;
      modbussettings.controlfirmware[4] = growattInterface.getResponseBuffer(14) >> 8;
      modbussettings.controlfirmware[5] = growattInterface.getResponseBuffer(14) & 0xff;

      modbussettings.startvoltage = growattInterface.getResponseBuffer(17) * 0.1;

      strncpy(modbussettings.serial, "          ", 10);
      modbussettings.serial[0] = growattInterface.getResponseBuffer(23) >> 8;
      modbussettings.serial[1] = growattInterface.getResponseBuffer(23) & 0xff;
      modbussettings.serial[2] = growattInterface.getResponseBuffer(24) >> 8;
      modbussettings.serial[3] = growattInterface.getResponseBuffer(24) & 0xff;
      modbussettings.serial[4] = growattInterface.getResponseBuffer(25) >> 8;
      modbussettings.serial[5] = growattInterface.getResponseBuffer(25) & 0xff;
      modbussettings.serial[6] = growattInterface.getResponseBuffer(26) >> 8;
      modbussettings.serial[7] = growattInterface.getResponseBuffer(26) & 0xff;
      modbussettings.serial[8] = growattInterface.getResponseBuffer(27) >> 8;
      modbussettings.serial[9] = growattInterface.getResponseBuffer(27) & 0xff;

      modbussettings.gridvoltlowlimit = growattInterface.getResponseBuffer(52) * 0.1;
      modbussettings.gridvolthighlimit = growattInterface.getResponseBuffer(53) * 0.1;
      modbussettings.gridfreqlowlimit = growattInterface.getResponseBuffer(54) * 0.01;
      modbussettings.gridfreqhighlimit = growattInterface.getResponseBuffer(55) * 0.01;
      setcounter ++;
      return Continue;
    }
    if (setcounter == 1) {      //register 64 -127
      modbussettings.gridvoltlowconnlimit = growattInterface.getResponseBuffer(64 - 64) * 0.1;
      modbussettings.gridvolthighconnlimit = growattInterface.getResponseBuffer(65 - 64) * 0.1;
      modbussettings.gridfreqlowconnlimit = growattInterface.getResponseBuffer(66 - 64) * 0.01;
      modbussettings.gridfreqhighconnlimit = growattInterface.getResponseBuffer(67 - 64) * 0.01;

      modbussettings.modul = growattInterface.getResponseBuffer(121 - 64);
      setcounter ++;
      return Continue;
    }
    if (setcounter == 2) {      //register 128-191
      //          //          modbussettings.modul = growattInterface.getResponseBuffer(130 - 128);
      setcounter = 0;
    }
  } else {
    return result;
  }
  // Generate the modbus MQTT message
  sprintf(json, "{");
  sprintf(json + strlen(json), " \"enable\":%d,", modbussettings.enable);
  sprintf(json + strlen(json), " \"safetyfuncen\":%d,", modbussettings.safetyfuncen);
  sprintf(json + strlen(json), " \"maxoutputactivepp\":%d,", modbussettings.maxoutputactivepp);
  sprintf(json + strlen(json), " \"maxoutputreactivepp\":%d,", modbussettings.maxoutputreactivepp);

  sprintf(json + strlen(json), " \"maxpower\":%.1f,", modbussettings.maxpower);
  sprintf(json + strlen(json), " \"voltnormal\":%.1f,", modbussettings.voltnormal);
  sprintf(json + strlen(json), " \"startvoltage\":%.1f,", modbussettings.startvoltage);
  sprintf(json + strlen(json), " \"gridvoltlowlimit\":%.1f,", modbussettings.gridvoltlowlimit);
  sprintf(json + strlen(json), " \"gridvolthighlimit\":%.1f,", modbussettings.gridvolthighlimit);
  sprintf(json + strlen(json), " \"gridfreqlowlimit\":%.1f,", modbussettings.gridfreqlowlimit);
  sprintf(json + strlen(json), " \"gridfreqhighlimit\":%.1f,", modbussettings.gridfreqhighlimit);
  sprintf(json + strlen(json), " \"gridvoltlowconnlimit\":%.1f,", modbussettings.gridvoltlowconnlimit);
  sprintf(json + strlen(json), " \"gridvolthighconnlimit\":%.1f,", modbussettings.gridvolthighconnlimit);
  sprintf(json + strlen(json), " \"gridfreqlowconnlimit\":%.1f,", modbussettings.gridfreqlowconnlimit);
  sprintf(json + strlen(json), " \"gridfreqhighconnlimit\":%.1f,", modbussettings.gridfreqhighconnlimit);

  sprintf(json + strlen(json), " \"firmware\":\"%s\",", modbussettings.firmware);
  sprintf(json + strlen(json), " \"controlfirmware\":\"%s\",", modbussettings.controlfirmware);
  sprintf(json + strlen(json), " \"serial\":\"%s\",", modbussettings.serial);
  sprintf(json + strlen(json), " \"modulPower\":\"%04X\" }", modbussettings.modul);
  return result;
}

String growattIF::sendModbusError(uint8_t result) {
  String message = "";
  if (result == growattInterface.ku8MBIllegalFunction) {
    message = "Illegal function";
  }
  if (result == growattInterface.ku8MBIllegalDataAddress) {
    message = "Illegal data address";
  }
  if (result == growattInterface.ku8MBIllegalDataValue) {
    message = "Illegal data value";
  }
  if (result == growattInterface.ku8MBSlaveDeviceFailure) {
    message = "Slave device failure";
  }
  if (result == growattInterface.ku8MBInvalidSlaveID) {
    message = "Invalid slave ID";
  }
  if (result == growattInterface.ku8MBInvalidFunction) {
    message = "Invalid function";
  }
  if (result == growattInterface.ku8MBResponseTimedOut) {
    message = "Response timed out";
  }
  if (result == growattInterface.ku8MBInvalidCRC) {
    message = "Invalid CRC";
  }
  if (message == "") {
    message = result;
  }
  return message;
}
