# Notes : les variables dans Renode, `mach clear` et le redémarrage des nœuds

## La règle de base

Quand on écrit `$bin = ...` ou `$id = ...` dans un script Renode, la valeur est rangée :

- **dans l'espace global** si **aucune machine n'est sélectionnée** ;
- **dans la machine sélectionnée** sinon (variable propre à cette machine).

Quand une machine lit une variable, elle regarde **d'abord chez elle**, puis **dans l'espace global**
si elle n'a rien trouvé.

Attention : `mach create` (caché dans `common/cc2538-node.resc`) **sélectionne automatiquement**
la machine qu'il crée, et elle reste sélectionnée ensuite.

>  La doc officielle confirme les deux points :
> - les variables sont « contextuelles » : *« for each machine you can have different variables
>   with the same name »* ([Monitor and script syntax][doc-syntax], section « Monitor variable types ») ;
> - `mach create` *« also switches the Monitor's context to this new machine »*
>   ([Working with machines][doc-machines], section « Creating machines »).

## Ce que fait `mach clear`

`mach clear` désélectionne la machine en cours. Après lui, plus aucune machine n'est sélectionnée.

Donc dans `two_nodes.resc`, après le `mach clear` qui suit l'émetteur :

- les variables définies **après** (`$bin = receiver`, `$id = 2`) vont dans l'**espace global** ;
- comme il y avait déjà un `$bin` et un `$id` dans l'espace global (ceux du début), les nouvelles
  valeurs **les remplacent** (même nom → on écrase) :

| Espace global | avant `mach clear` | après `$bin`/`$id` du 2e nœud |
|---|---|---|
| `$bin` | `sender` | `receiver` |
| `$id` | `1` | `2` |

- le récepteur est ensuite créé, regarde l'espace global et **trouve ses propres valeurs**.

L'émetteur n'est pas gêné par ce remplacement : il a déjà chargé son programme et son adresse au
moment de sa création.

>  Un exemple officiel de Renode explique exactement ça, en commentaire juste avant un `mach clear`
> ([`scripts/multi-node/quark-c1000-zephyr/demo.resc`][demo-quark], lignes 44-47) :
>
> *« The Monitor works in the context of the current machine. It allows the user to access the
> machine's peripherals and to set variables local to this machine. The selection must be cleared
> before creating a new machine so that the $bin and $name variables are global and accessible from
> the new context. »*
>
> Le tutoriel Contiki-NG le dit aussi à sa façon : changer `$bin` agit sur toutes les machines
> créées ensuite, et on peut le changer plusieurs fois dans un même script
> ([Running Contiki-NG in Renode][contiki-renode]).


## Est-ce un point faible de Renode ?

Oui et non : c'est **un choix voulu**, mais c'est **un piège facile**  x).

**Pourquoi c'est voulu** : les variables propres à une machine permettent à chaque nœud d'avoir
ses réglages à lui (nom, adresse, macros comme `reset`) sans gêner les autres.

**Pourquoi c'est un piège** :

- **Rien ne prévient** : aucun avertissement, le script se lance sans erreur mais ne fait pas ce
  qu'on veut.
- **La sélection est invisible** : c'est le `mach create` caché dans le fichier commun qui
  sélectionne la machine ; en lisant le scénario, on ne le voit pas.
- **Peu expliqué** : l'exemple officiel pour la CC2538 (`scripts/multi-node/cc2538/rpl-udp.resc`)
  met bien `mach clear` après chaque nœud, mais sans dire pourquoi. L'explication n'apparaît que
  dans un autre exemple ([`quark-c1000-zephyr/demo.resc`][demo-quark]), et la page
  [Working with machines][doc-machines] ne décrit pas `mach clear` (seulement `Clear`, qui efface
  toute la simulation).

**Bonne habitude** : toujours mettre `mach clear` après chaque nœud. 

## Autre approche : des variables propres à chaque machine

Au lieu de passer par l'espace global, on **inverse l'ordre** : on crée la machine d'abord, puis
on définit ses variables pendant qu'elle est sélectionnée. Elles deviennent alors volontairement
des variables à elle.

