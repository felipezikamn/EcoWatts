
// ==================================================
// BIBLIOTECAS
// ==================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <math.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>


// ==================================================
// LCD
// ==================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);


// ==================================================
// MEMÓRIA FLASH
// ==================================================

Preferences preferences;


// ==================================================
// CONFIGURAÇÕES
// ==================================================

const float redeVoltage = 127.0;
const float tarifa_kWh = 0.835;

const int NUM_SENSORES = 4;


// ==================================================
// API
// ==================================================

String servidor =
"https://ecowatts-api.onrender.com/api/leituras";
unsigned long ultimoJSON = 0;
const unsigned long intervaloJSON = 30000;


// ==================================================
// GPIO SENSORES
// ==================================================

int pinos[NUM_SENSORES] = {
  34,
  35,
  32,
  33
};


// ==================================================
// SENSIBILIDADES ACS712
// ==================================================

float sensibilidade[NUM_SENSORES] = {

  0.066, // sensor 1 = 30A
  0.100, // sensor 2 = 20A
  0.100, // sensor 3 = 20A
  0.100  // sensor 4 = 20A
};


// ==================================================
// BOTÕES
// ==================================================

const int pinBotaoModo = 14;
const int pinBotaoWiFi = 26;


// ==================================================
// MODOS LCD
// ==================================================

/*
1 = LCD OFF
2 = TOTAL
3 = SENSOR 1
4 = SENSOR 2
5 = SENSOR 3
6 = SENSOR 4
7 = WIFI
*/

int modo = 2;


// ==================================================
// CONTROLE BOTÃO MODO
// ==================================================

bool estadoBotaoModo = false;
bool estadoBotaoModoAnt = false;

unsigned long debounceModo = 0;

const int tempoDebounce = 150;

unsigned long tempoPressionado = 0;


// ==================================================
// RESET WIFI
// ==================================================

unsigned long tempoWiFi = 0;


// ==================================================
// TEMPO
// ==================================================

unsigned long ultimoUpdate = 0;
unsigned long ultimoSave = 0;

const unsigned long intervalo = 1000;
const unsigned long saveIntervalo = 60000;


// ==================================================
// ARRAYS
// ==================================================

float corrente[NUM_SENSORES];
float potencia[NUM_SENSORES];

float energia[NUM_SENSORES];
float custo[NUM_SENSORES];


// ==================================================
// TOTAIS
// ==================================================

float potenciaTotal = 0;
float offsetSensor[NUM_SENSORES] = {0};
float energiaTotal = 0;
float custoTotal = 0;

// ==================================================
// FUNÇÃO RMS
// ==================================================

float medirCorrente(int pino, float sensitivity) {

  const int samples = 1000;

  float somaQuadrados = 0;
  float somaTensao = 0;

  // OFFSET AUTOMÁTICO
  for(int i = 0; i < samples; i++) {

    int leitura = analogRead(pino);

    float tensao =
      leitura * (3.3 / 4095.0);

    somaTensao += tensao;

    delayMicroseconds(100);
  }

  float offset =
    somaTensao / samples;

  // RMS
  for(int i = 0; i < samples; i++) {

    int leitura = analogRead(pino);

    float tensao =
      leitura * (3.3 / 4095.0);

    float valorCentralizado =
      tensao - offset;

    somaQuadrados +=
      valorCentralizado *
      valorCentralizado;

    delayMicroseconds(100);
  }

  float voltageRMS =
    sqrt(somaQuadrados / samples);

  // CONVERSÃO ACS712
  float corrente =
    (voltageRMS / sensitivity) * 3;

  // FILTRO RUÍDO
  if(corrente < 0.25) {
    corrente = 0;
  }

  return corrente;
}
// ==================================================
// LCD SEM FLICKER
// ==================================================

void mostrarLCD(String linha1,
                String linha2) {

  while(linha1.length() < 16) {
    linha1 += " ";
  }

  while(linha2.length() < 16) {
    linha2 += " ";
  }

  lcd.setCursor(0,0);
  lcd.print(linha1);

  lcd.setCursor(0,1);
  lcd.print(linha2);
}


