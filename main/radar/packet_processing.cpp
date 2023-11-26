#include "packet_processing.h"

#include <esp_central.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "escort-jni.hpp"

#define TAG "packet_processing"
#include <math.h>

bool validateInboxItem(bleQueueItem inboxItem) {
    if (inboxItem.data[0] != RESP_START || inboxItem.data_length - 2 != inboxItem.data[1]) {
        return false;
    }
    return true;
}

int headingCalculation(int currentBearing, int targetBearing) {
    int d;
    if (currentBearing <= targetBearing) {
        int i = targetBearing - currentBearing;
        if (abs(i) <= 180) {
            d = i;
            if (d < 0) {
                d += 360;
            }
            if (d < 0) {
                d += 360;
            }
            return (d / 2.0);
        }
        targetBearing -= 360;
    }
    d = targetBearing - currentBearing;

    return floor(d / 2.0);
}

uint8_t *getDisplayLocationPacket(uint8_t alertType, uint32_t distanceFt, uint32_t heading) {
    uint8_t *arr = (uint8_t *)malloc(5);
    int i = heading & 255;
    arr[0] = (alertType & 0xFF);
    arr[1] = ((((alertType & 128) >> 7) | (distanceFt << 1)) & 127);
    arr[2] = ((distanceFt >> 6) & 127);
    arr[3] = (((distanceFt >> 13) | (i << 3)) & 127);
    arr[4] = ((((0 & 3) << 5) | (i >> 4) | ((1 & 1) << 4)) & 127);
    return arr;
}

void processPacket(RadarPacket packet, QueueHandle_t queue) {
    bleQueueItem outgoingPacket;
    if (packet.commandByte == RESP_BLUETOOTH_PROTOCOL_UNLOCK_REQUEST) {
        ESP_LOGI(TAG, "Unlocking detector");
        uint32_t *out = (uint32_t *)malloc(2);
        esc_pack(packet.buf, out);
        uint32_t *enc = (uint32_t *)malloc(2);
        xtea_encrypt(35, out, SMARTCORD_KEY, enc);
        uint8_t *unpack = (uint8_t *)malloc(10);
        esc_unpack(enc, unpack);
        uint8_t authResponsePacket[13] = {REQ_START, 0x0b,      REQ_BLUETOOTH_PROTOCOL_UNLOCK_RESPONSE,
                                          unpack[0], unpack[1], unpack[2],
                                          unpack[3], unpack[4], unpack[5],
                                          unpack[6], unpack[7], unpack[8],
                                          unpack[9]};
        free(out);
        free(enc);
        free(unpack);
        outgoingPacket.data = (uint8_t *)malloc(13);
        outgoingPacket.data_length = 13;
        memcpy(outgoingPacket.data, authResponsePacket, 13);
        xQueueSendToBack(queue, (void *)&outgoingPacket, portMAX_DELAY);
    } else if (packet.commandByte == RESP_OVERSPEED_WARNING_REQUEST) {
        // 15 over limit.
        outgoingPacket.data = (uint8_t *)malloc(4);
        uint8_t statusRequest[4] = {REQ_START, 0x02, REQ_OVERSPEED_LIMIT_DATA, 79};
        memcpy(outgoingPacket.data, statusRequest, sizeof(statusRequest));
        outgoingPacket.data_length = sizeof(statusRequest);
        xQueueSendToBack(queue, (void *)&outgoingPacket, portMAX_DELAY);
    }

    free(packet.buf);
}