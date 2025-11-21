## 1. Contexte du projet

### 1.1 Matériel

- Carte : **ESP32 "Cheap Yellow Display" (ESP32-2432S028R)**  
- Écran : TFT 320x240, piloté via **TFT_eSPI**
- Tactile : **XPT2046**, driver `XPT2046_Touchscreen` (ou variante `XPT2046_Touchscreen_TT`)
- Stockage : flash interne **4 MB**, partition :
  - **Huge APP (3MB No OTA / 1MB SPIFFS)**

### 1.2 Outils et environnement

- IDE principal : **Arduino IDE**
- Carte sélectionnée : `ESP32 Dev Module`
- Bibliothèques utilisées :
  - `TFT_eSPI`
  - `XPT2046_Touchscreen` ou `XPT2046_Touchscreen_TT`
  - Plus tard : `SPIFFS` / `LittleFS` pour la sauvegarde

### 1.3 Plan de développement
#### Phase 1 — Solidifier le socle “scènes + UI tactile”
##### 1.1 Normaliser la gestion des scènes (enum, changeScene) et ajouter un état GAME / SETTINGS avec callbacks d’entrée/sortie.

##### 1.2 Factoriser la gestion des boutons : struct Button enrichie (couleurs, texte, handler), tableau de boutons par scène, fonction processTouchForButtons() (hit-test + debounce centralisé).

##### 1.3 Écran TITLE : animation simple (clignotement “Tap to start”), timeout auto vers MENU après X secondes.

##### 1.4 Écran MENU : améliorer layout (icônes, séparateurs) et ajouter un bouton Settings placeholder.

#### Phase 2 — Création du pet et écran “NEW PET”
##### 2.1 Définir une struct Pet (nom, âge, besoins : hunger, energy, social, cleanliness, curiosity, mood). Prévoir valeurs 0..1.

##### 2.2 Écran NEW_PET : saisie rapide du nom (clavier virtuel minimal ou sélection pré-faite), bouton “Start”.

##### 2.3 Génération initiale : random seeds/gènes simples (poids Utility AI) + initialisation besoins à 0.4–0.6.

##### 2.4 Transition vers GAME après validation.

#### Phase 3 — Boucle de jeu de base (HUD + interactions)
##### 3.1 Écran GAME : dessin du pet (sprite simple / cercle), HUD barres de besoins + humeur, badge horloge (jour/nuit).

##### 3.2 Zones d’action tactiles (Manger, Jouer, Dormir, Laver, Socialiser) ; chaque bouton déclenche un effet sur besoins.

##### 3.3 Tick de mise à jour updateNeeds(dt) avec millis() (décroissance lente) ; clamp et mood calculée en moyenne pondérée.

##### 3.4 Mini animation (rebond/yeux clignotants) et feedback visuel lors d’une action (flash de couleur, petite particule).

#### Phase 4 — Utility AI / petit cerveau
##### 4.1 Implémenter chooseAction() : score par besoin (poids gènes) + bruit aléatoire ; exécuter périodiquement si aucune action joueur.

##### 4.2 applyAction(action) : modifier besoins et humeur ; cooldown pour éviter le spam.

##### 4.3 Ajustement léger des gènes (renforcement) si l’action améliore l’humeur (apprentissage simplifié).

##### 4.4 Journal interne (petites bulles texte ou icônes) indiquant l’action choisie par l’IA.

#### Phase 5 — Sauvegarde / chargement
##### 5.1 Ajouter SPIFFS/LittleFS init en setup ; struct de stockage (JSON léger).

##### 5.2 Fonctions bool savePet(const Pet&, const char* path) et bool loadPet(Pet&, const char* path), avec validation (version, checksum simple).

##### 5.3 Intégrer bouton Load dans MENU pour charger depuis SPIFFS ; fallback si aucun fichier (message d’erreur et retour MENU).

##### 5.4 Autosave périodique en jeu (toutes les X minutes) et sur extinction (hook ESP.restart / bouton reset).

