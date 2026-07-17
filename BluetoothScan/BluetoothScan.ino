/*
 * BluetoothScan.ino
 *
 * Scanner Bluetooth Low Energy (BLE) INTERACTIF pour une carte
 * Arduino Nano ESP32 (puce ESP32-S3), piloté par des commandes
 * envoyées via la liaison série USB.
 *
 * IMPORTANT : l'ESP32-S3 de l'Arduino Nano ESP32 ne supporte QUE le
 * Bluetooth Low Energy (BLE), pas le Bluetooth Classic. Ce sketch
 * utilise donc la pile BLE (bibliothèque "ESP32 BLE Arduino").
 *
 * Utilisation :
 *   1. Sélectionner la carte "Arduino Nano ESP32" dans l'IDE Arduino.
 *   2. Téléverser ce sketch.
 *   3. Ouvrir le moniteur série à 115200 bauds, "Nouvelle ligne" activée.
 *   4. Taper "help" puis Entrée pour voir les commandes disponibles.
 */

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Réglages par défaut (modifiables en direct via les commandes série).
// ---------------------------------------------------------------------------

// Nombre maximum de périphériques mémorisés par scan.
static const int  MAX_DEVICES = 64;

// Durée d'un scan, en secondes.
static int   g_scanSeconds = 5;
// Scan actif : demande aussi le "scan response" (nom, etc.).
static const bool ACTIVE_SCAN = true;
// Mode scan automatique en boucle (désactivé par défaut : on est interactif).
static bool  g_autoScan = false;

// Calibration RSSI -> distance (modèle log-distance path loss).
//   distance(m) = 10 ^ ((RSSI_1m - RSSI) / (10 * n))
// RSSI_1m : puissance reçue à 1 m (dépend du périphérique, ~ -59 dBm typique).
// n       : exposant d'atténuation (2 = espace libre, 2,7 à 4 en intérieur).
static float g_rssiAt1m    = -59.0f;
static float g_pathLoss    = 2.0f;

// ---------------------------------------------------------------------------

BLEScan* pBLEScan = nullptr;

// Informations mémorisées pour chaque périphérique détecté.
struct DeviceInfo {
  String  address;
  String  name;
  int     rssi;
  String  manufacturer;  // chaîne décodée (vide si absente)
};

static DeviceInfo g_devices[MAX_DEVICES];
static int        g_deviceCount = 0;

// ---------------------------------------------------------------------------
// Décodage des données fabricant.
// ---------------------------------------------------------------------------

// Nom du fabricant à partir de l'identifiant de société Bluetooth SIG
// (2 premiers octets des données fabricant, en little-endian).
// Référence : https://www.bluetooth.com/specifications/assigned-numbers/
static const char* companyName(uint16_t companyId) {
  switch (companyId) {
    case 0x004C: return "Apple";
    case 0x0006: return "Microsoft";
    case 0x00E0: return "Google";
    case 0x0075: return "Samsung";
    case 0x0087: return "Garmin";
    case 0x0157: return "Huami (Amazfit/Xiaomi)";
    case 0x038F: return "Xiaomi";
    case 0x0499: return "Ruuvi";
    case 0x0059: return "Nordic Semiconductor";
    case 0x02E5: return "Espressif";
    case 0x004F: return "Logitech";
    case 0x000F: return "Broadcom";
    case 0x0001: return "Ericsson";
    case 0x000D: return "Texas Instruments";
    case 0x0078: return "Nike";
    case 0x0171: return "Amazon";
    case 0x0110: return "Sony";
    case 0x00C4: return "LG Electronics";
    case 0x0131: return "Cypress";
    default:     return nullptr;
  }
}

// Ajoute un octet en hexadécimal (2 chiffres) à une chaîne.
static void appendHexByte(String& out, uint8_t b) {
  const char* hex = "0123456789ABCDEF";
  out += hex[b >> 4];
  out += hex[b & 0x0F];
}

