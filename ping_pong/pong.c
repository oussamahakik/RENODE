#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"
#include <stdio.h>
#include <string.h>

typedef struct { char type; uint32_t seq; } msg_t;

static msg_t reponse;
static linkaddr_t adr_ping;

PROCESS(pong_process, "Pong");
AUTOSTART_PROCESSES(&pong_process);

/* Reception : on note qui a envoye, et on reveille le process pour repondre */
static void reception(const void *data, uint16_t len,
                      const linkaddr_t *src, const linkaddr_t *dest)
{
  msg_t m;
  if(len != sizeof(msg_t)) return;
  memcpy(&m, data, sizeof(m));

  if(m.type == 'P') {
    reponse.type = 'O';
    reponse.seq = m.seq;
    linkaddr_copy(&adr_ping, src);
    process_poll(&pong_process);
  }
}

PROCESS_THREAD(pong_process, ev, data)
{
  PROCESS_BEGIN();
  nullnet_set_input_callback(reception);
  printf("PONG pret\n");

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
    nullnet_buf = (uint8_t *)&reponse;
    nullnet_len = sizeof(reponse);
    NETSTACK_NETWORK.output(&adr_ping);              /* unicast vers ping */
    printf("PING %lu recu -> PONG envoye\n", (unsigned long)reponse.seq);
  }
  PROCESS_END();
}