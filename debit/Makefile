CONTIKI_PROJECT = emetteur recepteur
all: $(CONTIKI_PROJECT)

MAKE_NET = MAKE_NET_NULLNET
TARGET ?= zoul
BOARD ?= firefly

# Reglages du test (voir emetteur.c) :
#   UNICAST=0 broadcast, sans accuse de reception (par defaut)
#   UNICAST=1 unicast, avec accuse de reception et reemissions
#   TAILLE    octets de donnees par trame (5 a 100)
#   NB        nombre de trames envoyees
# Exemple : gmake UNICAST=1 TAILLE=50 NB=500 BUILD_DIR=build/u1_t50
# Changer un reglage => utiliser un autre BUILD_DIR (ou faire "gmake clean"),
# sinon les fichiers deja compiles avec l'ancien reglage seraient reutilises.
UNICAST ?= 0
TAILLE ?= 100
NB ?= 1000
CFLAGS += -DUNICAST=$(UNICAST) -DTAILLE=$(TAILLE) -DNB_PAQUETS=$(NB)

# Logs de la couche CSMA, comme pour le ping-pong
ifeq ($(CSMA_LOGS),1)
  CFLAGS += -DLOG_CONF_LEVEL_MAC=LOG_LEVEL_INFO
endif

CONTIKI ?= ../../..
include $(CONTIKI)/Makefile.include
