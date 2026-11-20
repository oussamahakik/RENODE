/*
 * Test de debit - EMETTEUR
 *
 * Envoie NB_PAQUETS trames de TAILLE octets, le plus vite possible :
 * des qu'une trame est terminee (envoyee, ou abandonnee par CSMA), la suivante
 * est confiee a CSMA. Il n'y a donc jamais plus d'une trame en attente.
 *
 * Avant de commencer, l'emetteur attend un message "R" (pret) du recepteur :
 * cela synchronise le depart et donne l'adresse du recepteur (mode unicast).
 *
 * Reglages (a la compilation, voir le Makefile) :
 *   UNICAST    0 = broadcast (pas d'accuse de reception), 1 = unicast (avec ACK)
 *   TAILLE     taille des donnees de chaque trame, en octets (5 a 100)
 *   NB_PAQUETS nombre de trames a envoyer
 */
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "net/mac/mac.h"
#include "net/linkaddr.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include <string.h>

#ifndef UNICAST
#define UNICAST 0
#endif
#ifndef TAILLE
#define TAILLE 100
#endif
#ifndef NB_PAQUETS
#define NB_PAQUETS 1000
#endif
#if TAILLE < 5 || TAILLE > 100
#error "TAILLE doit etre comprise entre 5 et 100 octets"
#endif

static uint8_t buf[TAILLE];          /* 'D' + numero (4 octets) + remplissage */
static uint32_t seq = 0;             /* numero de la prochaine trame */
static linkaddr_t adr_recepteur;
static int recepteur_connu = 0;

/* Bilan des envois, rempli par le callback de CSMA */
static uint32_t termines = 0, ok = 0, noack = 0, collision = 0, autres = 0;
static uint32_t transmissions = 0;   /* nombre total d'emissions radio */

PROCESS(emetteur_process, "Emetteur");
AUTOSTART_PROCESSES(&emetteur_process);

/* Message "R" du recepteur : il est pret, on retient son adresse */
static void reception(const void *data, uint16_t len,
                      const linkaddr_t *src, const linkaddr_t *dest)
{
  if(len == 1 && ((const uint8_t *)data)[0] == 'R' && !recepteur_connu) {
    linkaddr_copy(&adr_recepteur, src);
    recepteur_connu = 1;
    process_poll(&emetteur_process);
  }
}

/* Appele par CSMA quand une trame est terminee */
static void envoi_termine(void *ptr, int status, int nb_tx)
{
  termines++;
  transmissions += nb_tx;
  switch(status) {
  case MAC_TX_OK:        ok++;        break;
  case MAC_TX_NOACK:     noack++;     break;
  case MAC_TX_COLLISION: collision++; break;
  default:               autres++;    break;
  }
  process_poll(&emetteur_process);   /* reveille le processus : trame suivante */
}

/* Prepare une trame et la confie directement a CSMA (comme NullNet, mais avec
 * un callback pour savoir quand elle est terminee) */
static void envoyer(void)
{
  buf[0] = 'D';
  memcpy(&buf[1], &seq, sizeof(seq));
  seq++;

  packetbuf_clear();
  packetbuf_copyfrom(buf, TAILLE);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,
                     UNICAST ? &adr_recepteur : &linkaddr_null);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  NETSTACK_MAC.send(envoi_termine, NULL);
}

PROCESS_THREAD(emetteur_process, ev, data)
{
  static rtimer_clock_t t_debut;
  PROCESS_BEGIN();

  memset(buf, 0xAA, sizeof(buf));
  nullnet_set_input_callback(reception);
  printf("EMETTEUR : %s, %u octets par trame, %u trames\n",
         UNICAST ? "unicast" : "broadcast", TAILLE, (unsigned)NB_PAQUETS);
  printf("EMETTEUR : attente du recepteur...\n");

  PROCESS_WAIT_EVENT_UNTIL(recepteur_connu);
  printf("EMETTEUR : depart\n");

  t_debut = RTIMER_NOW();
  envoyer();
  while(termines < NB_PAQUETS) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
    if(termines == seq && seq < NB_PAQUETS) {
      envoyer();
    }
  }

  {
    uint64_t us = (uint64_t)(RTIMER_NOW() - t_debut) * 1000000 / RTIMER_SECOND;
    printf("=== BILAN EMETTEUR : %lu trames, %lu ok, %lu sans ACK, "
           "%lu canal occupe, %lu autres erreurs, %lu emissions radio, "
           "duree = %lu ms ===\n",
           (unsigned long)termines, (unsigned long)ok, (unsigned long)noack,
           (unsigned long)collision, (unsigned long)autres,
           (unsigned long)transmissions, (unsigned long)(us / 1000));
  }

  PROCESS_END();
}