**Avant** (variables → puis création de la machine) :

```
$id  = ...
$bin = sender
include common        ← le mach create est caché ici
```

**Apres** (création de la machine → puis ses variables) :

```
mach create "emetteur"        ← l'émetteur est sélectionné
$id  = `next_value 1`         ← rangé chez l'émetteur
$bin = sender                 ← rangé chez l'émetteur
include common                ← (common sans son mach create)

mach create "recepteur"       ← le récepteur est sélectionné
$id  = `next_value 1`         ← rangé chez le récepteur
$bin = receiver               ← rangé chez le récepteur
include common
```

**Avantages** :

- plus besoin de `mach clear` entre les nœuds ;
- aucun mélange possible entre les valeurs des nœuds ;
- chaque nœud garde ses réglages pendant toute la simulation (important s'il redémarre, voir
  plus bas) ;
- plus lisible : « je crée tel nœud, voici ses réglages ».


## Et si un nœud redémarre pendant la simulation ?

### Ce qui se passait avant (testé)

Test : on redémarre l'émetteur au milieu de la simulation avec `machine Reset`.

| | Avant le redémarrage | Après le redémarrage |
|---|---|---|
| Programme en mémoire flash | présent | **effacé** (`0xFFFFFFFF`) |
| Ce qu'affiche l'émetteur | « Bonjour #0 »… | **plus rien**, il ne repart pas |

Le redémarrage efface le programme, et rien ne le recharge : le nœud devient muet.

**Pourquoi** : dans `common/cc2538-node.resc`, le programme est chargé **une seule fois**, au
lancement du script (`sysbus LoadELF $bin`). Renode ne sait pas qu'il faut le recharger.

### La solution habituelle : une macro `reset`

C'est ce que fait le script officiel de Renode pour la CC2538
(`scripts/single-node/cc2538.resc`) : on met le chargement dans une macro appelée `reset`, que
Renode relance **automatiquement à chaque redémarrage** :

```
macro reset
"""
    sysbus LoadBinary ...rom...
    sysbus LoadELF $bin
    sysbus WriteDoubleWord 0x00280028 $id
    ...
"""
runMacro $reset
```

>  La doc officielle : *« many Renode sample scripts define the `$reset` macro. This macro is used
> whenever the `machine Reset` method is called by the user or via the simulation logic »*
> ([Monitor and script syntax][doc-syntax], section « Monitor variable types »).

### Le lien avec les variables

La macro relit `$bin` et `$id` **au moment du redémarrage**, pas au lancement.


## Ressources

- [Monitor and script syntax][doc-syntax] (doc Renode) : variables contextuelles, préfixes
  `$global.var` et `$<nom_machine>.var`, valeur par défaut `$var?=`, macros et `$reset`.
- [Working with machines][doc-machines] (doc Renode) : `mach create`, `mach set`, `Clear`.
- [`quark-c1000-zephyr/demo.resc`][demo-quark] (exemple Renode) : le commentaire qui explique
  pourquoi il faut `mach clear`. 
- [Running Contiki-NG in Renode][contiki-renode] (doc Contiki-NG) : effet de `$bin` sur les
  machines créées ensuite.
- `scripts/multi-node/cc2538/rpl-udp.resc` et `scripts/single-node/cc2538.resc` (exemples Renode,
  dans `/Applications/Renode.app/Contents/MacOS/`) : `mach clear` entre les nœuds et macro `reset`
  pour la CC2538.


[doc-syntax]: https://renode.readthedocs.io/en/latest/basic/monitor-syntax.html
[doc-machines]: https://renode.readthedocs.io/en/latest/basic/machines.html
[demo-quark]: https://codex.ac6.fr/Renode/xref/Renode-v1.15.3-c57714d/scripts/multi-node/quark-c1000-zephyr/demo.resc
[contiki-renode]: https://docs.contiki-ng.org/en/develop/doc/tutorials/Running-Contiki-NG-in-Renode.html
