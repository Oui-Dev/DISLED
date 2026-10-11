# Review technique PCB — EasyEDA PRO

Tu vas réaliser une **review technique complète du PCB actuellement ouvert dans EasyEDA PRO**, projet `connected_status_light`.
Si besoin le projet ouvert correspond au fichier `v4_ProPrj_connected_status_light_2026-10-09.epro2` présent dans le dossier, tu as également divers export PNG dans le dossier si tu veux t'en servir en plus du MCP.
Dans la review dédie une petite partie lié a l'évolution du PCB au cours du developpemment en comparant avec `v1_ProPrj_connected_status_light_2026-10-03.epro2`, `v2_ProPrj_connected_status_light_2026-10-06.epro2` et `v3_ProPrj_connected_status_light_2026-10-07.epro2` avec les améliorations (et s'il y en a des régressions).


## 1. Objectif

L'objectif est d'identifier avant fabrication :

- les erreurs électriques ou de routage ;
- les problèmes de fabrication ;
- les problèmes liés aux capacités de fabrication de JLCPCB ;
- les problèmes de placement, assemblage et soudure ;
- les problèmes potentiels d'intégrité du signal ou d'alimentation ;
- les problèmes liés au fonctionnement réel du produit ;
- les choix de conception discutables ou pouvant être améliorés ;
- les coûts ou complexités inutiles ;
- les risques particuliers liés au boîtier et à l'utilisation prévue.

La review finale devra être fournie sous forme de **PDF**, suffisamment détaillée pour que je puisse corriger le PCB avant de lancer une petite production.

**Ne considère jamais mes choix de conception comme nécessairement corrects.** Je suis amateur et certaines décisions peuvent être mauvaises ou sous-optimales.

---

## 2. Contexte de fabrication

- Le PCB sera fabriqué par **JLCPCB**.
- Quantité envisagée : environ **10 à 50 exemplaires**.
- PCB **2 couches**.
- Les composants/soudures du **TOP LAYER** seront assemblés par JLCPCB en **Economic PCBA**.
- S'il y a des composants/soudures du **BOTTOM LAYER** elles seront réalisés manuellement par moi-même.
- Dans le cas de la v1 et la v2 il était prévu que la LED et le connecteur TOUCH seraient commandés séparément sur **LCSC** et soudés manuellement.
- Dans le cas de la v3 le conntecteur TOUCH, malgré qu'il soit monté en top, est exclu du BOM volontairement.
- Dans EasyEDA PRO, les Design Rules utilisées sont :
  `JLCPCB Capability(Two Layers Board)(Default)`
- Tous les composants ont été sélectionnés/importés depuis la bibliothèque LCSC intégrée à EasyEDA PRO.
- Le but est de limiter les coûts pour un petit projet DIY, **sans compromettre la fiabilité ou la qualité**.

Ne te contente donc pas de vérifier que le PCB respecte les Design Rules : vérifie également si ces règles sont suffisantes pour le fonctionnement réel du circuit.

---

## 3. Fonctionnement prévu

### LED de statut

La LED est une LED de statut destinée à être utilisée dans un environnement de bureau.

Chaque écran possède un dispositif situé au sommet permettant d'indiquer si l'employé est disponible ou non.

La LED :

- sera diffusée derrière une paroi en PLA ;
- sera située derrière environ **1 mm de PLA** ;
- devra être suffisamment visible dans un environnement de bureau ;
- sa luminosité sera ajustée dynamiquement en fonction de la luminosité ambiante.

Vérifie notamment :

- le pilotage électrique de la LED ;
- le courant maximal et nominal ;
- la dissipation ;
- la résistance ou le circuit de limitation de courant ;
- la capacité du MCU à piloter directement ou indirectement la LED ;
- les niveaux logiques ;
- la luminosité obtenue ;
- la plage de variation ;
- les éventuels problèmes de PWM ;
- les effets de la paroi PLA ;
- les risques de LED trop faible ou trop lumineuse ;
- les conséquences d'un fonctionnement prolongé.

---

## 4. Capteur de luminosité

Le capteur de luminosité ambiante sert à adapter dynamiquement la puissance/luminosité de la LED.

Le PCB sera installé dans un boîtier cylindrique pour la v1 et la v2 et un cube pour la v3 et v4.

Le boîtier comportera une ouverture (ou une paroi fine) permettant au capteur de luminosité de recevoir la lumière extérieure.

Vérifie :

