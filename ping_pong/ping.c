#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include <string.h>

#define NB_PINGS   50              // nombre de pings a envoyer
#define PERIODE    (CLOCK_SECOND)   // periode entre deux pings (en ticks)

typedef struct { char type; uint32_t seq; } msg_t;  // 'P' = ping, 'O' = pong 

static msg_t msg;
static uint32_t seq = 0;
static rtimer_clock_t t_envoi;
static int attente = 0;             // 1 = on attend le pong courant
static unsigned recus = 0;
static linkaddr_t adr_pong;
static int adr_connue = 0;

static void reception(const void *data, uint16_t len,
                      const linkaddr_t *src, const linkaddr_t *dest)
{
  msg_t m;
  if(len != sizeof(msg_t)) return;
  memcpy(&m, data, sizeof(m));

  if(m.type == 'O' && attente && m.seq == seq) {
    rtimer_clock_t rtt = RTIMER_NOW() - t_envoi;
    unsigned long us = (unsigned long)((uint64_t)rtt * 1000000 / RTIMER_SECOND);
    printf("PONG %lu recu, RTT = %lu ticks = %lu us\n",
           (unsigned long)m.seq, (unsigned long)rtt, us);
    attente = 0;
    recus++;
    if(!adr_connue) {               /* on retient l'adresse de pong */
      linkaddr_copy(&adr_pong, src);
      adr_connue = 1;
    }
  }
}

PROCESS(ping_process, "Ping");
AUTOSTART_PROCESSES(&ping_process);

PROCESS_THREAD(ping_process, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();

  nullnet_set_input_callback(reception);
  etimer_set(&timer, PERIODE);

  while(seq < NB_PINGS) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);

    if(attente) {
      printf("PING %lu perdu (pas de reponse)\n", (unsigned long)seq);
    }

    seq++;
    msg.type = 'P';
    msg.seq = seq;
    nullnet_buf = (uint8_t *)&msg;
    nullnet_len = sizeof(msg);

    attente = 1;
    t_envoi = RTIMER_NOW();
    NETSTACK_NETWORK.output(adr_connue ? &adr_pong : NULL);  /* NULL = broadcast */
  }

  // laisser le temps au dernier pong d'arriver 
  etimer_set(&timer, PERIODE);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  printf("=== BILAN : %u envoyes, %u recus, %u perdus ===\n",
         NB_PINGS, recus, NB_PINGS - recus);
  PROCESS_END();
}