// ==================================================
// BOTÃO MODO
// Clique curto = troca modo
// Clique longo = zera valores locais
// ==================================================
void verificarBotaoModo() {

  static bool ultimoEstado = HIGH;

  static unsigned long inicioPressionado = 0;

  static unsigned long debounce = 0;

  bool estadoAtual =
    digitalRead(pinBotaoModo);

  // debounce
  if (millis() - debounce < 50) {
    return;
  }

  // apertou
  if (estadoAtual == LOW &&
      ultimoEstado == HIGH) {

    debounce = millis();

    inicioPressionado = millis();
  }

  // segurando
  if (estadoAtual == LOW) {

    if (millis() - inicioPressionado >= 3000) {

      for(int i=0;i<NUM_SENSORES;i++) {

        energia[i] = 0;
        custo[i] = 0;
      }

      energiaTotal = 0;
      custoTotal = 0;

      salvarMemoria();

      mostrarLCD(
        "Valores",
        "Resetados"
      );

      delay(1500);

      inicioPressionado =
        millis() + 999999;
    }
  }

  // soltou
  if (estadoAtual == HIGH &&
      ultimoEstado == LOW) {

    debounce = millis();

    unsigned long tempo =
      millis() - inicioPressionado;

    // clique curto
    if (tempo < 10000) {

      modo++;

      if (modo > 6) {
        modo = 1;
      }
    }
  }

  ultimoEstado = estadoAtual;
}
// ==================================================
// RESET WIFI
// Segura 5 segundos
// ==================================================

void verificarResetWiFi() {

  bool pressionado =
    !digitalRead(pinBotaoWiFi);

  if(pressionado) {

    if(tempoWiFi == 0) {

      tempoWiFi = millis();
    }

    if(millis() - tempoWiFi
        >= 5000) {

      mostrarLCD(
        "Resetando",
        "WiFi..."
      );

      WiFiManager wm;

      wm.resetSettings();

      delay(2000);

      ESP.restart();
    }
  }
  else {

    tempoWiFi = 0;
  }
}


// ==================================================
// MEDIR SENSORES
// ==================================================

void medirSensores() {

  // LÊ TODOS OS SENSORES
  for(int i = 0; i < NUM_SENSORES; i++) {

    corrente[i] = medirCorrente(
      pinos[i],
      sensibilidade[i]
    );

    potencia[i] =
      corrente[i] * redeVoltage;
  }

  // TOTAL = SENSOR 1
  potenciaTotal = potencia[0];
}

// ==================================================
// ENERGIA
// ==================================================

void atualizarEnergia(float tempoHoras) {

  // SENSOR 1 = TOTAL
  energia[0] +=
    (potencia[0] / 1000.0)
    * tempoHoras;

  custo[0] =
    energia[0]
    * tarifa_kWh;

  energiaTotal =
    energia[0];

  custoTotal =
    custo[0];

  // SENSORES INDIVIDUAIS
  for(int i = 1;
      i < NUM_SENSORES;
      i++) {

    energia[i] +=
      (potencia[i] / 1000.0)
      * tempoHoras;

    custo[i] =
      energia[i]
      * tarifa_kWh;
  }
}


// ==================================================
// JSON REAL
// ==================================================

void enviarJSON() {

  if(WiFi.status()
      != WL_CONNECTED) {

    Serial.println(
      "WiFi desconectado"
    );

    return;
  }

  HTTPClient http;

  http.begin(servidor);

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  String json = "{";

  json += "\"potencia_total\":";
  json += String(potenciaTotal,2);

  json += ",\"energia_total_kwh\":";
  json += String(energiaTotal,6);

  json += ",\"custo_total\":";
  json += String(custoTotal,4);

  // SENSOR 1
  json += ",\"sensor1\":{";

  json += "\"potencia\":";
  json += String(potencia[0],2);

  json += ",\"energia\":";
  json += String(energia[0],6);

  json += ",\"custo\":";
  json += String(custo[0],4);

  json += "}";

  // SENSOR 2
  json += ",\"sensor2\":{";

  json += "\"potencia\":";
  json += String(potencia[1],2);

  json += ",\"energia\":";
  json += String(energia[1],6);

  json += ",\"custo\":";
  json += String(custo[1],4);

  json += "}";

  // SENSOR 3
  json += ",\"sensor3\":{";

  json += "\"potencia\":";
  json += String(potencia[2],2);

  json += ",\"energia\":";
  json += String(energia[2],6);

  json += ",\"custo\":";
  json += String(custo[2],4);

  json += "}";

  // SENSOR 4
  json += ",\"sensor4\":{";

  json += "\"potencia\":";
  json += String(potencia[3],2);

  json += ",\"energia\":";
  json += String(energia[3],6);

  json += ",\"custo\":";
  json += String(custo[3],4);

  json += "}";

  // WIFI
  json += ",\"wifi\":{";

  json += "\"ssid\":\"";
  json += WiFi.SSID();
  json += "\"";

  json += ",\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\"";

  json += ",\"rssi\":";
  json += String(WiFi.RSSI());

  json += "}";

  json += "}";

  int resposta =
    http.POST(json);

  Serial.print(
    "HTTP Response: "
  );

  Serial.println(resposta);

  Serial.println(json);

  http.end();
}


