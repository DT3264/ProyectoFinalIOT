/// KEYPAD
#include <SimpleKeypad.h>
const byte nb_rows = 4;                         // 4 filas
const byte nb_cols = 3;                         // 3 columnas
char key_chars[nb_rows][nb_cols] = {            // Los caracteres posibles
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'.', '0', '.'}
};
byte rowPins[nb_rows] = {27, 26, 25, 33};       // Los pines de las filas
byte colPins[nb_cols] = {14, 12, 13};           // Los pines de las columnas
SimpleKeypad keypad((char*)key_chars, rowPins, colPins, nb_rows, nb_cols);   // Definición del keypad

/// Conexión serial a la otra placa
#include <HardwareSerial.h>
// RX debe estar conectado a otro TX
// TX debe estar conectado a otro RX
HardwareSerial serial( 2 ); // Usa UART2 con RX2 y TX2

/// Sensor de temperatura
#include "DHTesp.h"
//#define DHTTYPE DHT11
DHTesp dht;
int dhtPin = 4; // Pin del DHT

/// WIFI
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
const String EventoRegistro = "registro";             // Evento para registrar entrada
const String EventoEnviarMensaje = "enviar_mensaje";  // Evento para enviar codigo de ingreso

/// Lector RFID
#include <MFRC522.h>  //Libreria para comunicarse con el RFID-RC522
#include <SPI.h>      //library responsible for communicating of SPI bus
int SS_PIN = 21;      // Pin SS (SDA en la placa)
int RST_PIN = 22;     // Pin RST (SCL en la placa)
// Están conectados en este orden
// SDA  -> D21
// SCK  -> D18
// MOSI -> D23
// MISO -> D19
// IRQ  -> No conectado
// GND  -> GND
// RST  -> D22
// 3.3v -> 3.3v
#define SIZE_BUFFER     18  // Tamaño del buffer (Ni idea por que lo definen aquí))
#define MAX_SIZE_BLOCK  16  // Tamaño de datos   (Ni idea por que lo definen aquí x2)
// Define el lector RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);
struct Usuario {
  String nombre;
  byte   uidByte[4];
};
Usuario usuarios[2] = {
  {"Mike", {0x42, 0xC8, 0xC6, 0x0D}}, // Tarjeta
  {"Dennis", {0xEC, 0x23, 0xDE, 0x89}} // Token azul
};

/// SECCION DE INITS
void setup() {
  initSerial();
  initWiFi();
  initTempReader();
  initReader();
}
void initSerial() {
  serial.begin(115200);
}
void initWiFi() {
  WiFi.begin("CasaGG", "32641224GG");
  toLCD("Conectando a red", "", 1);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    toLCD("Conexion fallida", "", 1000);
    return;
  }
  toLCD("Conectado a red", "", 1000);
}
void initReader() {
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  toLCD("Lector listo", "", 1000);
}
void initTempReader() {
  dht.setup(dhtPin, DHTesp::DHT11);
  toLCD("Sensor de temp", "listo", 1000);
}

/// SECCION DEL LOOP
String usuarioAct = "";
String codigoEsperado = "";
String codigoActual = "";
int etapa = 1;
int mensajeLectorListo = 0;
void loop() {
  if (etapa == 1) {
    usuarioAct = "";
    codigoEsperado = "";
    codigoActual = "";
    toLCD("Acerque su tag", "", 1);
    leeTarjeta();
  }
  else if (etapa == 2) {
    leeTemperatura();
  }
  else if (etapa == 3) {
    leeCaracter();
  }
}

void leeTarjeta() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) return;
  if ( ! mfrc522.PICC_ReadCardSerial()) return;
  for (auto &u : usuarios) {
    bool esValido = true;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] != u.uidByte[i]) {
        esValido = false;
        break;
      }
    }
    if (esValido) usuarioAct = u.nombre;
  }
  if (usuarioAct.length() == 0) {
    toLCD("Usuario", "no encontrado", 1000);
    etapa = 1;
    return;
  }
  toLCD("Usuario:", usuarioAct, 1000);
  codigoEsperado = "";
  for (int i = 0; i < 4; i++) {
    int randValue = random(1000) % 10;
    codigoEsperado += String(randValue);
  }
  // Envia el codigo esperado al correo
  postDataToServer(EventoEnviarMensaje, usuarioAct, codigoEsperado);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  etapa = 2;
}
void leeTemperatura() {
  TempAndHumidity newValues = dht.getTempAndHumidity();
  if (dht.getStatus() != 0) {
    toLCD("DHT11 error", String(dht.getStatusString()), 2000);
  }
  toLCD("Presione el DHT", "por 5 segundos", 1000);
  delay(5000);
  float temp = newValues.temperature;
  toLCD("Temperatura:", String(temp), 2000);
  if (temp > 38.0) {
    toLCD("Temp alta", "Acceso denegado", 2000);
    postDataToServer(EventoRegistro, usuarioAct + ", Alta temperatura", "");
    etapa = 1;
  }
  else {
    toLCD("Temp normal", "Ingrese clave", 2000);
    etapa = 3;
  }
}
void leeCaracter() {
  toLCD("Codigo:", codigoActual, 50);
  char caracter = keypad.getKey();
  delay(10);
  if (caracter) {
    codigoActual += caracter;
    toLCD("Codigo:", codigoActual, 50);
  }
  if (codigoActual.length() == 4) {
    delay(1000);
    if (codigoActual == codigoEsperado) {
      toLCD("Ingreso exitoso", "", 2000);
      // Manda registrar la entrada
      postDataToServer(EventoRegistro, usuarioAct + ", Ingreso exitoso", "");
      etapa = 1;
    }
    else {
      toLCD("Ingreso fallido", "", 2000);
      etapa = 1;
      postDataToServer(EventoRegistro, usuarioAct + ", Ingreso fallido", "");
    }
  }
}

// Ninguna cadena debe llevar '#'
// pues es el caracter que indica el fin de la linea.
// Escribe dos cadenas por serial
// s1 en la fila superior indicado por '0'
// y s2 en la fila inferior indicado por '1'
String prevS1, prevS2;
void toLCD(String s1, String s2, int delayIndicado) {
  // Evita mandar el mismo mensaje repetidamente
  if (s1 == prevS1 && s2 == prevS2) {
    delay(delayIndicado);
    return;
  };
  prevS1 = s1;
  prevS2 = s2;
  // Indicador de cadena 1
  serial.write('0');
  delay(1);
  for (int i = 0; i < s1.length(); i++) {
    serial.write(s1[i]);
    delay(1);
  }
  // Indicador de fin de cadena
  serial.write('#');
  delay(1);
  // Indicador de cadena 1
  serial.write('1');
  delay(1);
  for (int i = 0; i < s2.length(); i++) {
    serial.write(s2[i]);
    delay(1);
  }
  // Indicador de fin de cadena
  serial.write('#');
  // Da un tiempo para que se muestre el mensaje
  delay(delayIndicado);
}


void postDataToServer(String evento, String v1, String v2) {
  //  Serial.println("Enviando evento " + evento);
  HTTPClient http;
  String url = "https://maker.ifttt.com/trigger/" + evento + "/with/key/bCobziHXuqJvilbo-w-VY7";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String requestBody;
  StaticJsonDocument<200> doc;
  doc["value1"] = v1;
  doc["value2"] = v2;
  serializeJson(doc, requestBody);
  int httpResponseCode = http.POST(requestBody);
  //  if(httpResponseCode>0){
  //    String response = http.getString();
  //    Serial.print("Respuesta del servidor: " + String(httpResponseCode) + "\n");
  //    Serial.println(response);
  //  }
  http.end();
}
