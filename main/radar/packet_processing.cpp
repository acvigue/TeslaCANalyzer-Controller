#include "packet_processing.h"

#include <esp_central.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "escort-jni.hpp"

#define TAG "packet_processing"

const uint8_t PACKET_SEP = 0xF5;

bool validateInboxItem(bleQueueItem inboxItem) {
    if (inboxItem.data[0] != 0xF5 || inboxItem.data_length - 2 != inboxItem.data[1]) {
        return false;
    }
    return true;
}

void processPacket(RadarPacket packet, QueueHandle_t queue) {
    bleQueueItem outgoingPacket;
    if (packet.commandByte == 0xA1) {
        // Radar locked, unlock it with the static keys
        uint32_t *out = (uint32_t *)malloc(2);

        esc_pack(packet.buf, out);

        uint32_t *enc = (uint32_t *)malloc(2);
        xtea_encrypt(35, out, SMARTCORD_KEY, enc);

        uint8_t *unpack = (uint8_t *)malloc(10);
        esc_unpack(enc, unpack);

        uint8_t authResponsePacket[13] = {PACKET_SEP, 0x0b,      0xA4,      unpack[0], unpack[1], unpack[2], unpack[3],
                                          unpack[4],  unpack[5], unpack[6], unpack[7], unpack[8], unpack[9]};
        free(out);
        free(enc);
        free(unpack);
        outgoingPacket.data = (uint8_t *)malloc(13);
        outgoingPacket.data_length = 13;
        memcpy(outgoingPacket.data, authResponsePacket, 13);
        ESP_LOGI(TAG, "Unlocking detector");
        xQueueSendToBack(queue, (void *)&outgoingPacket, portMAX_DELAY);
    } else if (packet.commandByte == 0xA6) {
        if (packet.buf[0] == 0x01) {
            // speed limit

            outgoingPacket.data = (uint8_t *)malloc(4);
            uint8_t statusRequest[4] = {0xF5, 0x02, 86, 0};
            memcpy(outgoingPacket.data, statusRequest, sizeof(statusRequest));
            outgoingPacket.data_length = sizeof(statusRequest);
            xQueueSendToBack(queue, (void *)&outgoingPacket, portMAX_DELAY);
        } else if (packet.buf[0] == 0x02) {
            // current speed
        }
    }

    free(packet.buf);
}