// ==================================================
// LCD
// ==================================================

void atualizarLCD() {

  switch(modo) {

    // LCD OFF
    case 1:

      lcd.noDisplay();
      lcd.noBacklight();

    break;


    // TOTAL
    case 2:

      lcd.display();
      lcd.backlight();

      mostrarLCD(
        "TOTAL:"
        + String(potenciaTotal,0)
        + "W",

        "R$:"
        + String(custoTotal,4)
      );

    break;



    // SENSOR 2
    case 3:

      lcd.display();
      lcd.backlight();

      mostrarLCD(
        "S2:"
        + String(potencia[1],0)
        + "W",

        "R$:"
        + String(custo[1],4)
      );

    break;


    // SENSOR 3
    case 4:

      lcd.display();
      lcd.backlight();

      mostrarLCD(
        "S3:"
        + String(potencia[2],0)
        + "W",

        "R$:"
        + String(custo[2],4)
      );

    break;


    // SENSOR 4
    case 5:

      lcd.display();
      lcd.backlight();

      mostrarLCD(
        "S4:"
        + String(potencia[3],0)
        + "W",

        "R$:"
        + String(custo[3],4)
      );

    break;


    // WIFI
    case 6:

      lcd.display();
      lcd.backlight();

      mostrarLCD(
        WiFi.SSID(),

        "RSSI:"
        + String(WiFi.RSSI())
      );

    break;
  }
}


// ==================================================
// SALVAR MEMÓRIA
// ==================================================

void salvarMemoria() {

  preferences.begin(
    "energia",
    false
  );

  for(int i = 0;
      i < NUM_SENSORES;
      i++) {

    String chave =
      "e" + String(i);

    preferences.putFloat(
      chave.c_str(),
      energia[i]
    );
  }

  preferences.end();

  Serial.println(
    "Memoria salva"
  );
}


// ==================================================
// CARREGAR MEMÓRIA
// ==================================================

void carregarMemoria() {

  preferences.begin(
    "energia",
    true
  );

  for(int i = 0;
      i < NUM_SENSORES;
      i++) {

    String chave =
      "e" + String(i);

    energia[i] =
      preferences.getFloat(
        chave.c_str(),
        0
      );

    custo[i] =
      energia[i]
      * tarifa_kWh;
  }

  preferences.end();

  energiaTotal = energia[0];
  custoTotal = custo[0];

  Serial.println(
    "Memoria carregada"
  );
}


// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(
    pinBotaoModo,
    INPUT_PULLUP
  );

  pinMode(
    pinBotaoWiFi,
    INPUT_PULLUP
  );

  // LCD
  lcd.init();
  lcd.backlight();

  mostrarLCD(
    "Inicializando",
    "EcoWatts"
  );

  // MEMÓRIA
  carregarMemoria();

  // WIFI
  WiFiManager wm;

  mostrarLCD(
    "Conectando",
    "WiFi..."
  );

  bool conectado =
    wm.autoConnect("EcoWatts");

  if(conectado) {

    mostrarLCD(
      "WiFi OK",
      WiFi.localIP()
      .toString()
    );
  }
  else {

    mostrarLCD(
      "Falha",
      "WiFi"
    );
  }

  delay(2000);
}


// ==================================================
// LOOP
// ==================================================

void loop() {

  verificarBotaoModo();

  verificarResetWiFi();

  unsigned long agora = millis();

  // LOOP PRINCIPAL
  if(agora - ultimoUpdate >= intervalo) {

    float tempoHoras =
      (agora - ultimoUpdate)
      / 3600000.0;

    ultimoUpdate = agora;

    medirSensores();

    atualizarEnergia(tempoHoras);

    atualizarLCD();
  }

  // ENVIO JSON
  if(agora - ultimoJSON >= intervaloJSON) {

    ultimoJSON = agora;

    enviarJSON();
  }

  // SALVA FLASH
  if(agora - ultimoSave >= saveIntervalo) {

    ultimoSave = agora;

    salvarMemoria();
  }
}