// Construit une chaîne lisible décrivant les données fabricant :
// nom du fabricant + octets restants en hexadécimal et en texte.
static String manufacturerToString(const std::string& data) {
  const uint8_t* bytes = (const uint8_t*)data.data();
  size_t length = data.length();
  String out;

  if (length >= 2) {
    uint16_t companyId = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    const char* name = companyName(companyId);

    if (name != nullptr) {
      out += name;
    } else {
      out += "code 0x";
      appendHexByte(out, bytes[1]);
      appendHexByte(out, bytes[0]);
    }

    // Cas particulier : iBeacon Apple (0x02 0x15 après l'ID société).
    if (companyId == 0x004C && length >= 4 && bytes[2] == 0x02 && bytes[3] == 0x15) {
      out += " (iBeacon)";
    }

    if (length > 2) {
      out += " [hex: ";
      for (size_t i = 2; i < length; i++) {
        appendHexByte(out, bytes[i]);
        if (i + 1 < length) out += ' ';
      }
      out += " | txt: ";
      for (size_t i = 2; i < length; i++) {
        char c = (char)bytes[i];
        out += (c >= 0x20 && c <= 0x7E) ? c : '.';
      }
      out += ']';
    }
  } else {
    out += "[hex: ";
    for (size_t i = 0; i < length; i++) {
      appendHexByte(out, bytes[i]);
      if (i + 1 < length) out += ' ';
    }
    out += ']';
  }
  return out;
}

// ---------------------------------------------------------------------------
// RSSI -> distance.
// ---------------------------------------------------------------------------

// Estimation de la distance en centimètres à partir du RSSI.
// Renvoie une valeur négative si le calcul n'est pas possible.
static float rssiToDistanceCm(int rssi) {
  if (rssi == 0) return -1.0f;
  float meters = powf(10.0f, (g_rssiAt1m - (float)rssi) / (10.0f * g_pathLoss));
  return meters * 100.0f;
}

// ---------------------------------------------------------------------------
// Gestion de la liste des périphériques.
// ---------------------------------------------------------------------------

// Ajoute ou met à jour un périphérique (dédoublonnage par adresse).
static void addOrUpdateDevice(const String& address, const String& name,
                              int rssi, const String& manufacturer) {
  for (int i = 0; i < g_deviceCount; i++) {
    if (g_devices[i].address == address) {
      g_devices[i].rssi = rssi;                       // dernière mesure
      if (name.length() > 0)          g_devices[i].name = name;
      if (manufacturer.length() > 0)  g_devices[i].manufacturer = manufacturer;
      return;
    }
  }
  if (g_deviceCount >= MAX_DEVICES) return;            // liste pleine
  g_devices[g_deviceCount].address      = address;
  g_devices[g_deviceCount].name         = name;
  g_devices[g_deviceCount].rssi         = rssi;
  g_devices[g_deviceCount].manufacturer = manufacturer;
  g_deviceCount++;
}

// Tri décroissant par RSSI (le plus fort = le plus proche) — tri par insertion.
static void sortDevicesByRssiDesc() {
  for (int i = 1; i < g_deviceCount; i++) {
    DeviceInfo key = g_devices[i];
    int j = i - 1;
    while (j >= 0 && g_devices[j].rssi < key.rssi) {
      g_devices[j + 1] = g_devices[j];
      j--;
    }
    g_devices[j + 1] = key;
  }
}

// Met en forme la distance estimée dans un tampon (ex. "~35 cm", "~1.20 m").
static void formatDistance(int rssi, char* buf, size_t n) {
  float cm = rssiToDistanceCm(rssi);
  if (cm < 0)            snprintf(buf, n, "~?");
  else if (cm < 100.0f)  snprintf(buf, n, "~%.0f cm", cm);
  else                   snprintf(buf, n, "~%.2f m", cm / 100.0f);
}

// Barre horizontale composée de '=' de la largeur du tableau.
static void printSeparator() {
  Serial.println("---------------------------------------------------------------");
}

