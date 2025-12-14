#include "mr24hpc_types.h"
#include <stdint.h>
#include <stdbool.h>
#include "mr24hpc_port.h"

// Interni state parsera
typedef struct {
    uint8_t buffer[256];
    mr24hpc_state_t state; // trenutni "parsed" state
} mr24hpc_parser_t;

// Interni API drivera
void mr24hpc_parser_feed(uint8_t byte);
void mr24hpc_update_state(const mr24hpc_state_t *new_state);

// Mutex za thread-safe access
extern void mr24hpc_lock(void);
extern void mr24hpc_unlock(void);
