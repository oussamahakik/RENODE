# mes_tests

Expériences Contiki-NG sur une carte CC2538 émulée avec [Renode](https://renode.io).

## Organisation

```
common/cc2538-node.resc   config Renode d'un nœud CC2538 (partagée par tous les scénarios)
single_node/              scénario à un seul nœud
  node.c                  firmware (affiche un message toutes les 2 s)
  node.resc               lance le nœud dans Renode
  Makefile
```

## Prérequis

- Ce dépôt cloné dans `contiki-ng/examples/` (sinon, passer `CONTIKI=/chemin/vers/contiki-ng` à `make`)
- La chaîne `arm-none-eabi-gcc`
- Renode

## Compiler et lancer

```sh
cd single_node
make                 # TARGET=cc2538dk par défaut
renode node.resc
```

Sur macOS, le `make` du système (3.81) est trop ancien : utiliser `gmake` (`brew install make`).

Les variables `$name` et `$bin` de `node.resc` peuvent être redéfinies avant l'`include`.

## Problème connu : blocage au démarrage (CLOCK_STATUS)

Au démarrage, Contiki-NG configure l'horloge du CC2538 (oscillateur à quartz) et, comme sur la vraie
puce, attend qu'elle soit stable en lisant en boucle le registre `CLOCK_STATUS` (`0x400D2004`)
jusqu'à ce qu'un bit change.

Ce registre n'est pas modélisé dans `cc2538.repl` : Renode renvoyait toujours la même valeur
(`0x010004B0`), le bit ne changeait jamais et le firmware restait bloqué à `0x208C48`, sans jamais
atteindre le `printf` (fenêtre UART vide). Symptôme dans le terminal :

```
WARNING ... ReadDoubleWord from non existing peripheral at 0x400D2004
```

**Contournement** (dans `common/cc2538-node.resc`) : le périphérique Python `rolling-bit.py`, fourni
avec Renode, est placé à cette adresse. Il fait basculer un bit à chaque lecture, le firmware voit
l'horloge « stable » et continue son démarrage.
