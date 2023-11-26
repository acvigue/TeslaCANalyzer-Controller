#ifndef PACKET_PROCESSING_H_
#define PACKET_PROCESSING_H_

#include <esp_central.h>
#include <stdio.h>

// rx
const uint8_t RESP_START = 0xF5;
const uint8_t RESP_LOCK_RESPONSE_ERROR = 0x01;
const uint8_t RESP_MUTE_BUTTON_PRESS = 0x80;        // d=1
const uint8_t RESP_GPS_EQUIPPED_RESPONSE = 0x81;    // d=1
const uint8_t RESP_ALERT_RESPONSE = 0x82;           // d=0 (no alerts) or d=4*n (n chained alerts)
const uint8_t RESP_BRIGHTNESS_RESPONSE = 0x83;      // d=2
const uint8_t RESP_LOCK_RESPONSE = 0x84;            // d=1
const uint8_t RESP_DISPLAY_LENGTH_RESPONSE = 0x85;  // d=1
const uint8_t RESP_UNLOCK_RESPONSE = 0x86;          // d=1
const uint8_t RESP_MARKED_LOCATION = 0x87;          // d=13
const uint8_t RESP_ERROR_RESPONSE = 0x88;
const uint8_t RESP_MODEL_NUMBER_RESPONSE = 0x89;
const uint8_t RESP_SETTINGS_RESPONSE = 0x8A;  // d>=2 F5038A 0105  or   F5278A 0105 0204 0400 0608 1B04 0703 0803 0900 0A00 0D04 0E00 0F00 1000 1300
                                              // 1400 1500 1601 1700 1A01 (setting no + cur value)
const uint8_t RESP_SETTINGS_INFORMATION_RESPONSE = 0x8B;  // d>=2 F5038B nnF4 (unsupported setting) F5068B 16 00010203 (values for setting 16)
const uint8_t RESP_BAND_ENABLES_RESPONSE = 0x8C;          // d=5
const uint8_t RESP_BAND_ENABLES_DEFAULTS_RESPONSE = 0x8D;
const uint8_t RESP_FLASH_USAGE_RESPONSE = 0x8E;
const uint8_t RESP_BAND_ENABLES_SUPPORTED_RESPONSE = 0x8F;  // d=5
const uint8_t RESP_FLASH_ERASE_RESPONSE = 0x90;             // d=1 (0x00 = flash erased OK, otherwise error)
const uint8_t RESP_SHIFTER_STATUS_RESPONSE = 0x91;
const uint8_t RESP_VERSION_RESPONSE = 0x92;            // d>=7
const uint8_t RESP_SETTINGS_DEFAULTS_RESPONSE = 0x93;  // F50393 0100 (00 is default for setting 01)
const uint8_t RESP_FIRMWARE_UPDATE_STATUS = 0x94;      // d=2
const uint8_t RESP_MODEL_INFO_RESPONSE = 0x95;         // d>=1 f50a95 00 4d6178203336300 (00=escort, 01=Beltronics, 02=Cincinnati Microwave) + Name
const uint8_t RESP_AUTOLOCK_EVENT = 0x96;              // d=0 F50196
const uint8_t RESP_MARKER_ENABLES_RESPONSE = 0x97;     // d=2
const uint8_t RESP_BUTTON_PRESS_REPORT = 0x98;
const uint8_t RESP_STATUS_RESPONSE = 0x99;  // d>=1 f50299 0B | 0BA9 | 03A9 | 0BA92F52000008 | 0BA9073C090200 | 0BA9073C292300
const uint8_t RESP_MARKER_ENABLES_DEFAULTS_RESPONSE = 0x9A;
const uint8_t RESP_MARKER_ENABLES_SUPPORTED_RESPONSE = 0x9B;  // d=2
const uint8_t RESP_REPORT_BUTTON_PRESS = 0x9C;                // d=1
const uint8_t RESP_MODEL_NUMBER_REQUEST = 0x9D;
const uint8_t RESP_MUTE_RESPONSE = 0x9E;
const uint8_t RESP_POWER_DETECTOR_ON_OFF_RESPONSE = 0x9F;
const uint8_t RESP_UPDATE_APPROVAL_REQUEST = 0xA0;              // d=3
const uint8_t RESP_BLUETOOTH_PROTOCOL_UNLOCK_REQUEST = 0xA1;    // d=10
const uint8_t RESP_BLUETOOTH_PROTOCOL_UNLOCK_RESPONSE = 0xA2;   // d=10
const uint8_t RESP_BLUETOOTH_PROTOCOL_UNLOCK_STATUS = 0xA3;     // d=1
const uint8_t RESP_BLUETOOTH_CONNECTION_DELAY_RESPONSE = 0xA4;  // d=2
const uint8_t RESP_BLUETOOTH_SERIAL_NUMBER_RESPONSE = 0xA5;     // d=8
const uint8_t RESP_SPEED_INFORMATION_REQUEST = 0xA6;            // d=1 (data: 01=speed limit, 02=current speed)
const uint8_t RESP_OVERSPEED_WARNING_REQUEST = 0xA7;            // d=1
const uint8_t RESP_DISPLAY_CAPABILITIES_RESPONSE = 0xA8;  // d=1 (bits set: 0=beep supported, 1=display msg supported, 2=display location supported
const uint8_t RESP_ALERT_RESPONSE_FRONT_REAR = 0xA9;      // d=0 (no alets) or d=5*n (n chained alerts)
const uint8_t RESP_BAND_DIRECTION_RESPONSE = 0xAA;
const uint8_t RESP_SHIFTER_TBD = 0xB6;          // d=1
const uint8_t RESP_UNSUPPORTED_REQUEST = 0xF0;  // d=1
const uint8_t RESP_UNSUPPORTED_SETTING = 0xF4;  // response to SETTINGS_INFORMATION_RESPONSE
const uint8_t RESP_UNKNOWN_SETTING = 0xF3;      // response to SETTINGS_CHANGE request for invalid setting code
const uint8_t RESP_INVALID_RESPONSE_ERROR = 0xFF;