#### Phase 6 — Améliorations visuelles & UX
##### 6.1 Thème visuel cohérent (palette, cadres arrondis, ombres simples), refactor dessins en fonctions utilitaires (fillRoundedFrame, drawProgressBar).

##### 6.2 Animations supplémentaires : sprite sheet basique (2–4 frames), clignotement nuit/jour, transitions d’écran (fondu slide).

##### 6.3 Écran SETTINGS : volume (placeholder), luminosité/sleep timeout, recalibration tactile rapide.

##### 6.4 Effets audio (quand hardware dispo) ou vibration légère (buzzer) — optionnel.

#### Phase 7 — Gestion du temps et événements
##### 7.1 Cycle jour/nuit basé sur horloge interne (millis + offset) ; impact sur humeur/énergie.

##### 7.2 Vieillissement : âge qui progresse, événements seuils (adolescence, adulte) modifiant sprites ou poids AI.

##### 7.3 Événements aléatoires légers (cadeaux, météo) influençant besoins.

##### Phase 8 — Mini-activités
##### 8.1 Mini-jeu “tap/drag” simple (ex : attraper des bulles) pour booster social/curiosity.

##### 8.2 Mini-jeu “sieste” (barre de timing) pour énergie.

##### 8.3 Tableau des scores internes / stats affichables dans SETTINGS ou un sous-écran.

#### Phase 9 — Refactors & modularisation
##### 9.1 Scinder en modules/fichiers : ui_scenes.*, pet_model.*, pet_ai.*, storage.*, input_touch.*.

##### 9.2 Centraliser constantes (couleurs, tailles, chemins SPIFFS) et types (enum actions).

##### 9.3 Ajouter un petit logger conditionnel (macro DEBUG) pour Serial.

#### Phase 10 — Tests manuels ciblés
##### 10.1 Scénarios tactiles : détection, debounce, calibration rapide.

##### 10.2 Scénarios sauvegarde/chargement : fichier absent, fichier corrompu, nouvelle partie.

##### 10.3 Stabilité : boucle de 24 h (simulation) pour vérifier la dérive des besoins et la robustesse de l’AI.





---

## 2. État actuel du projet (base)

Le projet possède déjà :

- Une **machine d’états** simple via un `enum AppState` et une fonction centrale `changeScene(AppState next)` avec au moins les états :
  - `STATE_TITLE`
  - `STATE_MENU`
  - `STATE_NEW_PET`
  - `STATE_GAME`
- Un système de **rendu d’écran** :
  - `drawTitleScreen()`
  - `drawMenuScreen()`
  - `drawNewPetScreen()`
  - `drawGameScreen()`
- Une abstraction de **bouton** :

```cpp
  typedef void (*ButtonAction)();

  struct Button {
    int16_t x, y, w, h;
    const char* label;
    uint16_t fillColor;
    uint16_t textColor;
    uint16_t borderColor;
    ButtonAction onTap;
  };
```

* Une gestion centralisée du **tactile** :

  * Fonction `getTouch(int16_t &x, int16_t &y)` qui retourne `true` si un touch valide est détecté, et fait le `map()` brut → écran 320x240.
  * Fonction `processTouchForButtons(Button* buttons, size_t count)` qui :

    * lit le touch,
    * fait le **hit-test** sur chaque bouton,
    * applique un **debounce**,
    * appelle le `ButtonAction` correspondant.

* Un **flow de base** :

  * Écran **TITLE** → tap → **MENU**
  * MENU avec boutons :

    * `New Pet` (ou texte équivalent) → `STATE_NEW_PET`
    * `Load Pet` → (pour l’instant, mène directement à `STATE_GAME`)

---

## 3. Vision globale du projet

Objectif à terme : un **pet virtuel façon dvpet** :

* Un pet avec :

  * des **besoins internes** : faim, énergie, social, propreté, curiosité
  * une **humeur** globale
* Un **cerveau** (Utility AI simple) :

  * qui choisit périodiquement des actions (MANGER, DORMIR, JOUER, LAVER, etc.)
  * qui peut légèrement **apprendre** en ajustant des poids/gènes
