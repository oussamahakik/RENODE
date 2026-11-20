#!/usr/bin/env python3
"""
Compare le test de debit (dossier debit/) entre plusieurs logs du RECEPTEUR
(ex. cartes reelles, Renode d'origine, Renode corrige).

Usage :
    python3 compareDebit.py debit_reel.log debit_renode.log debit_renode_realiste.log
    python3 compareDebit.py *.log --csv debit.csv
    python3 compareDebit.py *.log --html rapport.html

Lit les lignes ecrites par recepteur.c :
    "SECONDE 3 : 42 trames"
    "=== BILAN RECEPTEUR : 1000 recues, 0 perdues, 100 octets par trame,
     duree = 24546 ms, 40.6 trames/s, debit utile = 32.5 kbit/s ==="

Affiche un tableau de comparaison et genere un rapport HTML avec deux
graphiques (par defaut : debit_comparaison.html, a ouvrir dans un navigateur ;
aucune bibliotheque a installer).
"""
import argparse
import csv
import html
import os
import re

SEC_RE = re.compile(r"SECONDE\s+(\d+)\s*:\s*(\d+)\s+trames")
BILAN_RE = re.compile(
    r"BILAN RECEPTEUR\s*:\s*(\d+)\s+recues,\s*(\d+)\s+perdues,\s*(\d+)\s+octets par trame,"
    r"\s*duree\s*=\s*(\d+)\s*ms,\s*([\d.]+)\s+trames/s,\s*debit utile\s*=\s*([\d.]+)\s*kbit/s")

COULEURS = {"reel": "#2A78D6", "origine": "#EB6834", "corrige": "#1BAF7A"}
AUTRES = ["#8E5CD9", "#D9A21B", "#3BA7A7", "#C2417B"]


def parse(path):
    """Retourne (liste des trames par seconde, bilan ou None)."""
    secondes, bilan = [], None
    with open(path, errors="replace") as f:
        for line in f:
            m = SEC_RE.search(line)
            if m:
                secondes.append(int(m.group(2)))
                continue
            m = BILAN_RE.search(line)
            if m:
                bilan = {
                    "recues": int(m.group(1)),
                    "perdues": int(m.group(2)),
                    "octets": int(m.group(3)),
                    "duree_ms": int(m.group(4)),
                    "trames_s": float(m.group(5)),
                    "kbit_s": float(m.group(6)),
                }
    return secondes, bilan


def label_of(path):
    """debit_renode_realiste.log -> renode_realiste ; debit_reel.log -> reel."""
    name = re.sub(r"\.log(\.\d+)?$", "", os.path.basename(path))
    return name[6:] if name.startswith("debit_") else name


def couleur(label, i):
    low = label.lower()
    if "reel" in low:
        return COULEURS["reel"]
    if "realiste" in low or "corrig" in low:
        return COULEURS["corrige"]
    if "renode" in low:
        return COULEURS["origine"]
    return AUTRES[i % len(AUTRES)]


def resume(label, secondes, bilan):
    if bilan is None:
        print(f"\n[{label}] pas de ligne BILAN RECEPTEUR (test interrompu ?)")
        if secondes:
            print(f"  {len(secondes)} secondes lues, moyenne "
                  f"{sum(secondes) / len(secondes):.1f} trames/s")
        return
    ecart = 1000 / bilan["trames_s"] if bilan["trames_s"] else 0
    total = bilan["recues"] + bilan["perdues"]
    pertes = 100 * bilan["perdues"] / total if total else 0
    print(f"\n[{label}]")
    print(f"  trames recues   : {bilan['recues']} ({bilan['perdues']} perdues, {pertes:.1f} %)")
    print(f"  taille          : {bilan['octets']} octets de donnees par trame")
    print(f"  duree           : {bilan['duree_ms'] / 1000:.2f} s")
    print(f"  trames/s        : {bilan['trames_s']:.1f}"
          f"  (une trame toutes les {ecart:.2f} ms en moyenne)")
    print(f"  debit utile     : {bilan['kbit_s']:.1f} kbit/s")
    if secondes:
        # la premiere et la derniere seconde sont souvent incompletes
        milieu = secondes[1:-1] or secondes
        print(f"  par seconde     : min {min(milieu)}, max {max(milieu)} trames"
              f" (premiere et derniere seconde exclues)")



