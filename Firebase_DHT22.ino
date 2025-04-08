#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>  // Inclure la bibliothèque DHT pour le capteur DHT22

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
#include "time.h"

// Informations de connexion Wi-Fi
#define WIFI_SSID "ooredoo_C23D38"
#define WIFI_PASSWORD "R53600-GTX1660SUPER-2024--"

// Clé API de votre projet Firebase
#define API_KEY "your owen token"

// Email et mot de passe autorisés
#define USER_EMAIL "abdelkader21@gmail.com"
#define USER_PASSWORD "1234560987"

// URL de la RTDB
#define DATABASE_URL "your owen link in realtimedatabse"

// Définir les objets Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable pour sauvegarder le UID utilisateur
String uid;

// Chemin principal de la base de données (à mettre à jour dans setup avec le UID de l'utilisateur)
String databasePath;

// Nœuds enfants de la base de données
String humidityPath = "/humidity";
String temperaturePath = "/temperature";
String timePath = "/timestamp";

// Définir la broche pour le capteur DHT22
#define brocheDeBranchementDHT 21   // GPIO 21 sur l'ESP32
#define typeDeDHT DHT22             // Type de capteur DHT utilisé

// Instanciation de la bibliothèque DHT
DHT dht(brocheDeBranchementDHT, typeDeDHT);

// Variables de minuterie (envoyer de nouvelles lectures toutes les trois minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;

// Serveur NTP pour obtenir l'heure
const char* ntpServer = "pool.ntp.org";

FirebaseJson json;

// Initialiser le capteur DHT22
void initDHTSensor() {
  dht.begin();
}

// Initialiser le Wi-Fi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Fonction pour obtenir l'heure actuelle en epoch
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0;
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);

  // Initialiser le capteur DHT22
  initDHTSensor();
  initWiFi();
  configTime(0, 0, ntpServer);

  // Configurer Firebase
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback; // voir addons/TokenHelper.h
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);

  // Obtenir le UID de l'utilisateur peut prendre quelques secondes
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Mettre à jour le chemin de la base de données
  databasePath = "/UsersData/" + uid + "/readings";
}

void loop() {
  // Envoyer de nouvelles lectures à la base de données
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Lire les données du capteur DHT22
    float tauxHumidite = dht.readHumidity();
    float temperatureEnCelsius = dht.readTemperature();

    // Vérifier si les données ont été reçues correctement
    if (isnan(tauxHumidite) || isnan(temperatureEnCelsius)) {
      Serial.println("Aucune valeur retournée par le DHT22. Est-il bien branché ?");
      return;
    }

    // Obtenir l'heure actuelle
    unsigned long timestamp = getTime();
    Serial.print("Time: ");
    Serial.println(timestamp);

    // Afficher les valeurs sur le Moniteur Série
    Serial.print("Humidité = "); Serial.print(tauxHumidite); Serial.println(" %");
    Serial.print("Température = "); Serial.print(temperatureEnCelsius); Serial.println(" °C");

    // Définir le chemin parent
    String parentPath = databasePath + "/" + String(timestamp);

    // Définir les valeurs à envoyer
    json.set(humidityPath.c_str(), tauxHumidite);
    json.set(temperaturePath.c_str(), temperatureEnCelsius);
    json.set(timePath, String(timestamp));

    // Envoyer les données à Firebase
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}
