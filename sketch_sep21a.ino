#include <Arduino.h>

// Doit rester tout en haut (l'IDE Arduino génère des prototypes avant le reste du code)
struct Note { uint16_t freq, duree; };   // freq en Hz (0 = silence), duree en ms

// ---------- Broches ----------
const uint8_t DS_PIN = 13, SHCP_PIN = 14, STCP_PIN = 12;   // 74HC595 : D7, D5, D6 (OE -> GND, CLEAR -> HIGH)
// Sorties du 74HC595 : Q1..Q5 = lumières du bandeau 1 à 5 | Q7, Q6, Q0 = LED bleues 1 à 3
const uint8_t BLEUES[] = {1 << 7, 1 << 6, 1 << 0};         // masques des LED bleues, dans l'ordre d'allumage
const uint8_t VERTES[] = {16, 4, 5};                       // D0, D2, D1 : LED vertes, dans l'ordre d'allumage
const uint8_t NB_VERTES = sizeof(VERTES);
const uint8_t ROUGE_PIN = 15, BOUTON_PIN = 0, BUZZER_PIN = 3, BANDEAU_PIN = 2;   // D8, D3, RX, D4 (D4 inutilisée)

// ---------- Réglages ----------
const bool BUZZER_PASSIF = true;   // false = buzzer actif (seul le rythme est audible)
const unsigned long DELAI_DEMARRAGE_MS = 2000, DELAI_PREPARATION_MS = 500, INACTIVITE_MS = 2000, DELAI_ANTIREBOND = 30;

// ---------- 74HC595 (1 puce) ----------
void ecrire595(uint8_t motif) {                            // bit 0 = Q0 ... bit 7 = Q7
  digitalWrite(STCP_PIN, LOW);
  shiftOut(DS_PIN, SHCP_PIN, MSBFIRST, motif);
  digitalWrite(STCP_PIN, HIGH);
}
void stripUneSeule(uint8_t n) { ecrire595(n < 5 ? 2 << n : 0); }       // lumière 0 à 4 (Q1 à Q5), sinon tout éteint
void bleueSeule(uint8_t n)    { ecrire595(n < 3 ? BLEUES[n] : 0); }    // LED bleue 0 à 2, sinon tout éteint
void stripBarre(uint8_t n)     { ecrire595(((1 << n) - 1) << 1); }      // les n premières lumières (Q1 à Qn), n = 0 à 5

// ---------- LED vertes et rouge ----------
void setVertes(uint8_t nb)      { for (uint8_t i = 0; i < NB_VERTES; i++) digitalWrite(VERTES[i], i < nb); }    // les nb premières
void verteSeule(uint8_t numero) { for (uint8_t i = 0; i < NB_VERTES; i++) digitalWrite(VERTES[i], i == numero); }
void setRouge(bool etat)        { digitalWrite(ROUGE_PIN, etat); }
void toutEteindre()             { setVertes(0); setRouge(false); ecrire595(0); }   // tous les indicateurs

// ---------- Buzzer : lecteur de sons non bloquant ----------
const uint16_t NOTE_G3 = 196, NOTE_C4 = 262, NOTE_E4 = 330, NOTE_G4 = 392, NOTE_C5 = 523,
               NOTE_E5 = 659, NOTE_G5 = 784, NOTE_C6 = 1047, NOTE_E6 = 1319, NOTE_G6 = 1568;

const Note*   sonActuel = nullptr;
uint8_t       sonLongueur = 0, sonIndex = 0;
unsigned long tNote = 0, dureeNote = 0;

void sortieBuzzer(uint16_t freq) {
  if (BUZZER_PASSIF && freq) { tone(BUZZER_PIN, freq); return; }
  if (BUZZER_PASSIF) noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, !BUZZER_PASSIF && freq);
}

void lancerSon(const Note* notes, uint8_t longueur) {
  sortieBuzzer(0);
  sonActuel = notes; sonLongueur = longueur; sonIndex = 0;
  tNote = millis(); dureeNote = 0;
}
#define LANCER_SON(t) lancerSon(t, sizeof(t) / sizeof(t[0]))

