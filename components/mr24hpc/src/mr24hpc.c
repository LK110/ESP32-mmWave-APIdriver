
#include <stdint.h>

static void mr24hpc_task(void *arg)
{
    uint8_t byte;

    while (1) {
        if (mr24hpc_port_uart_read(&byte, 1, 100) == 1) {
            mr24hpc_parser_feed(byte);
        }
    }
}
