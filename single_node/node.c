#include "contiki.h"
#include <stdio.h>

PROCESS(mon_noeud_process, "Mon noeud");
AUTOSTART_PROCESSES(&mon_noeud_process);

PROCESS_THREAD(mon_noeud_process, ev, data) 
{
  static struct etimer timer; // déclaration du timer de l'événement
  static unsigned compteur = 0; // compteur de messages pour le suivi de l'exécution du processus

  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_SECOND * 2);  // initialisation du timer pour déclencher l'événement toutes les 2 secondes

  while(1) {
    printf("Bonjour de Oussama ! message #%u\n", compteur++);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }
  PROCESS_END();
}