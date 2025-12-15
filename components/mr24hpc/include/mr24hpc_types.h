#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool presence;          // osoba prisutna / ne
    uint8_t motion_state;   // stanje kretanja
    float distance_m;       // udaljenost u metrima
    float speed_m_s;        // brzina u m/s
    uint8_t body_signals;   // bazični signal parametri
    uint32_t last_update_ms; // timestamp posljednje izmjene
} mr24hpc_state_t;