* Une **interface graphique** agréable :

  * écran titre
  * menu principal
  * écran de création de pet (NEW_PET)
  * écran de jeu (GAME) avec HUD + sprite/emoji
* Une **persistance** :

  * sauvegarde de l’état du pet en SPIFFS/LittleFS
  * chargement via “Load Pet”
* Des **extensions** potentielles :

  * mini-jeux
  * cycle jour/nuit
  * vieillissement (âge, étapes de vie)
  * améliorations visuelles (animations, effets, thèmes)

Le cœur du projet se fait **entièrement offline**, sur la carte ESP32, sans cloud.

---

## 4. Plan de développement (version résumée de “Plan 2”)

> **Important pour l’agent :**
> L’utilisateur fait avancer le projet **par phases**.
> Tu ne dois pas tout implémenter d’un coup, mais respecter la phase demandée.

### Phase 1 – Scènes & UI tactile (EN GRANDE PARTIE FAITE)

1.1 Normaliser la gestion des scènes :
`AppState`, `changeScene()`, callbacks d’entrée/sortie.

1.2 Factoriser la gestion des boutons :
`struct Button`, tableaux de boutons par scène, `processTouchForButtons()`.

1.3 (Plus tard) Améliorer l’écran TITLE :
clignotement “Tap to start”, timeout auto vers MENU.

1.4 (Plus tard) Améliorer le MENU :
layout, icônes, bouton Settings placeholder.

### Phase 2 – Création du pet et écran “NEW PET”

2.1 Définir `struct Pet` :
nom, âge (plus tard), hunger, energy, social, cleanliness, curiosity, mood.

2.2 (Version simple d’abord) : génération de pet par défaut, nom fixe ou random parmi une petite liste.

2.3 Écran NEW_PET :
texte “Nouveau Pet” + résumé des stats initiales, bouton “Start”.

2.4 Transition vers GAME avec un pet initialisé.

### Phase 3 – Boucle de jeu de base (HUD + interactions)

3.1 Écran GAME :
affichage du pet (sprite simple ou emoji), HUD (textes ou barres des besoins + humeur).

3.2 Zones d’action/boutons : Manger / Jouer / Dormir / Laver / Socialiser.

3.3 `updateNeeds(dt)` :
évolution des besoins dans le temps via `millis()`, calcul de `mood`.

3.4 Petites animations / feedback visuels d’actions.

### Phase 4 – Utility AI / Cerveau

4.1 `chooseAction()` :
sélection d’une action selon les besoins (Utility AI + un peu de random).

4.2 `applyAction(action)` :
modifie les besoins, mood, gère des cooldowns.

4.3 Ajustement léger des “gènes” si l’action améliore l’humeur (apprentissage).

4.4 Feedback visuel (bulle d’icône ou texte) indiquant l’action décidée par l’IA.

### Phase 5 – Sauvegarde / chargement (SPIFFS/LittleFS)

5.1 Init SPIFFS/LittleFS en `setup()`.

5.2 Fonctions :

* `bool savePet(const Pet&, const char* path);`
* `bool loadPet(Pet&, const char* path);`

5.3 Intégrer le chargement dans “Load Pet” + gestion d’absence de fichier.

5.4 Autosave périodique en jeu.

### Phases 6–10 (à long terme)

* 6 : Jour/nuit, âge, événements.
* 7 : Améliorations visuelles & UX.
* 8 : Mini-jeux.
* 9 : Refactor modulaire en plusieurs fichiers.
* 10 : Tests (stabilité, comportements sur longue durée, etc.).

---

## 5. Règles de travail pour l’agent (ChatGPT-Codex)

### 5.1 Rôle spécifique

Tu es l’**agent codeur** du projet :

* Tu dois :

  * produire du **code concret** (C++ / Arduino),
  * respecter l’architecture et les conventions déjà en place,
  * être **progressif** : implémenter uniquement la ou les tâches **explicitement demandées** (ex: “Phase 2.1 et 2.3”).