// tx
const uint8_t REQ_START = 0xF5;                 // d == data length:
const uint8_t REQ_LOCK_REQUEST = 0x80;          // d=0 current alert lockout
const uint8_t REQ_UNLOCK_REQUEST = 0x81;        // d=0 current alert unlock
const uint8_t REQ_SETTINGS_INFORMATION = 0x82;  // d=1
const uint8_t REQ_SETTING_CHANGE = 0x83;        // d=2 (num,new_val)
const uint8_t REQ_SETTINGS_REQUEST = 0x84;      // d=1 (0x00 ?)
const uint8_t REQ_BAND_ENABLES_SET = 0x85;      // d=5
const uint8_t REQ_BAND_ENABLES_REQUEST = 0x86;  // d=0
const uint8_t REQ_FLASH_UTILIZATION = 0x87;
const uint8_t REQ_FLASH_ERASE = 0x88;      // d=0
const uint8_t REQ_VERSION_REQUEST = 0x89;  // d=0
const uint8_t REQ_SHIFT_STATUS_REQUEST = 0x8A;
const uint8_t REQ_SHIFTER_CONTROL = 0x8B;
const uint8_t REQ_BAND_ENABLES_SUPPORTED = 0x8C;    // d=0
const uint8_t REQ_BAND_ENABLES_DEFAULT = 0x8D;      // d=0
const uint8_t REQ_BRIGHTNESS = 0x8E;                // (guessed - d = 0 or d = 1)
const uint8_t REQ_SETTINGS_DEFAULTS = 0x8F;         // d=1 REQ_(setting number)
const uint8_t REQ_MARKED_LOCATION = 0x90;           // d=13
const uint8_t REQ_MODEL_INFO_REQUEST = 0x91;        // d=0
const uint8_t REQ_MARKER_ENABLES_REQUEST = 0x92;    // d=0
const uint8_t REQ_MARKER_ENABLES_SET = 0x93;        // d=2
const uint8_t REQ_STATUS_REQUEST = 0x94;            // d=0
const uint8_t REQ_MARKER_ENABLES_SUPPORTED = 0x95;  // d=0
const uint8_t REQ_MARKER_ENABLES_DEFAULTS = 0x96;   // d=0
const uint8_t REQ_GPS_EQUIPPED_REQUEST = 0x97;      // d=0
const uint8_t REQ_LOCKOUT_LIST = 0x98;              // d=5 (127, 127, 127, 127, 11) == when auto lockout enabled
                                        // only valid if GPS-equipped DOES NOTHING ON 360  // or (0, 0, 0, 0, 0) == clear all lockout DB
