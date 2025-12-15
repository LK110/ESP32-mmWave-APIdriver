#include <stdint.h>

#include "mr24hpc_types.h"
#include "internal.h"

mr24hpc_parser_t mr24hpc_parser;
int i = 0;

void mr24hpc_parser_init() {
    //TODO: inicijalizacija parsera
}

void mr24hpc_parser_feed(uint8_t byte) {
    mr24hpc_parser.buffer[i++] = byte;
    mr24hpc_state_t new_state;
    // TODO: parsiranje podataka iz buffer-a
}

void mr24hpc_update_state(const mr24hpc_state_t *new_state) {
    mr24hpc_state_t current_state = mr24hpc_parser.state;
    mr24hpc_parser.state = *new_state;
    // TODO: azuriranje stanja senzora
}