/*
 * Test de debit - RECEPTEUR
 *
 * Tant que l'emetteur n'a rien envoye, envoie chaque seconde un message "R"
 * (pret) en broadcast. Ensuite, compte les trames recues :
 *   - chaque seconde : nombre de trames recues pendant la seconde ;
 *   - a la fin (2 secondes sans rien recevoir) : bilan avec le debit mesure
 *     entre la premiere et la derniere trame (horloge rtimer).
 *
 * Le debit affiche est le debit UTILE : seules les donnees (TAILLE octets par
 * trame) sont comptees, pas les en-tetes radio.
 */
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include <string.h>

static uint32_t recus = 0, recus_seconde = 0, octets = 0, seq_max = 0;
static int premier_recu = 0;
static rtimer_clock_t t_premier, t_dernier;
static uint8_t pret = 'R';

static void reception(const void *data, uint16_t len,
                      const linkaddr_t *src, const linkaddr_t *dest)
{
  const uint8_t *d = data;
  uint32_t seq;
  rtimer_clock_t maintenant = RTIMER_NOW();

  if(len < 5 || d[0] != 'D') {
    return;
  }
  memcpy(&seq, &d[1], sizeof(seq));
  if(!premier_recu) {
    premier_recu = 1;
    t_premier = maintenant;
  }
  t_dernier = maintenant;
  recus++;
  recus_seconde++;
  octets += len;
  if(seq + 1 > seq_max) {
    seq_max = seq + 1;               /* nombre de trames envoyees, vu d'ici */
  }
}

static void bilan(void)
{
  uint64_t us = (uint64_t)(t_dernier - t_premier) * 1000000 / RTIMER_SECOND;
  uint32_t taille = recus ? octets / recus : 0;
  uint64_t pps10 = 0, kbps10 = 0;    /* valeurs x10, pour une decimale */

  if(us > 0 && recus > 1) {
    /* entre la premiere et la derniere trame, il y a (recus - 1) intervalles */
    pps10 = (uint64_t)(recus - 1) * 10000000 / us;
    kbps10 = (uint64_t)(recus - 1) * taille * 8 * 10000 / us;
  }
  printf("=== BILAN RECEPTEUR : %lu recues, %lu perdues, %lu octets par trame, "
         "duree = %lu ms, %lu.%lu trames/s, debit utile = %lu.%lu kbit/s ===\n",
         (unsigned long)recus, (unsigned long)(seq_max - recus),
         (unsigned long)taille, (unsigned long)(us / 1000),
         (unsigned long)(pps10 / 10), (unsigned long)(pps10 % 10),
         (unsigned long)(kbps10 / 10), (unsigned long)(kbps10 % 10));
}

PROCESS(recepteur_process, "Recepteur");
AUTOSTART_PROCESSES(&recepteur_process);

PROCESS_THREAD(recepteur_process, ev, data)
{
  static struct etimer timer;
  static unsigned seconde = 0, inactif = 0;
  static int fini = 0;
  PROCESS_BEGIN();

  nullnet_set_input_callback(reception);
  printf("RECEPTEUR pret\n");
  etimer_set(&timer, CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);

    if(!premier_recu) {
      nullnet_buf = &pret;           /* "je suis pret" */
      nullnet_len = 1;
      NETSTACK_NETWORK.output(NULL);
    } else if(recus_seconde > 0) {
      seconde++;
      printf("SECONDE %u : %lu trames\n", seconde, (unsigned long)recus_seconde);
      recus_seconde = 0;
      inactif = 0;
    } else if(!fini && ++inactif >= 2) {
      bilan();
      fini = 1;
    }
  }

  PROCESS_END();
}
