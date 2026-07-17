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
      Serial.print(" | Fabricant: ");
      Serial.print(advertisedDevice.getManufacturerData().length());
      Serial.print(" octet(s)");
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
