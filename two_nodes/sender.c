#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <stdio.h>
#include <string.h>

PROCESS(sender_process, "Emetteur");
AUTOSTART_PROCESSES(&sender_process);

PROCESS_THREAD(sender_process, ev, data)
{
  static struct etimer timer;
  static unsigned compteur = 0;
  static char message[32];

  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_SECOND * 2);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer)); // On attend que le timer soit expire

    snprintf(message, sizeof(message), "Bonjour #%u", compteur);

    // On donne le message a NullNet puis on l'envoie 
    nullnet_buf = (uint8_t *)message;
    nullnet_len = strlen(message) + 1;
    NETSTACK_NETWORK.output(NULL);   //

    printf("[EMETTEUR] envoye : %s\n", message);
    compteur++;
    etimer_reset(&timer);
  }
  PROCESS_END();
}