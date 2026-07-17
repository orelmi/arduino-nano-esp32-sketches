# BluetoothScan — Scanner Bluetooth LE pour Arduino Nano ESP32

Ce sketch scanne en continu les périphériques **Bluetooth Low Energy (BLE)**
à portée et affiche leurs informations dans le moniteur série.

## Matériel

- **Arduino Nano ESP32** (puce ESP32-S3, antenne Bluetooth intégrée).

> ⚠️ L'ESP32-S3 supporte **uniquement le Bluetooth Low Energy (BLE)**, pas
> le Bluetooth Classic. Les périphériques Bluetooth Classic (ex. certaines
> enceintes, manettes) n'apparaîtront donc **pas** dans le scan.

## Installation

1. Dans l'IDE Arduino, installer le paquet de cartes **esp32** d'Espressif
   (Gestionnaire de cartes → rechercher « esp32 »). La bibliothèque
   *ESP32 BLE Arduino* est incluse.
2. Sélectionner la carte **Arduino Nano ESP32**.
3. Téléverser le sketch `BluetoothScan.ino`.

## Utilisation

1. Ouvrir le **moniteur série** à **115200 bauds**.
2. Chaque cycle de scan dure 5 secondes puis affiche la liste des
   périphériques détectés.

## Exemple de sortie

```
=== Scanner Bluetooth LE - Arduino Nano ESP32 ===
Initialisation de la pile BLE...
Initialisation terminée.

Scan en cours (5 s)...
  -> Adresse: aa:bb:cc:dd:ee:ff | RSSI: -67 dBm | Nom: Mi Band 6 | Fabricant: Apple [hex: 02 15 ... | txt: ..]
  -> Adresse: 11:22:33:44:55:66 | RSSI: -80 dBm | Service UUID: 0000fe9f-0000-1000-8000-00805f9b34fb
Scan terminé : 2 périphérique(s) trouvé(s).
--------------------------------------------------
```

## Informations affichées par périphérique

- **Adresse** : adresse MAC BLE du périphérique.
- **RSSI** : puissance du signal reçu (en dBm ; plus proche de 0 = plus proche).
- **Nom** : nom diffusé (si présent).
- **Service UUID** : UUID de service annoncé (si présent).
- **Fabricant** : nom du fabricant décodé depuis l'identifiant de société
  Bluetooth SIG (2 premiers octets, little-endian). Suivi des octets restants
  en **hexadécimal** (`hex:`) et en **texte lisible** (`txt:`, les octets non
  imprimables sont remplacés par `.`). Les iBeacon Apple sont signalés.

### Décodage des données fabricant

Les données fabricant BLE commencent par un **identifiant de société** de
2 octets attribué par le Bluetooth SIG. Le sketch connaît les principaux
fabricants (Apple, Google, Samsung, Microsoft, Xiaomi, Garmin, Espressif…) ;
pour les autres, l'identifiant est affiché sous forme `code 0xXXXX`. La liste
officielle complète est disponible ici :
<https://www.bluetooth.com/specifications/assigned-numbers/>

Pour ajouter un fabricant, complétez la fonction `companyName()` dans
`BluetoothScan.ino`.

## Réglages

Modifiables en haut de `BluetoothScan.ino` :

- `SCAN_DURATION_SECONDS` : durée d'un scan (défaut : 5 s).
- `ACTIVE_SCAN` : scan actif pour récupérer le nom (défaut : activé).
- `PAUSE_BETWEEN_SCANS_MS` : pause entre deux scans (défaut : 2000 ms).
