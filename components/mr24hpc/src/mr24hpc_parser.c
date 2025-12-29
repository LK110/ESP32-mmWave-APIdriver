#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "mr24hpc.h"
#include "mr24hpc_types.h"
#include "internal.h"

#define MR24_HEADER_1 0x53
#define MR24_HEADER_2 0x59

static mr24hpc_parser_t parser;

void mr24hpc_parser_init(void) {
    parser = (mr24hpc_parser_t){0};
    parser.ps = WAIT_H1;
}

void mr24hpc_parser_feed(uint8_t byte) {
    // TODO: provjeriti sto se dogada u slucaju probijanja buffer-a
    static uint8_t checksum_buf[256];
    static size_t checksum_len = 0;

    switch (parser.ps) {

    case WAIT_H1:
        if (byte == MR24_HEADER_1)
            parser.ps = WAIT_H2;
        break;

    case WAIT_H2:
        if (byte == MR24_HEADER_2)
            parser.ps = WAIT_CTRL;
        else
            parser.ps = WAIT_H1;
        break;

    case WAIT_CTRL:
        parser.ctrl = byte;
        checksum_len = 0;
        checksum_buf[checksum_len++] = byte;
        parser.ps = WAIT_CMD;
        break;

    case WAIT_CMD:
        parser.cmd = byte;
        checksum_buf[checksum_len++] = byte;
        parser.ps = WAIT_LEN1;
        break;

    case WAIT_LEN1:
        // MSB first (big-endian) — start fresh to avoid leftover bits
        parser.len = ((uint16_t)byte << 8) & 0xFF00;
        checksum_buf[checksum_len++] = byte;
        parser.ps = WAIT_LEN2;
        break;

    case WAIT_LEN2:
        // LSB — explicitly overwrite low byte
        parser.len = (parser.len & 0xFF00) | (uint16_t)byte;
        checksum_buf[checksum_len++] = byte;
        parser.data_idx = 0;
        if (parser.len == 0)
            parser.ps = WAIT_CHECKSUM;
        else if (parser.len <= (uint16_t)sizeof(parser.data))
            parser.ps = WAIT_DATA;
        else
            parser.ps = WAIT_H1; // invalid/overflow
        break;

    case WAIT_DATA:
        if (parser.data_idx < (uint16_t)sizeof(parser.data)) {
            parser.data[parser.data_idx++] = byte;
            checksum_buf[checksum_len++] = byte;
        } else {
            parser.ps = WAIT_H1;
            break;
        }

        if (parser.data_idx >= parser.len)
            parser.ps = WAIT_CHECKSUM;
        break;

    case WAIT_CHECKSUM:
        uint8_t cs = calculate_checksum(checksum_buf, checksum_len);
        if (cs == byte) {
            handle_frame(parser.ctrl, parser.cmd, parser.data, parser.len);
        }
        parser.ps = WAIT_H1;
        break;

    }
}

void handle_frame(uint8_t ctrl, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    // trebam samo report stanje senzora
    if (ctrl != 0x01) {
        return;
    }

    mr24hpc_state_t update = {0}; // novi update stanja, sva polja na 0

    switch (cmd) {

    /* ---------------- Presence report ----------------
     * CMD = 0x01
     * DATA[0]: 0x00 = nobody, 0x01 = somebody
     */
    case 0x01:
        if (len < 1) return;
        update.presence = data[0] ? true : false;
        update.valid_mask |= MR24HPC_VALID_PRESENCE;
        break;

    /* ---------------- Motion state ----------------
     * CMD = 0x02
     * DATA[0]: motion state enum (raw)
     */
    case 0x02:
        if (len < 1) return;
        update.motion_state = data[0];
        update.valid_mask |= MR24HPC_VALID_MOTION_STATE;
        break;

    /* ---------------- Distance ----------------
     * CMD = 0x03
     * DATA[0..1]: distance in mm (uint16)
     */
    case 0x03:
        if (len < 2) return;
        uint16_t mm = ((uint16_t)data[0] << 8) | data[1];
        update.distance_m = mm / 1000.0f;
        update.valid_mask |= MR24HPC_VALID_DISTANCE;
        break;

    /* ---------------- Speed ----------------
     * CMD = 0x04
     * DATA[0..1]: speed in mm/s (int16)
     */
    case 0x04:
        if (len < 2) return;
        int16_t speed_mm_s = ((int16_t)data[0] << 8) | data[1];
        update.speed_m_s = speed_mm_s / 1000.0f;
        update.valid_mask |= MR24HPC_VALID_SPEED;
        break;

    /* ---------------- Body signal ----------------
     * CMD = 0x05
     * DATA[0]: signal strength
     */
    case 0x05:
        if (len < 1) return;
        update.body_signals = data[0];
        update.valid_mask |= MR24HPC_VALID_BODY_SIGNALS;
        break;

    /* ---------------- Unknown ---------------- */
    default: return;
    }

    update.last_update_ms = mr24hpc_ms_since_last_update();
    mr24hpc_update_state(&update);
}


// ================ pomocne funkcije ================
uint8_t calculate_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (int i = 0; i < len; ++i) sum += data[i];
    return (uint8_t)(sum & 0xFF);
}