def svg_barres(sources, cle, titre, unite):
    vals = [(lab, b[cle], col) for lab, _, b, col in sources if b]
    if not vals:
        return ""
    larg, h_barre, gauche, droite = 760, 34, 190, 110
    haut = 40 + len(vals) * (h_barre + 14)
    vmax = max(v for _, v, _ in vals) or 1
    echelle = (larg - gauche - droite) / vmax
    out = [f'<svg viewBox="0 0 {larg} {haut}" role="img" aria-label="{html.escape(titre)}">',
           f'<text x="0" y="20" class="titre">{html.escape(titre)}</text>']
    y = 40
    for lab, v, col in vals:
        w = max(v * echelle, 2)
        out.append(f'<text x="{gauche - 10}" y="{y + h_barre / 2 + 5}" text-anchor="end">'
                   f'{html.escape(lab)}</text>')
        out.append(f'<rect x="{gauche}" y="{y}" width="{w:.1f}" height="{h_barre}" '
                   f'rx="4" fill="{col}"><title>{html.escape(lab)} : {v:g} {unite}</title></rect>')
        out.append(f'<text x="{gauche + w + 8:.1f}" y="{y + h_barre / 2 + 5}">{v:g} {unite}</text>')
        y += h_barre + 14
    out.append("</svg>")
    return "\n".join(out)


def svg_courbes(sources):
    series = [(lab, s, col) for lab, s, _, col in sources if s]
    if not series:
        return ""
    larg, haut, g, d, h, b = 760, 320, 60, 20, 50, 50
    nmax = max(len(s) for _, s, _ in series)
    vmax = max(max(s) for _, s, _ in series) or 1
    vmax = vmax * 1.1
    sx = lambda i: g + (i / max(nmax - 1, 1)) * (larg - g - d)
    sy = lambda v: haut - b - (v / vmax) * (haut - h - b)
    out = [f'<svg viewBox="0 0 {larg} {haut}" role="img" aria-label="Trames recues par seconde">',
           '<text x="0" y="20" class="titre">Trames reçues chaque seconde</text>']
    # grille horizontale
    for k in range(5):
        v = vmax * k / 4
        y = sy(v)
        out.append(f'<line x1="{g}" x2="{larg - d}" y1="{y:.1f}" y2="{y:.1f}" class="grille"/>')
        out.append(f'<text x="{g - 8}" y="{y + 4:.1f}" text-anchor="end" class="axe">{v:.0f}</text>')
    out.append(f'<text x="{(g + larg - d) / 2}" y="{haut - 10}" text-anchor="middle" class="axe">'
               f'seconde</text>')
    # legende
    x = g
    for lab, _, col in series:
        out.append(f'<rect x="{x}" y="30" width="12" height="12" rx="2" fill="{col}"/>')
        out.append(f'<text x="{x + 18}" y="41">{html.escape(lab)}</text>')
        x += 30 + 8 * len(lab)
    # courbes
    for lab, s, col in series:
        pts = " ".join(f"{sx(i):.1f},{sy(v):.1f}" for i, v in enumerate(s))
        out.append(f'<polyline points="{pts}" fill="none" stroke="{col}" stroke-width="2"/>')
        for i, v in enumerate(s):
            out.append(f'<circle cx="{sx(i):.1f}" cy="{sy(v):.1f}" r="3.5" fill="{col}">'
                       f'<title>{html.escape(lab)} — seconde {i + 1} : {v} trames</title></circle>')
    out.append("</svg>")
    return "\n".join(out)