- le placement du capteur ;
- les risques d'ombre causés par le PCB ou le boîtier ;
- l'influence potentielle de la LED sur la mesure ;
- la plage dynamique ;
- le type de sortie et son raccordement au MCU ;
- les éventuels besoins de filtrage/découplage ;
- la pertinence du routage ;
- la possibilité d'avoir des mesures instables ou bruitées ;
- si le fonctionnement prévu est réellement cohérent avec le composant choisi.

Si certains problèmes ne peuvent pas être évalués correctement sans connaître la géométrie exacte du boîtier, indique-le explicitement plutôt que de faire une hypothèse.

---

## 5. Touch capacitif

Le connecteur TOUCH servira à connecter un câble DUPONT 2.54 mm à une feuille de cuivre externe.
Dans le cas de la V4 il s'agira d'un connecteur JST-XH 2.54mm.
La prise GND du connecteur n'est pas prévu d'être utilisé pour le moment c'est une sécurité dans le cas ou il y aurait besoin d'ajouter un blindage sur le cable du TOUCH.

Cette feuille de cuivre :

- fera environ **20 à 30 mm de diamètre** ;
- sera soit un disque, soit un anneau ;
- servira d'électrode capacitive ;
- sera placée sous/contre le boîtier PLA ;
- aura environ **1 à 2 mm de PLA** entre elle et l'extérieur (infill 100%) ;
- fera face aux plan de masse du PCB ;
- sera située à environ **2 à 4 cm du PCB** ;
- le câble aura approximativement la même longueur que cette distance.

La géométrie exacte du boîtier n'est pas encore complètement définie.

Cette partie est particulièrement importante : réalise une vraie analyse du système capacitif et ne te limite pas à vérifier la connectique.

Vérifie notamment :

- le circuit électrique du touch ;
- le composant utilisé ;
- le principe de détection ;
- la topologie de l'électrode ;
- le routage entre le connecteur et le circuit ;
- la proximité du plan de masse ;
- l'influence du câble/connecteur DUPONT/JST-XH ;
- la longueur du câble ;
- la capacité parasite ;
- l'influence du boîtier PLA ;
- l'influence de la distance de 2–4 cm entre PCB et électrode ;
- la possibilité de faux positifs/négatifs ;
- l'influence de la personne utilisant le dispositif ;
- l'influence de l'alimentation USB ;
- les éventuelles précautions de layout nécessaires.

Si le fonctionnement capacitif dépend fortement de paramètres qui ne sont pas encore définis, **ne conclus pas arbitrairement** : indique les paramètres critiques et pose-moi les questions nécessaires.

---

## 6. MCU / USB / programmation

L'USB aura plusieurs fonctions :

1. alimentation du PCB ;
2. flash/programming du MCU ;
3. debug ;
4. communication USB HID après programmation.

Le flash/debug USB utilise notamment les boutons :

- `BOOT`
- `RESET`

Vérifie notamment :

- le schéma de programmation ;
- le boot mode ;
- le reset ;
- les pull-up/pull-down ;
- les niveaux logiques ;
- les alimentations ;
- les condensateurs de découplage ;
- les connexions USB ;
- les protections USB éventuelles ;
- les résistances éventuelles sur USB ;
- le routage différentiel USB ;
- l'impédance et la géométrie du routage dans la mesure pertinente pour ce PCB ;
- les longueurs et symétries D+/D− ;
- la proximité avec des signaux bruyants ;
- le plan de masse autour de l'USB ;
- les risques ESD ;
- les limites d'alimentation depuis USB ;
- les éventuelles erreurs de conception pouvant empêcher le flash, le debug ou le fonctionnement USB HID.

Ne suppose pas qu'un USB fonctionnera simplement parce que D+ et D− sont connectés.

---

## 7. Connexions de secours

Les **5 pastilles présentes au dos du PCB** servent de connexion de secours pour le flash/debug.

Vérifie :

- quelles fonctions sont réellement accessibles via ces 5 pads ;
- si toutes les lignes nécessaires sont présentes ;
- leur ordre et leur identification ;
- leur accessibilité physique ;
- leur espacement ;
- leur compatibilité avec une méthode de programmation réaliste ;
- si elles constituent réellement un moyen de récupération du MCU en cas de problème.

Si l'information disponible ne permet pas de déterminer quel programmateur/debugger sera utilisé, signale-le et pose la question.

---

## 8. Montage mécanique

Le PCB sera monté dans un boîtier cylindrique.

### v1 et v2
Le PCB sera installé à plat avec :

- la **TOP Layer orientée vers le bas** ;
- la LED orientée vers le haut une fois le PCB installé dans le boîtier.

Le boîtier comportera des ouvertures permettant :