void majSon() {                                            // à appeler souvent : passe à la note suivante
  if (!sonActuel || millis() - tNote < dureeNote) return;
  if (sonIndex >= sonLongueur) { sortieBuzzer(0); sonActuel = nullptr; return; }
  Note n = sonActuel[sonIndex++];
  sortieBuzzer(n.freq);
  tNote = millis(); dureeNote = n.duree;
}

// ---------- Effets sonores ----------
const Note SON_DEBUT[] = {                                 // ~3,7 s
  {262,120}, {294,120}, {330,120}, {349,120}, {392,120}, {440,120}, {494,120}, {523,120},
  {587,120}, {659,120}, {698,120}, {784,120}, {880,120}, {988,120}, {1047,120}, {1175,120},
  {NOTE_G4,100}, {NOTE_C5,100}, {NOTE_E5,100}, {NOTE_G5,250}, {0,50},
  {NOTE_E5,100}, {NOTE_G5,100}, {NOTE_C6,150}, {NOTE_G5,100}, {NOTE_C6,150}, {NOTE_E6,150}, {NOTE_G6,400}
};
const Note SON_PUISSANCE[] = { {1200,30}, {0,15}, {1600,30} };
const Note SON_TIR[] = {
  {400,20}, {550,20}, {750,20}, {1000,20}, {1300,20}, {1650,20}, {2000,20},
  {1800,22}, {1450,22}, {1150,22}, {900,22}, {700,22}, {550,22}, {420,22}
};
const Note SON_SUCCES[]   = { {NOTE_C6,70}, {NOTE_G6,180} };
const Note SON_ECHEC[]    = { {NOTE_E4,180}, {NOTE_C4,450} };
const Note SON_VICTOIRE[] = { {NOTE_C5,120}, {NOTE_G5,120}, {NOTE_C6,120}, {NOTE_E6,240}, {0,60},
                              {NOTE_C6,120}, {NOTE_E6,120}, {NOTE_G6,500} };
const Note SON_DEFAITE[]  = { {NOTE_G4,250}, {NOTE_E4,250}, {NOTE_C4,250}, {0,100}, {NOTE_G3,700} };

void fanfareDebut()           { LANCER_SON(SON_DEBUT); }
void sonChangementPuissance() { LANCER_SON(SON_PUISSANCE); }
void sonTir()                 { LANCER_SON(SON_TIR); }
void sonSucces()              { LANCER_SON(SON_SUCCES); }
void sonEchec()               { LANCER_SON(SON_ECHEC); }
void fanfareVictoire()        { LANCER_SON(SON_VICTOIRE); }
void fanfareDefaite()         { LANCER_SON(SON_DEFAITE); }

// ---------- Réglages du jeu ----------
// Les 5 lumières de la barre de puissance servent toutes au tir (une de plus toutes les 500 ms).
// Chaque cible est touchée quand on relâche avec un nombre de lumières allumées dans sa plage.
// Une cible peut couvrir 2 lumières pour faciliter le tir. Cibles dans l'ordre de BLEUES (Q7, Q6, Q0).
const uint8_t NIVEAU_DEBUT[] = {1, 3, 4};                  // première lumière qui touche la cible (incluse)
const uint8_t NIVEAU_FIN[]   = {2, 3, 5};                  // dernière lumière qui touche la cible (incluse)
const uint8_t NB_CIBLES  = sizeof(BLEUES);
const uint8_t NB_NIVEAUX = 5;                              // lumières de la barre de puissance (Q1 à Q5)
const unsigned long MS_PAR_NIVEAU = 500;                   // une lumière de plus toutes les 500 ms
const unsigned long TEMPS_MAX_MS  = (NB_NIVEAUX + 1) * MS_PAR_NIVEAU;   // 3000 ms : au-delà de la 5e lumière = hors circuit
const unsigned long DUREE_CIBLE_MS = 1000, DUREE_VOL_MS = 300, PAUSE_FIN_RONDE_MS = 1000;
const unsigned long PAUSE_ENTRE_RONDES_MS = 400;           // noir avant chaque cible : la même cible refait un clignotement visible
const uint8_t  POINTS_VICTOIRE = 3, PRISES_DEFAITE = 2;     // 3 points = partie gagnée, 2 prises = partie perdue
const unsigned long PAUSE_FIN_PARTIE_MS = 4000;            // attente avant de recommencer une partie

