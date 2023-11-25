#ifndef PACKET_PROCESSING_H_
#define PACKET_PROCESSING_H_

#include <esp_central.h>
#include <stdio.h>

typedef struct RadarPacket {
    uint8_t commandByte;
    size_t len;
    uint8_t *buf;
} RadarPacket;

bool validateInboxItem(bleQueueItem inboxItem);
void processPacket(RadarPacket packet, QueueHandle_t queue);

#endif