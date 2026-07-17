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
   (Gestionnaire de cartes → rechercher « esp32 »). Les bibliothèques
   *ESP32 BLE Arduino* et *WiFi* sont incluses.
2. Installer la bibliothèque **PubSubClient** (Nick O'Leary) via le
   Gestionnaire de bibliothèques (nécessaire pour MQTT).
3. Sélectionner la carte **Arduino Nano ESP32**.
4. Téléverser le sketch `BluetoothScan.ino`.

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
| `identify`      | Passe en mode identification (jumelage iPhone). Alias : `pair`.    |
| `stop`          | Quitte le mode identification, revient au mode scan.               |
| `wifi <ssid> <mdp>` | Connecte le WiFi. Sans argument : affiche l'état et l'IP.      |
| `mqtt <host> [port]` | Configure et connecte le broker MQTT (port 1883 par défaut). |
| `topic [nom]`   | Change le topic de publication (défaut : nom de la carte).         |
| `pub`           | Lance un scan et publie la liste des devices en JSON sur MQTT.     |

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

## Mode identification (jumelage iPhone)

Ce mode transforme la carte en **périphérique BLE** auquel votre iPhone se
connecte pour s'identifier. C'est un « truc bidon » : quand le bon code est
reçu, la carte affiche un message et **la LED RGB passe au vert**.

> Comme iOS **randomise son adresse MAC**, on ne peut pas identifier un iPhone
> de façon fiable par son adresse. On l'identifie donc par un **code secret**
> que vous écrivez depuis le téléphone.

### Étapes

1. Dans le moniteur série, tapez `identify`.
2. Sur l'iPhone, installez une app BLE générique gratuite :
   **nRF Connect** (Nordic) ou **LightBlue** (Punch Through).
3. Dans l'app, scannez puis connectez-vous au périphérique **`NanoESP32-ID`**.
4. Ouvrez le service d'identification, trouvez la caractéristique
   **en écriture**, et écrivez-y votre code (par défaut **`aurelien`**),
   en **texte / UTF-8** (pas en hexadécimal).
5. La carte affiche « Identifie ! » et passe au **vert**. La caractéristique
   de statut renvoie « Bonjour ... ! » (lisible / notifiée).

### Code couleur de la LED

| Couleur | Signification                          |
|---------|----------------------------------------|
| éteinte | En attente d'une connexion             |
| bleu    | iPhone connecté, code pas encore reçu  |
| vert    | Identifié (bon code)                   |
| rouge   | Code refusé                            |

### Personnaliser le code

Modifiez le `#define MON_CODE "aurelien"` en haut de `BluetoothScan.ino`
(la comparaison respecte la casse). Vous pouvez aussi changer le nom annoncé
via `#define DEVICE_NAME`.

> **Astuce iOS** : iOS met en cache les services BLE. Si vous modifiez le
> sketch puis re-téléversez, oubliez le périphérique / relancez le Bluetooth
> sur l'iPhone pour éviter d'afficher d'anciens services.

> Ce mode utilise une simple **connexion + écriture d'un code**, pas le
> jumelage/bonding système d'iOS (avec code à 6 chiffres). C'est volontaire :
> c'est plus simple et ça suffit pour « s'identifier ». Le bonding chiffré est
> possible mais plus lourd — demandez si vous le voulez.

## Publication MQTT (via WiFi)

La carte peut se connecter à un WiFi puis publier le résultat d'un scan BLE
sur un **broker MQTT** (port **1883**, sans TLS).

### Étapes

```
BLE> wifi MonReseau motdepasse
  WiFi connecte. IP : 192.168.1.42 | RSSI : -58 dBm

BLE> mqtt 192.168.1.10 1883
  MQTT connecte.

BLE> topic capteurs/nano-esp32      (optionnel ; défaut : NanoESP32-ID)

BLE> pub
Scan en cours (5 s)...
Publication sur le topic "capteurs/nano-esp32" (312 octets)...
  Publie avec succes.
```

- `wifi <ssid> <mdp>` : le mot de passe peut contenir des espaces ; en revanche
  le SSID ne doit pas en contenir (limite du parseur de commandes).
- `mqtt <host> [port]` : `host` = IP ou nom d'hôte du broker (ex. `test.mosquitto.org`).
- Le **topic** par défaut est le nom de la carte (`NanoESP32-ID`), modifiable
  avec `topic`.

### Format du message publié (JSON)

```json
{
  "board": "NanoESP32-ID",
  "count": 2,
  "devices": [
    { "address": "e7:63:26:3f:5a:e4", "rssi": -55, "distance_cm": 35,
      "name": "JBL Tune 520BT-LE" },
    { "address": "aa:bb:cc:dd:ee:ff", "rssi": -70, "distance_cm": 178,
      "name": "", "manufacturer": "Apple (iBeacon) [hex: ... | txt: ..]" }
  ]
}
```

`distance_cm` vaut `null` si l'estimation n'est pas possible. Le message est
publié en flux (`beginPublish`/`endPublish`), donc sa taille n'est pas limitée
par le buffer interne de PubSubClient.

### Vérifier la réception (côté ordinateur)

Avec les outils Mosquitto :

```bash
mosquitto_sub -h 192.168.1.10 -p 1883 -t "NanoESP32-ID" -v
```

> ⚠️ **Sécurité** : le port 1883 est **en clair, sans authentification**. À
> réserver à un réseau de confiance / des tests. Pour Internet, préférez un
> broker avec TLS (port 8883) et identifiants — je peux l'ajouter au besoin.

> Note : sur l'ESP32-S3, le WiFi et le BLE partagent la même radio 2,4 GHz
> (coexistence). Tout fonctionne, mais un scan pendant une activité WiFi
> intense peut être légèrement moins rapide.

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
- `MON_CODE` : code secret d'identification (défaut : `aurelien`).
- `DEVICE_NAME` : nom BLE annoncé + topic MQTT par défaut (`NanoESP32-ID`).
- `g_mqttPort` : port MQTT (défaut : `1883`, sans TLS) — cf. `mqtt`.
