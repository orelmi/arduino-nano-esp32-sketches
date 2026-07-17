/*
 * BluetoothScan.ino
 *
 * Scan des periphériques Bluetooth Low Energy (BLE) avec une carte
 * Arduino Nano ESP32 (puce ESP32-S3) et affichage des résultats
 * dans le moniteur série.
 *
 * IMPORTANT : l'ESP32-S3 de l'Arduino Nano ESP32 ne supporte QUE le
 * Bluetooth Low Energy (BLE), pas le Bluetooth Classic. Ce sketch
 * utilise donc la pile BLE (bibliothèque "ESP32 BLE Arduino", fournie
 * avec le paquet de cartes esp32 d'Espressif).
 *
 * Câblage : aucun, l'antenne Bluetooth est intégrée à la carte.
 *
 * Utilisation :
 *   1. Sélectionner la carte "Arduino Nano ESP32" dans l'IDE Arduino.
 *   2. Téléverser ce sketch.
 *   3. Ouvrir le moniteur série à 115200 bauds.
 */

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Durée d'un scan, en secondes.
static const int   SCAN_DURATION_SECONDS = 5;
// Scan actif : demande aussi le "scan response" (nom du périphérique, etc.).
static const bool  ACTIVE_SCAN           = true;
// Pause entre deux scans, en millisecondes.
static const int   PAUSE_BETWEEN_SCANS_MS = 2000;

BLEScan* pBLEScan = nullptr;

// Retourne le nom du fabricant à partir de l'identifiant de société
// Bluetooth SIG (les 2 premiers octets des données fabricant, en
// little-endian). Liste des principaux fabricants ; les autres sont
// affichés sous forme de code hexadécimal.
// Référence complète : https://www.bluetooth.com/specifications/assigned-numbers/
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

// Affiche un tableau d'octets sous forme hexadécimale (ex. "4C 00 02 15").
static void printHex(const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
    if (i + 1 < length) {
      Serial.print(' ');
    }
  }
}

// Affiche les octets imprimables sous forme de texte ('.' pour les autres).
static void printAscii(const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    char c = (char)data[i];
    Serial.print((c >= 0x20 && c <= 0x7E) ? c : '.');
  }
}

// Décode et affiche les données fabricant : nom du fabricant si connu,
// puis les octets restants en hexadécimal et en texte lisible.
static void printManufacturerData(const std::string& data) {
  const uint8_t* bytes = (const uint8_t*)data.data();
  size_t length = data.length();

  Serial.print(" | Fabricant: ");

  if (length >= 2) {
    uint16_t companyId = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    const char* name = companyName(companyId);

    if (name != nullptr) {
      Serial.print(name);
    } else {
      Serial.print("code 0x");
      if (companyId < 0x1000) Serial.print('0');
      if (companyId < 0x0100) Serial.print('0');
      if (companyId < 0x0010) Serial.print('0');
      Serial.print(companyId, HEX);
    }

    // Cas particulier : iBeacon Apple (0x02 0x15 après l'ID société).
    if (companyId == 0x004C && length >= 4 && bytes[2] == 0x02 && bytes[3] == 0x15) {
      Serial.print(" (iBeacon)");
    }

    // Octets restants après l'identifiant de société.
    if (length > 2) {
      Serial.print(" [hex: ");
      printHex(bytes + 2, length - 2);
      Serial.print(" | txt: ");
      printAscii(bytes + 2, length - 2);
      Serial.print("]");
    }
  } else {
    // Données trop courtes pour contenir un identifiant de société.
    Serial.print("[hex: ");
    printHex(bytes, length);
    Serial.print("]");
  }
}

// Callback appelé pour chaque périphérique détecté pendant le scan.
class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    Serial.print("  -> ");
    Serial.print("Adresse: ");
    Serial.print(advertisedDevice.getAddress().toString().c_str());

    Serial.print(" | RSSI: ");
    Serial.print(advertisedDevice.getRSSI());
    Serial.print(" dBm");

    if (advertisedDevice.haveName()) {
      Serial.print(" | Nom: ");
      Serial.print(advertisedDevice.getName().c_str());
    }

    if (advertisedDevice.haveServiceUUID()) {
      Serial.print(" | Service UUID: ");
      Serial.print(advertisedDevice.getServiceUUID().toString().c_str());
    }

    if (advertisedDevice.haveManufacturerData()) {
      printManufacturerData(advertisedDevice.getManufacturerData());
    }

    Serial.println();
  }
};

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Attente de l'ouverture du moniteur série (utile sur USB natif).
  }

  Serial.println();
  Serial.println("=== Scanner Bluetooth LE - Arduino Nano ESP32 ===");
  Serial.println("Initialisation de la pile BLE...");

  BLEDevice::init("");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  pBLEScan->setActiveScan(ACTIVE_SCAN);
  // Réglages recommandés pour un scan fiable (valeurs en unités de 0,625 ms).
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  Serial.println("Initialisation terminée.");
  Serial.println();
}

void loop() {
  Serial.printf("Scan en cours (%d s)...\n", SCAN_DURATION_SECONDS);

  BLEScanResults foundDevices = pBLEScan->start(SCAN_DURATION_SECONDS, false);

  Serial.printf("Scan terminé : %d périphérique(s) trouvé(s).\n",
                foundDevices.getCount());
  Serial.println("--------------------------------------------------");

  // Libère la mémoire utilisée par les résultats du scan.
  pBLEScan->clearResults();

  delay(PAUSE_BETWEEN_SCANS_MS);
}
