CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

# Cible par défaut : la carte émulée dans Renode (cc2538.repl).
TARGET ?= cc2538dk

# Racine de Contiki-NG. Par défaut, ce dépôt est placé dans contiki-ng/examples/.
# Sinon : make CONTIKI=/chemin/vers/contiki-ng
CONTIKI ?= ../../..
include $(CONTIKI)/Makefile.include