uint8_t points = 0, prises = 0;                            // points = LED vertes allumées, prises = LED rouge

uint8_t niveauDe(unsigned long ms) {                       // nombre de lumières allumées après 'ms' de maintien
  unsigned long n = ms / MS_PAR_NIVEAU;
  return n > NB_NIVEAUX ? NB_NIVEAUX : n;
}

// ---------- Bouton (INPUT_PULLUP : LOW = enfoncé), avec anti-rebond ----------
bool etatBoutonStable = HIGH, dernierEtat = HIGH;
unsigned long tChangement = 0;                             // instant du dernier changement brut du bouton

bool boutonEnfonce() {                                     // à appeler souvent
  bool lecture = digitalRead(BOUTON_PIN);
  if (lecture != dernierEtat) { dernierEtat = lecture; tChangement = millis(); }
  if (millis() - tChangement >= DELAI_ANTIREBOND) etatBoutonStable = lecture;
  return etatBoutonStable == LOW;
}

void reinitialiserBouton() {                               // oublie tout ce qui s'est passé avant
  dernierEtat = etatBoutonStable = digitalRead(BOUTON_PIN);
  tChangement = millis();
}

// ---------- Attente qui garde le son en marche (remplace delay) ----------
void attendre(unsigned long ms) {
  unsigned long debut = millis();
  while (millis() - debut < ms) { majSon(); yield(); }
}

// ---------- Début de partie ----------
void debutDePartie() {
  fanfareDebut();                                          // joue pendant toute la séquence

  for (uint8_t i = 0; i < 5; i++) {                        // 1. bandeau : Q1 à Q5, une lumière à la fois
    stripUneSeule(i); attendre(500);
    ecrire595(0);     attendre(250);
  }
  for (uint8_t i = 0; i < 3; i++) {                        // 2. LED bleues : Q7, Q6, Q0
    bleueSeule(i);    attendre(200);
    ecrire595(0);     attendre(i < 2 ? 250 : 100);
  }
  setRouge(true);  attendre(250);                          // 3. LED rouge
  setRouge(false); attendre(100);
  for (uint8_t i = 0; i < 3; i++) {                        // 4. LED vertes, une à la fois
    verteSeule(i);    attendre(i < 2 ? 200 : 300);
    setVertes(0);     attendre(i < 2 ? 250 : 100);
  }

  toutEteindre();                                          // 5. tout éteint, puis courte inactivité
  points = prises = 0;
  attendre(INACTIVITE_MS);
}

