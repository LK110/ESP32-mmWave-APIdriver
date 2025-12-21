#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool presence;          // osoba prisutna / ne
    uint8_t motion_state;   // stanje kretanja
    float distance_m;       // udaljenost u metrima
    float speed_m_s;        // brzina u m/s
    uint8_t body_signals;   // bazični signal parametri
    uint32_t last_update_ms; // timestamp posljednje izmjene
    uint32_t valid_mask;    // maska validnosti podataka (ostavljen prostor za proširenje)
} mr24hpc_state_t;

typedef enum {
    MR24HPC_VALID_PRESENCE      = (1 << 0), // 00001
    MR24HPC_VALID_MOTION_STATE  = (1 << 1), // 00010
    MR24HPC_VALID_DISTANCE      = (1 << 2), // 00100
    MR24HPC_VALID_SPEED         = (1 << 3), // 01000
    MR24HPC_VALID_BODY_SIGNALS  = (1 << 4), // 10000
} mr24hpc_valid_mask_t;