- l'accès à l'USB ;
- l'accès aux boutons ;
- l'entrée de lumière vers le capteur ALS.

### v3 et v4
Le PCB sera installé à plat avec :

- la **TOP Layer orientée vers le haut** ;

Le boîtier comportera des ouvertures permettant :

- l'accès à l'USB ;
- l'entrée de lumière vers le capteur ALS.

L'accès aux boutons devra se faire uniquement en ouvrant le boitier.

### Global

Des trous présents sur le PCB serviront à la fixation avec des **vis M2**.

La feuille de cuivre du touch sera fixée au-dessus du cylindre/cube, à environ **2–4 cm du PCB**, et fera donc face au plan de masse.

Vérifie :

- les dimensions et positions des trous ;
- la compatibilité réelle avec des vis M2 ;
- les distances aux pistes/plans ;
- les risques de court-circuit avec la visserie ;
- les contraintes mécaniques ;
- les accès aux connecteurs et boutons ;
- la compatibilité avec le boîtier décrit ;
- les éventuelles contraintes de montage ;
- les conséquences de l'orientation du PCB ;
- les éventuelles collisions mécaniques identifiables depuis le PCB.

Si la géométrie du boîtier est insuffisante pour valider un point, marque-le comme **à vérifier lors de la conception mécanique**.

---

## 9. Fabrication JLCPCB

Vérifie spécifiquement :

- largeur minimale des pistes ;
- espacement ;
- vias ;
- tailles de perçage ;
- pads ;
- solder mask ;
- silkscreen ;
- distances au bord ;
- cuivre ;
- zones de masse ;
- capacités de fabrication pertinentes ;
- compatibilité avec l'assemblage Economic ;
- orientation des composants ;
- composants traversants éventuels ;
- composants placés sur le mauvais côté ;
- composants qui risquent d'être exclus de l'assemblage automatique ;
- composants à souder manuellement ;
- contraintes liées aux composants LCSC sélectionnés ;
- éventuelles références/footprints incompatibles ;
- polarités ;
- repères de polarité ;
- lisibilité du silkscreen ;
- risques de tombstoning ou autres défauts d'assemblage lorsque pertinents.

Ne te limite pas aux Design Rules EasyEDA : vérifie autant que possible les exigences réelles de fabrication et d'assemblage de JLCPCB.

---

## 10. Analyse électrique

Effectue une vérification systématique du schéma et du PCB.

Vérifie notamment :

- alimentation ;
- GND ;
- découplage ;
- régulation ;
- consommation ;
- courants ;
- tensions ;
- niveaux logiques ;
- pull-up/pull-down ;
- entrées flottantes ;
- sorties en conflit ;
- polarités ;
- valeurs des composants ;
- références des composants ;
- erreurs de net ;
- nets non connectés ;
- connexions involontaires ;
- court-circuits potentiels ;
- chemins de retour de courant ;
- plans de masse ;
- placement des condensateurs ;
- signaux sensibles ;
- signaux rapides ;
- signaux analogiques ;
- séparation entre signaux bruyants et sensibles lorsque pertinente.

Si tu détectes une valeur de composant potentiellement incorrecte, explique **pourquoi** et, lorsque possible, calcule ou estime la valeur attendue.

---

## 11. Analyse du PCB

Analyse séparément :

### Schéma

Cherche les erreurs fonctionnelles ou incohérences électriques.

### Placement

Vérifie la pertinence du placement des composants par rapport à leur fonction.

### Routage

Vérifie notamment :

- qualité générale du routage ;
- largeur des pistes ;
- retours de masse ;
- vias ;
- angles et géométrie lorsque pertinents ;
- USB ;
- alimentation ;
- signaux analogiques ;
- signaux capacitifs ;
- zones sensibles ;
- distances ;
- découplage.

### Plan de masse

Analyse le plan de masse comme un élément électrique réel et pas seulement comme une règle DRC.

Porte une attention particulière au **touch capacitif**, à l'USB, à l'ALS et aux éventuels signaux sensibles.

---

## 12. Méthode de review

Avant de produire le PDF :

1. Inspecte l'environnement disponible.
2. Vérifie quels MCP/Skills sont déjà installés.
3. Installe les MCP/Skills nécessaires à la review (exemple : `https://github.com/easyeda/easyeda-api-skill`).
4. Tu peux utiliser Node.js et installer/utiliser des `node_modules` avec NPM si nécessaires.
5. Tu peux utiliser le dossier de travail ouvert.
6. Inspecte le projet EasyEDA PRO actuellement ouvert a l'aide du MCP.
7. Récupère autant d'informations que possible directement depuis le projet plutôt que de supposer.
8. Vérifie le schéma et le PCB.
9. Vérifie les composants et leurs datasheets lorsqu'une vérification est nécessaire.
10. Vérifie les capacités actuelles de JLCPCB/LCSC lorsque nécessaire.
11. Identifie les informations manquantes.
12. **Si une information manquante peut changer significativement la conclusion, arrête-toi et pose-moi une question avant de finaliser la review.**
13. Une fois les informations suffisantes obtenues, réalise la review complète.
14. Génère le PDF final.