// Affiche une ligne alignée pour un périphérique classé.
static void printDeviceRow(int rank, const DeviceInfo& d) {
  char dist[16];
  formatDistance(d.rssi, dist, sizeof(dist));

  // Colonnes : rang | adresse | RSSI | distance | nom
  Serial.printf(" %2d  %-17s  %4d dBm  %-9s  %s\n",
                rank, d.address.c_str(), d.rssi, dist,
                d.name.length() > 0 ? d.name.c_str() : "(sans nom)");

  // Le fabricant (souvent long) est affiché sur une sous-ligne indentée.
  if (d.manufacturer.length() > 0) {
    Serial.print("       > Fabricant: ");
    Serial.println(d.manufacturer);
  }
}

// Liste les n périphériques au signal le plus fort (n <= 0 : tous).
static void listDevices(int n) {
  if (g_deviceCount == 0) {
    Serial.println("(vide) Aucun périphérique en mémoire. Lancez 'scan' d'abord.");
    return;
  }
  sortDevicesByRssiDesc();
  int count = (n <= 0 || n > g_deviceCount) ? g_deviceCount : n;

  Serial.println();
  Serial.printf("  %d appareil(s) le(s) plus proche(s) sur %d detecte(s)\n",
                count, g_deviceCount);
  Serial.println("  (RSSI le plus fort = le plus proche ; distance estimee)");
  printSeparator();
  Serial.println("  #  Adresse             RSSI     Distance   Nom");
  printSeparator();
  for (int i = 0; i < count; i++) {
    printDeviceRow(i + 1, g_devices[i]);
  }
  printSeparator();
}

// ---------------------------------------------------------------------------
// Callback de scan : alimente la liste des périphériques.
// ---------------------------------------------------------------------------

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    String address = advertisedDevice.getAddress().toString().c_str();
    String name    = advertisedDevice.haveName()
                       ? String(advertisedDevice.getName().c_str()) : String("");
    String manuf   = advertisedDevice.haveManufacturerData()
                       ? manufacturerToString(advertisedDevice.getManufacturerData())
                       : String("");
    addOrUpdateDevice(address, name, advertisedDevice.getRSSI(), manuf);
  }
};

// ---------------------------------------------------------------------------
// Actions.
// ---------------------------------------------------------------------------

static void doScan(int seconds) {
  g_deviceCount = 0;  // repart d'une liste vide
  Serial.printf("Scan en cours (%d s)...\n", seconds);
  pBLEScan->start(seconds, false);
  pBLEScan->clearResults();
  Serial.printf("Scan terminé : %d périphérique(s) détecté(s).\n", g_deviceCount);
}

// Bannière d'accueil (affichée au démarrage).
static void printBanner() {
  Serial.println();
  Serial.println("  +-------------------------------------------------------+");
  Serial.println("  |        Scanner Bluetooth LE - Arduino Nano ESP32      |");
  Serial.println("  |          Mode interactif (liaison serie USB)         |");
  Serial.println("  +-------------------------------------------------------+");
}

// Affiche l'état courant des réglages.
static void printStatus() {
  Serial.printf("  Etat : duree scan=%ds | auto=%s | RSSI@1m=%.0f dBm | n=%.2f | %d en memoire\n",
                g_scanSeconds, g_autoScan ? "ON" : "OFF",
                g_rssiAt1m, g_pathLoss, g_deviceCount);
}