* Tu ne dois pas :

  * tout réécrire sans raison,
  * changer les bibliothèques fondamentales (TFT_eSPI / XPT2046) sans accord explicite,
  * casser la structure de scène déjà en place.

### 5.2 Style de code

* Clarity > cleverness
* Noms explicites : `initDefaultPet()`, `updateNeeds()`, `drawGameHUD()`, etc.
* Commentaires courts mais utiles.
* Éviter les macros obscures ou la métaprogrammation inutile.

### 5.3 Interaction avec l’utilisateur

L’utilisateur va généralement :

1. Te rappeler le **Plan** ou la **Phase** qu’il souhaite avancer.
2. Te fournir éventuellement le **code actuel** (fichier `.ino` ou extraits).
3. Te demander d’implémenter une ou plusieurs tâches précises, ex. :

   * “Implémente 2.1 + 2.3”
   * “Ajoute la mise à jour des besoins (3.3)”
   * “Refactorise les boutons (1.2) pour permettre X”

Dans ta réponse :

* Explique brièvement **ce que tu changes**.
* Donne le **code complet** des fonctions modifiées/ajoutées.
* Si tu modifies un bloc existant, précise :

  * “Remplace la fonction X par : …”
  * ou “Ajoute ce bloc après la ligne Y”.

### 5.4 Respect de l’état du projet

* Ne pas supprimer ou écraser :

  * `changeScene(AppState)`
  * la structure de base des scènes,
  * `processTouchForButtons()`.
* Tu peux introduire de nouveaux fichiers (plus tard, Phase 9), mais uniquement si l’utilisateur demande explicitement un refactor modulaire.

### 5.5 Ce que tu peux suggérer

Tu peux proposer :

* Des améliorations de lisibilité.
* Des micro refactors (ex. fonctions pour dessiner une barre de besoin).
* Des optimisations légères, **à condition** de rester compatible Arduino/ESP32.

Mais **ne les applique pas** sans l’accord explicite de l’utilisateur si cela implique un changement d’architecture important.

---

## 6. Exemple de pattern de travail

### Exemple 1 – Ajout de struct Pet (Phase 2.1)

> Utilisateur :
> “Implémente la Phase 2.1 : struct Pet + initDefaultPet(), en modifiant mon code actuel.”

> Agent (résumé de la démarche) :
>
> * Expliquer :
>
>   * où tu ajoutes la `struct Pet`,
>   * ce que contient `initDefaultPet()`,
>   * où tu appelles `initDefaultPet()` (par ex. dans `actionStartNewPet()`).
> * Fournir le code :
>
>   * nouveau `struct Pet`,
>   * variable globale `Pet currentPet;` + `bool petInitialized;`,
>   * fonction `initDefaultPet()`,
>   * modifications dans les fonctions d’action des boutons.

### Exemple 2 – HUD de base (Phase 3.1)

> Utilisateur :
> “Maintenant, ajoute un HUD texte dans `drawGameScreen()` pour afficher les besoins du pet et son humeur.”

> Agent :
>
> * Expliquer brièvement l’approche.
> * Donner une nouvelle version de `drawGameScreen()` qui :
>
>   * affiche le nom du pet,
>   * affiche hunger/energy/social/cleanliness/curiosity/mood sous forme de textes numéraires,
>   * dessine un petit emoji au centre.

---

## 7. Résumé

* Ce projet vise un **virtual pet dvpet-like** sur ESP32 CYD.
* La **structure de scènes et de boutons existe déjà** – à respecter.
* Le développement avance **par phases** clairement énoncées.
* L’agent doit :

  * garder le code **modulaire et lisible**,
  * respecter le **plan demandé**,
  * implémenter **uniquement** les tâches explicites de la phase en cours,
  * toujours partir du **code actuel** fourni par l’utilisateur.

Si tu es en doute, demande à l’utilisateur **quelle phase / quelles sous-tâches** du plan il souhaite traiter en priorité avant de pondre une grosse refonte.
