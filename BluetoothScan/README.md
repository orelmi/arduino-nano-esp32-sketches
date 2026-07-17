# BluetoothScan — Scanner Bluetooth LE pour Arduino Nano ESP32

Scanner **Bluetooth Low Energy (BLE)** interactif, piloté par des commandes
tapées dans le moniteur série. Permet de lancer des scans à la demande, de
lister les N appareils au signal le plus fort (les plus proches) et d'estimer
leur distance à partir du RSSI.

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
2. Régler la fin de ligne sur **« Nouvelle ligne » (`\n`)** dans le moniteur.
3. Taper une commande puis Entrée. Commencez par `help`.

Invite affichée : `BLE> `. La commande saisie est ré-affichée (écho) pour
un suivi clair même si le moniteur série n'affiche pas la frappe.

## Commandes

| Commande        | Description                                                        |
|-----------------|--------------------------------------------------------------------|
| `help`          | Affiche le menu des commandes.                                     |
| `scan [s]`      | Lance un scan de `s` secondes (défaut : durée courante).           |
| `list [N]`      | Liste les `N` appareils au signal le plus fort. Sans `N` : tous.   |
| `top N`         | Alias de `list N`.                                                 |
| `auto [s]`      | Scan automatique en boucle toutes les `s` s. `auto off` pour arrêter. |
| `calib [r] [n]` | Calibration distance : `r` = RSSI à 1 m (dBm), `n` = exposant.     |
| `status`        | Affiche les réglages courants.                                     |
| `clear`         | Vide la liste des périphériques.                                   |

## Exemple de session

```
  +-------------------------------------------------------+
  |        Scanner Bluetooth LE - Arduino Nano ESP32      |
  |          Mode interactif (liaison serie USB)         |
  +-------------------------------------------------------+
  Pret. Tapez 'help' pour la liste des commandes.

BLE> scan 5
scan 5
Scan en cours (5 s)...
Scan terminé : 4 périphérique(s) détecté(s).

  3 appareil(s) le(s) plus proche(s) sur 4 detecte(s)
  (RSSI le plus fort = le plus proche ; distance estimee)
---------------------------------------------------------------
  #  Adresse             RSSI     Distance   Nom
---------------------------------------------------------------
  1  e7:63:26:3f:5a:e4   -55 dBm  ~35 cm     JBL Tune 520BT-LE
  2  aa:bb:cc:dd:ee:ff   -67 dBm  ~1.40 m    (sans nom)
       > Fabricant: Apple (iBeacon) [hex: ... | txt: ..]
  3  11:22:33:44:55:66   -80 dBm  ~6.30 m    Mi Band 6
---------------------------------------------------------------

BLE> top 1
top 1
...
```

## Informations affichées par périphérique

- **Adresse** : adresse MAC BLE du périphérique.
- **RSSI** : puissance du signal reçu (dBm ; plus proche de 0 = plus proche).
- **Distance** : estimation en cm/m à partir du RSSI (voir plus bas).
- **Nom** : nom diffusé (ou `(sans nom)`).
- **Fabricant** (sous-ligne) : nom décodé + octets restants en hexadécimal et
  en texte.

## Estimation de la distance (RSSI → cm)

La distance est estimée avec le modèle log-distance :

```
distance(m) = 10 ^ ((RSSI_1m - RSSI) / (10 × n))
```

- `RSSI_1m` : puissance reçue à 1 m (défaut **−59 dBm**).
- `n` : exposant d'atténuation (défaut **2.0** ; 2 = espace libre, 2,7 à 4 en intérieur).

Réglable en direct avec `calib`, par exemple `calib -62 2.5`.

> ⚠️ **C'est une estimation grossière.** Le RSSI fluctue beaucoup (obstacles,
> orientation de l'antenne, puissance d'émission propre à chaque appareil). La
> distance affichée donne un ordre de grandeur, pas une mesure précise. Pour de
> meilleurs résultats, calibrez `RSSI_1m` en plaçant un appareil connu à 1 m et
> en relevant son RSSI.

## Décodage des données fabricant

Les données fabricant BLE commencent par un **identifiant de société** de
2 octets attribué par le Bluetooth SIG. Le sketch connaît les principaux
fabricants (Apple, Google, Samsung, Microsoft, Xiaomi, Garmin, Espressif…) ;
pour les autres, l'identifiant est affiché sous forme `code 0xXXXX`. La liste
officielle complète est disponible ici :
<https://www.bluetooth.com/specifications/assigned-numbers/>

Pour ajouter un fabricant, complétez la fonction `companyName()` dans
`BluetoothScan.ino`.

> Note : pour la plupart des appareils grand public, les octets après
> l'identifiant sont chiffrés ou aléatoires (protection de la vie privée). Le
> champ texte n'est réellement lisible que sur certains capteurs/beacons.

## Réglages par défaut

Modifiables en haut de `BluetoothScan.ino` (et la plupart en direct via les
commandes) :

- `g_scanSeconds` : durée d'un scan (défaut : 5 s) — cf. `scan`, `auto`.
- `ACTIVE_SCAN` : scan actif pour récupérer le nom (défaut : activé).
- `g_rssiAt1m` / `g_pathLoss` : calibration distance — cf. `calib`.
- `MAX_DEVICES` : nombre maximum d'appareils mémorisés par scan (défaut : 64).