def write_html(path, sources):
    lignes = []
    for lab, _, bil, _ in sources:
        if bil:
            lignes.append(
                f"<tr><td>{html.escape(lab)}</td><td>{bil['recues']}</td><td>{bil['perdues']}</td>"
                f"<td>{bil['octets']}</td><td>{bil['duree_ms'] / 1000:.2f} s</td>"
                f"<td>{bil['trames_s']:.1f}</td><td>{1000 / bil['trames_s']:.2f} ms</td>"
                f"<td>{bil['kbit_s']:.1f}</td></tr>")
        else:
            lignes.append(f"<tr><td>{html.escape(lab)}</td><td colspan='7'>pas de bilan</td></tr>")
    page = f"""<!doctype html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Comparaison du débit</title>
<style>
  :root {{ --fond:#fff; --texte:#1d1d1f; --doux:#6e6e73; --grille:#e5e5ea; }}
  @media (prefers-color-scheme: dark) {{
    :root {{ --fond:#1c1c1e; --texte:#f2f2f7; --doux:#a1a1a6; --grille:#3a3a3c; }}
  }}
  body {{ background:var(--fond); color:var(--texte); font-family:-apple-system,
         Segoe UI, Roboto, sans-serif; max-width:820px; margin:24px auto; padding:0 16px; }}
  svg {{ width:100%; height:auto; margin:18px 0; }}
  svg text {{ fill:var(--texte); font-size:13px; }}
  svg .titre {{ font-size:15px; font-weight:600; }}
  svg .axe {{ fill:var(--doux); font-size:12px; }}
  svg .grille {{ stroke:var(--grille); stroke-width:1; }}
  table {{ border-collapse:collapse; width:100%; font-size:14px; }}
  th, td {{ padding:6px 8px; border-bottom:1px solid var(--grille); text-align:right; }}
  th:first-child, td:first-child {{ text-align:left; }}
  th {{ color:var(--doux); font-weight:500; }}
</style></head><body>
<h1>Test de débit</h1>
<table>
<tr><th>Configuration</th><th>Reçues</th><th>Perdues</th><th>Octets/trame</th>
<th>Durée</th><th>Trames/s</th><th>Écart moyen</th><th>kbit/s</th></tr>
{''.join(lignes)}
</table>
{svg_barres(sources, 'trames_s', 'Trames reçues par seconde (moyenne)', 'trames/s')}
{svg_barres(sources, 'kbit_s', 'Débit utile', 'kbit/s')}
{svg_courbes(sources)}
</body></html>
"""
    with open(path, "w", encoding="utf-8") as f:
        f.write(page)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="+", help="fichiers .log du RECEPTEUR")
    ap.add_argument("--csv", help="exporter les trames par seconde dans ce CSV")
    ap.add_argument("--html", default="debit_comparaison.html",
                    help="nom du rapport HTML (defaut : debit_comparaison.html)")
    ap.add_argument("--no-html", action="store_true", help="ne pas generer le rapport HTML")
    args = ap.parse_args()

    sources = []
    for i, path in enumerate(args.logs):
        lab = label_of(path)
        secondes, bilan = parse(path)
        sources.append((lab, secondes, bilan, couleur(lab, i)))
        resume(lab, secondes, bilan)

    # comparaison avec la premiere source qui a un bilan (ex. les cartes reelles)
    ref = next(((lab, b) for lab, _, b, _ in sources if b), None)
    if ref and len(sources) > 1:
        print(f"\nRapport au debit de « {ref[0]} » :")
        for lab, _, b, _ in sources:
            if b and lab != ref[0] and ref[1]["kbit_s"]:
                print(f"  {lab:<22} x {b['kbit_s'] / ref[1]['kbit_s']:.2f}")

    if args.csv:
        with open(args.csv, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["source", "seconde", "trames"])
            for lab, secondes, _, _ in sources:
                for i, v in enumerate(secondes, 1):
                    w.writerow([lab, i, v])
        print(f"\nCSV ecrit : {args.csv}")

    if not args.no_html:
        write_html(args.html, sources)
        print(f"Rapport HTML : {args.html}")


if __name__ == "__main__":
    main()