Ne modifie pas le PCB ou le schéma sans mon autorisation explicite.

---

## 13. Gestion des incertitudes

Utilise les catégories suivantes :

- **CRITIQUE** : risque important de fonctionnement incorrect, dommage ou impossibilité de fabrication.
- **MAJEUR** : problème sérieux qui devrait être corrigé avant fabrication.
- **MINEUR** : amélioration recommandée mais ne bloquant probablement pas la fabrication.
- **OPTIMISATION** : amélioration facultative, notamment coût, simplicité ou robustesse.
- **À VÉRIFIER** : impossible à conclure avec les informations actuellement disponibles.

Pour chaque problème, indique :

1. le niveau ;
2. le composant/net/zone concerné ;
3. le problème ;
4. pourquoi c'est un problème ;
5. la conséquence potentielle ;
6. la correction recommandée ;
7. le niveau de confiance de la conclusion ;
8. si une information supplémentaire est nécessaire.

Ne présente jamais une hypothèse comme un fait.

---

## 14. Questions avant conclusion

Je préfère que tu me poses une question plutôt que de faire une hypothèse incorrecte.

Pose-moi une question si, par exemple :

- un choix de composant est ambigu ;
- le fonctionnement attendu n'est pas clair ;
- une contrainte mécanique manque ;
- la méthode de programmation/debug n'est pas définie ;
- la géométrie du touch est insuffisante ;
- un paramètre USB est critique mais impossible à déterminer ;
- une contrainte de fabrication JLCPCB n'est pas suffisamment connue ;
- plusieurs solutions raisonnables sont possibles et ont des conséquences différentes.

Regroupe les questions lorsque c'est possible afin d'éviter des allers-retours inutiles.

---

## 15. Rapport PDF

Le PDF final doit être structuré au minimum comme suit :

1. **Résumé exécutif**
   - état général du PCB ;
   - nombre de problèmes critiques/majeurs/mineurs ;
   - décision recommandée : `FABRIQUER`, `CORRIGER AVANT FABRICATION` ou `IMPOSSIBLE À VALIDER`.

2. **Points bloquants**
   - problèmes critiques et majeurs.

3. **Analyse électrique**
   - alimentation ;
   - MCU ;
   - USB ;
   - LED ;
   - ALS ;
   - touch ;
   - programmation/debug.

4. **Analyse du schéma**

5. **Analyse du placement**

6. **Analyse du routage**

7. **Analyse du plan de masse**

8. **Analyse fabrication JLCPCB**

9. **Analyse assemblage**

10. **Analyse mécanique**

11. **Analyse du système capacitif**

12. **Optimisations coût / fiabilité**

13. **Liste des corrections recommandées**
   - sous forme de checklist directement exploitable.

14. **Points restant à vérifier**

15. **Évolution de la v1 à la v4**

Avec, si possible, des screenshots du PCB Top Layer.

16. **Comparaison de la v4 avec les propositions**

Compare la v4 au proposition (carré et ronde) de cette artifact : `https://claude.ai/artifact/PfbCM2u4ZrA6LeXi5URJQj`

17. **Conclusion**

Lorsque cela est utile, ajoute des captures ou extraits annotés du PCB/schéma dans le rapport.

Pour chaque problème important, indique précisément **où il se trouve** afin que je puisse le retrouver facilement dans EasyEDA PRO.

---

## 18. Priorité

La priorité absolue est :

1. sécurité ;
2. fonctionnement correct ;
3. fiabilité ;
4. fabricabilité ;
5. assemblage ;
6. maintenabilité/debug ;
7. coût ;
8. optimisation esthétique.

Ne recommande pas une économie qui dégrade significativement les points 1 à 5.

À l'inverse, ne recommande pas automatiquement des composants ou solutions plus chers si le gain réel est faible pour une production de seulement 10–20 exemplaires.

---

## 19. Communication

En dehors du PDF et des questions nécessaires à la review, reste très concis.

Évite les explications inutiles pendant l'exécution afin d'économiser les tokens.

**Ne fais aucune modification du projet EasyEDA sans mon autorisation explicite.**
