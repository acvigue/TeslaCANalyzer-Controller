#ifndef ESCORT_JNI_HPP_
#define ESCORT_JNI_HPP_

#include <stdio.h>

uint32_t SMARTCORD_KEY[4] = {0xB67423AB, 0x7B7F599E, 0x831E63EB, 0x535C1285};

void xtea_encrypt(int rounds, uint32_t *v, uint32_t *key, uint32_t *out) {
    uint32_t v0 = v[0];
    uint32_t v1 = v[1];

    int sum = 0;
    const uint32_t delta = 0xf160a3d8;

    for (int i = 0; i < rounds; i++) {
        v0 += (((v1 << 4 ^ v1 >> 5) + v1) ^ (key[(sum & 0x3)] + sum));
        sum += delta;
        v1 += (((v0 << 4 ^ v0 >> 5) + v0) ^ (key[(sum >> 11 & 0x3)] + sum));
    }

    out[0] = v0;
    out[1] = v1;
}

void esc_pack(uint8_t *esc_req, uint32_t *out) {
    uint32_t v0 = (esc_req[0] & 0x7F) | (esc_req[1] & 0x7F) << 7 | (esc_req[2] & 0x7F) << 14 | (esc_req[3] & 0x7F) << 21 | (esc_req[4] & 0xF) << 28;
    uint32_t v1 = (esc_req[4] & 0x70) >> 4 | (esc_req[5] & 0x7F) << 3 | (esc_req[6] & 0x7F) << 10 | (esc_req[7] & 0x7F) << 17 |
                  (esc_req[8] & 0x7F) << 24 | (esc_req[9] & 0x1) << 31;

    out[0] = v0;
    out[1] = v1;
}

void esc_unpack(uint32_t *vv, uint8_t *out) {
    uint32_t v0 = vv[0];
    uint32_t v1 = vv[1];

    uint8_t b0 = (v0 & 0x7F);
    uint8_t b1 = (v0 >> 7 & 0x7F);
    uint8_t b2 = (v0 >> 14 & 0x7F);
    uint8_t b3 = (v0 >> 21 & 0x7F);
    uint8_t b4 = ((v0 >> 28) & 0x7f) | ((v1 << 4) & 0x70);
    uint8_t b5 = (v1 >> 3 & 0x7F);
    uint8_t b6 = (v1 >> 10 & 0x7F);
    uint8_t b7 = (v1 >> 17 & 0x7F);
    uint8_t b8 = (v1 >> 24 & 0x7F);
    uint8_t b9 = (v1 >> 31 & 0x1);

    out[0] = b0;
    out[1] = b1;
    out[2] = b2;
    out[3] = b3;
    out[4] = b4;
    out[5] = b5;
    out[6] = b6;
    out[7] = b7;
    out[8] = b8;
    out[9] = b9;
}
#endif