static void printHelp() {
  Serial.println();
  Serial.println("  === COMMANDES ===");
  Serial.println("  help              Affiche ce menu.");
  Serial.println("  scan [s]          Lance un scan de [s] secondes (defaut: courant).");
  Serial.println("  list [N]          Liste les N appareils au signal le plus fort");
  Serial.println("                    (le plus proche). Sans N : tous.");
  Serial.println("  top N             Alias de 'list N'.");
  Serial.println("  auto [s]          Scan automatique en boucle toutes les [s] s.");
  Serial.println("                    'auto 0' ou 'auto off' pour arreter.");
  Serial.println("  calib [r] [n]     Calibration distance : r = RSSI a 1 m (dBm),");
  Serial.println("                    n = exposant d'attenuation. Sans arg : affiche.");
  Serial.println("  status            Affiche les reglages courants.");
  Serial.println("  clear             Vide la liste des peripheriques.");
  Serial.println();
  printStatus();
  Serial.println();
}

// Invite de saisie (affichée quand on attend une commande).
static void printPrompt() {
  if (!g_autoScan) Serial.print("\nBLE> ");
}

// ---------------------------------------------------------------------------
// Analyse des commandes série.
// ---------------------------------------------------------------------------

static void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  // Découpe "commande argument1 argument2".
  String cmd = line, a1 = "", a2 = "";
  int sp1 = line.indexOf(' ');
  if (sp1 >= 0) {
    cmd = line.substring(0, sp1);
    String rest = line.substring(sp1 + 1);
    rest.trim();
    int sp2 = rest.indexOf(' ');
    if (sp2 >= 0) {
      a1 = rest.substring(0, sp2);
      a2 = rest.substring(sp2 + 1);
      a2.trim();
    } else {
      a1 = rest;
    }
  }
  cmd.toLowerCase();

  if (cmd == "help" || cmd == "h" || cmd == "?") {
    printHelp();

  } else if (cmd == "scan") {
    if (a1.length() > 0) g_scanSeconds = max(1, (int)a1.toInt());
    doScan(g_scanSeconds);
    listDevices(0);

  } else if (cmd == "list" || cmd == "top") {
    int n = (a1.length() > 0) ? (int)a1.toInt() : 0;
    listDevices(n);

  } else if (cmd == "auto") {
    if (a1 == "off" || a1 == "0") {
      g_autoScan = false;
      Serial.println("Scan automatique : OFF.");
    } else {
      if (a1.length() > 0) g_scanSeconds = max(1, (int)a1.toInt());
      g_autoScan = true;
      Serial.printf("Scan automatique : ON (toutes les %d s). 'auto off' pour arrêter.\n",
                    g_scanSeconds);
    }

  } else if (cmd == "calib") {
    if (a1.length() > 0) g_rssiAt1m = a1.toFloat();
    if (a2.length() > 0) g_pathLoss = a2.toFloat();
    Serial.printf("Calibration : RSSI@1m=%.0f dBm, n=%.2f\n", g_rssiAt1m, g_pathLoss);

  } else if (cmd == "status") {
    printStatus();

  } else if (cmd == "clear") {
    g_deviceCount = 0;
    Serial.println("Liste videe.");

  } else {
    Serial.print("Commande inconnue : '");
    Serial.print(cmd);
    Serial.println("'. Tapez 'help'.");
  }
}

// Lit une ligne complète sur le port série (non bloquant).
static void pollSerial() {
  static String buffer;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (buffer.length() > 0) {
        Serial.println(buffer);       // écho de la commande saisie
        handleCommand(buffer);
        buffer = "";
        printPrompt();                // invite pour la commande suivante
      }
    } else {
      buffer += c;
      if (buffer.length() > 80) buffer = "";  // garde-fou anti-débordement
    }
  }
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Attente de l'ouverture du moniteur série (USB natif).
  }

  printBanner();
  Serial.println("  Initialisation de la pile BLE...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  pBLEScan->setActiveScan(ACTIVE_SCAN);
  pBLEScan->setInterval(100);   // unités de 0,625 ms
  pBLEScan->setWindow(99);

  Serial.println("  Pret. Tapez 'help' pour la liste des commandes.");
  printHelp();
  printPrompt();
}

void loop() {
  if (g_autoScan) {
    doScan(g_scanSeconds);
    listDevices(0);
  }
  pollSerial();
  delay(20);
}