// ---------- Ronde (un tour de jeu) ----------
void ronde() {
  // 1. Court noir (pour que la même cible fasse un clignotement), puis cible au hasard :
  //    sa LED bleue brille 1 s, le bouton n'est pas lu pendant ce temps
  ecrire595(0);  attendre(PAUSE_ENTRE_RONDES_MS);
  uint8_t cible = random(NB_CIBLES);
  bleueSeule(cible);  attendre(DUREE_CIBLE_MS);
  ecrire595(0);

  // 2. Tout appui fait avant est ignoré : un bouton déjà enfoncé doit être relâché, puis réenfoncé
  reinitialiserBouton();
  while (boutonEnfonce())  { majSon(); yield(); }
  while (!boutonEnfonce()) { majSon(); yield(); }

  // 3. Charge de la puissance tant que le bouton est enfoncé (un bip à chaque lumière de plus)
  unsigned long debut = tChangement, tenu = 0;
  uint8_t niveau = 0;
  bool max = false;
  while (boutonEnfonce()) {
    tenu = millis() - debut;
    if (niveauDe(tenu) != niveau) { niveau = niveauDe(tenu); stripBarre(niveau); sonChangementPuissance(); }
    if (tenu >= TEMPS_MAX_MS) { max = true; break; }       // maximum atteint : le tir part tout seul
    majSon(); yield();
  }
  if (!max) {                                              // relâché : on prend l'instant réel du relâchement
    tenu = tChangement - debut;
    stripBarre(niveauDe(tenu));
  }

  // 4. Tir : bruit de vol, la barre de puissance reste affichée
  sonTir();  attendre(DUREE_VOL_MS);

  // 5. Résultat : la LED de la cible touchée s'allume (aucune si raté ou hors circuit)
  int8_t touche = -1;
  if (!max) {
    uint8_t n = niveauDe(tenu);                            // lumières allumées au relâchement
    for (uint8_t i = 0; i < NB_CIBLES; i++) {
      if (n >= NIVEAU_DEBUT[i] && n <= NIVEAU_FIN[i]) { touche = i; break; }
    }
  }
  ecrire595(touche >= 0 ? BLEUES[touche] : 0);

  if (touche == cible) {                                   // succès : un point, la LED verte clignote puis reste allumée
    sonSucces();
    if (points < NB_VERTES) points++;
    for (uint8_t i = 0; i < 3; i++) { setVertes(points); attendre(150); setVertes(points - 1); attendre(150); }
    setVertes(points);
  } else {                                                 // échec : une prise, la LED rouge clignote puis reste allumée
    sonEchec();
    if (prises < 255) prises++;
    for (uint8_t i = 0; i < 3; i++) { setRouge(true); attendre(150); setRouge(false); attendre(150); }
    setRouge(true);
  }

  attendre(PAUSE_FIN_RONDE_MS);
  ecrire595(0);
}

// ---------- Après chaque ronde : la partie est-elle terminée ? ----------
void postRonde() {
  bool gagnee = points >= POINTS_VICTOIRE;
  bool perdue = prises >= PRISES_DEFAITE;
  if (!gagnee && !perdue) return;                          // partie en cours : loop() lance une nouvelle ronde

  toutEteindre();
  if (gagnee) {                                            // victoire : LED vertes en alternance + fanfare
    fanfareVictoire();
    for (uint8_t i = 0; i < 8; i++) {
      for (uint8_t j = 0; j < NB_VERTES; j++) digitalWrite(VERTES[j], (i + j) % 2 == 0);
      attendre(200);
    }
  } else {                                                 // défaite : LED rouge qui clignote + fanfare
    fanfareDefaite();
    for (uint8_t i = 0; i < 8; i++) { setRouge(i % 2 == 0); attendre(200); }
  }

  toutEteindre();
  attendre(PAUSE_FIN_PARTIE_MS);
  debutDePartie();                                         // nouvelle partie (remet points et prises à 0)
}

// ---------- Programme principal ----------
void setup() {
  delay(DELAI_DEMARRAGE_MS);                               // laisse la carte et les composants s'initialiser

  pinMode(DS_PIN, OUTPUT); pinMode(SHCP_PIN, OUTPUT); pinMode(STCP_PIN, OUTPUT);
  ecrire595(0);

  for (uint8_t i = 0; i < NB_VERTES; i++) pinMode(VERTES[i], OUTPUT);
  setVertes(0);
  pinMode(ROUGE_PIN, OUTPUT);  setRouge(false);
  pinMode(BOUTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  pinMode(BANDEAU_PIN, OUTPUT); digitalWrite(BANDEAU_PIN, LOW);   // D4 : inutilisée, simplement à LOW

  randomSeed(analogRead(A0));                              // cible choisie au hasard à chaque ronde

  delay(DELAI_PREPARATION_MS);
  debutDePartie();
}

void loop() {
  ronde();
  postRonde();
}
