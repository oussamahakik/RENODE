#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <stdio.h>
#include <string.h>

/* Appelee automatiquement a chaque paquet recu */
static void reception(const void *data, uint16_t len,
                      const linkaddr_t *src, const linkaddr_t *dest)
{
  char message[32];
  if(len > sizeof(message) - 1) {
    len = sizeof(message) - 1;
  }
  memcpy(message, data, len);
  message[len] = '\0';

  printf("[RECEPTEUR] recu : %s (de %02x:%02x)\n",
         message, src->u8[LINKADDR_SIZE - 2], src->u8[LINKADDR_SIZE - 1]);
}

PROCESS(receiver_process, "Recepteur");
AUTOSTART_PROCESSES(&receiver_process);

PROCESS_THREAD(receiver_process, ev, data)
{
  PROCESS_BEGIN();
  nullnet_set_input_callback(reception);
  printf("[RECEPTEUR] pret, en attente...\n");
  PROCESS_END();
}