# Générateur de nombres aléatoires (RNG) de la CC2538, pour Renode.
#
# Renode ne simule pas ce bloc (registres ADC/RNG à 0x400D7000) : toute lecture
# renvoie 0. Contiki-NG tire alors toujours random_rand() = 0, et la couche CSMA
# n'attend jamais avant d'émettre (son délai d'attente au hasard vaut toujours 0).
#
# Ce petit périphérique imite le RNG :
#   - écrire dans RNDL (0x14) fait entrer un octet dans la graine
#     (Contiki y écrit une graine tirée de l'adresse du nœud -> chaque nœud a son propre tirage) ;
#   - mettre le bit RCTRL0 (0x04) dans ADCCON1 (0x00) fait avancer le générateur d'un pas ;
#   - lire RNDL (0x14) / RNDH (0x18) donne l'octet bas / haut de la valeur courante.
# Le générateur est un LFSR 16 bits (pas exactement celui de la vraie puce,
# mais il produit une suite pseudo-aléatoire du même genre).
#
# Utilisation (dans un .resc, machine sélectionnée) :
#   machine PyDevFromFile @chemin/cc2538-rng.py 0x400D7000 0x28 True "rng"

if request.IsInit:
    state = 0xACE1
    regs = {}

elif request.IsWrite:
    off = request.Offset
    val = request.Value
    if off == 0x14:                       # RNDL : on fait entrer un octet de graine
        state = ((state << 8) | (val & 0xFF)) & 0xFFFF
        if state == 0:
            state = 1
    elif off == 0x00:                     # ADCCON1
        if val & 0x04:                    # RCTRL0 = 1 : un pas du générateur
            lsb = state & 1
            state >>= 1
            if lsb:
                state ^= 0xB400           # LFSR de Galois 16 bits, période maximale
        regs[off] = val & ~0x04           # le bit retombe tout seul, comme sur la puce
    else:
        regs[off] = val

elif request.IsRead:
    off = request.Offset
    if off == 0x14:
        request.Value = state & 0xFF
    elif off == 0x18:
        request.Value = (state >> 8) & 0xFF
    else:
        request.Value = regs.get(off, 0)