const uint8_t REQ_DISPLAY_LENGTH = 0x99;   // d=0
const uint8_t REQ_DISPLAY_MESSAGE = 0x9A;  // d=XX (see below) or d=2(0x00,0x00 = clear screen)
                                           //  F5 9A XX [message bytes]
const uint8_t REQ_PLAY_TONE = 0x9B;  // d=0,1,2
// 9C -- TBD BOOLEAN, data = 1(0,1)
const uint8_t REQ_MODEL_NUMBER_RESPONSE = 0x9D;  // d=1(0x0=acknowledge) or d=4 (0x01,'0',modelnum1,modelnum2)
const uint8_t REQ_MUTE = 0x9E;                   // d=1 (01/00 mute/unmute)
const uint8_t REQ_POWER_DETECTOR_ON_OFF = 0x9F;
const uint8_t REQ_MODEL_NUMBER_REQUEST = 0xA0;      // d=0
const uint8_t REQ_UPDATE_APPROVAL_RESPONSE = 0xA1;  // d=1 [1=accept, 0=acknowledge, 2=deny]
const uint8_t REQ_LIVE_ALERT = 0xA2;
const uint8_t REQ_BLUETOOTH_PROTOCOL_UNLOCK_REQUEST = 0xA3;   // d=10
const uint8_t REQ_BLUETOOTH_PROTOCOL_UNLOCK_RESPONSE = 0xA4;  // d=10
const uint8_t REQ_BLUETOOTH_PROTOCOL_UNLOCK_STATUS = 0xA5;    // d=1
const uint8_t REQ_BLUETOOTH_CONNECTION_DELAY_REQUEST = 0xA6;  // d=0
const uint8_t REQ_BLUETOOTH_CONNECTION_DELAY_SET = 0xA7;      // d=2 (delay is 2-REQ_word, LSB first)
const uint8_t REQ_BLUETOOTH_SERIAL_NUMBER_REQUEST = 0xA8;     // d=0
const uint8_t REQ_SPEED_LIMIT_UPDATE = 0xA9;                  // d=2
const uint8_t REQ_ACTUAL_SPEED_UPDATE = 0xAA;                 // d=2
const uint8_t REQ_OVERSPEED_LIMIT_DATA = 0xAB;                // d=1 (64 + [0,1,5,7,10,15]) 0 == off, 1 = spd limit, 5-15 = 5-15 over
const uint8_t REQ_DISPLAY_CAPABILITIES = 0xAC;                // d=0
const uint8_t REQ_DISPLAY_LOCATION = 0xAD;                    // d=5
const uint8_t REQ_DISPLAY_CLEAR_LOCATION = 0xAE;              // d=0
const uint8_t REQ_COMM_INIT = 0xC0;                           // d=1 C002XX xx = 16=(bt full o
typedef struct RadarPacket {
    uint8_t commandByte;
    size_t len;
    uint8_t *buf;
} RadarPacket;

bool validateInboxItem(bleQueueItem inboxItem);
void processPacket(RadarPacket packet, QueueHandle_t queue);
int headingCalculation(int currentBearing, int targetBearing);
uint8_t *getDisplayLocationPacket(uint8_t alertType, uint32_t distanceFt, uint32_t heading);

#endif