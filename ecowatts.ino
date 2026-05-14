/*
====================================================
 TESTE WIFI MANAGER - ESP32
====================================================

✔ Cria portal Wi-Fi automático
✔ Salva credenciais
✔ Reconecta sozinho
✔ LCD mostrando status
✔ Botão para resetar Wi-Fi
✔ Estrutura pronta pro projeto principal

====================================================
*/


// ==================================================
// BIBLIOTECAS
// ==================================================

#include <WiFi.h>
#include <WiFiManager.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// ==================================================
// LCD
// ==================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);


// ==================================================
// BOTÃO RESET WIFI
// ==================================================

const int pinResetWiFi = 26;


// ==================================================
// VARIÁVEIS RESET
// ==================================================

unsigned long tempoReset = 0;

bool resetAtivado = false;


// ==================================================
// RESET WIFI
// ==================================================

void verificarResetWiFi(
  WiFiManager &wm
) {

  bool pressionado =
    !digitalRead(pinResetWiFi);


  // COMEÇOU A SEGURAR
  if(pressionado &&
     !resetAtivado) {

    tempoReset = millis();

    resetAtivado = true;
  }


  // SOLTOU BOTÃO
  if(!pressionado) {

    resetAtivado = false;
  }


  // SEGURANDO 5s
  if(resetAtivado &&
    (millis() - tempoReset >= 5000)) {

    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Resetando");

    lcd.setCursor(0,1);
    lcd.print("WiFi...");


    // APAGA WIFI SALVO
    wm.resetSettings();


    delay(2000);


    ESP.restart();
  }
}


// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(115200);


  // BOTÃO
  pinMode(
    pinResetWiFi,
    INPUT_PULLUP
  );


  // LCD
  lcd.init();

  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Iniciando");

  lcd.setCursor(0,1);
  lcd.print("WiFi Manager");


  // ==================================================
  // WIFI MANAGER
  // ==================================================

  WiFiManager wm;


  // Nome do AP
  bool conectado =
    wm.autoConnect(
      "MonitorEnergia"
    );


  // FALHA
  if(!conectado) {

    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Falha WiFi");

    Serial.println(
      "Falha WiFi"
    );

    delay(3000);

    ESP.restart();
  }


  // SUCESSO
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("WiFi OK");


  lcd.setCursor(0,1);

  lcd.print(
    WiFi.localIP()
  );


  Serial.println(
    "Conectado!"
  );

  Serial.println(
    WiFi.localIP()
  );
}


// ==================================================
// LOOP
// ==================================================

void loop() {

  static WiFiManager wm;

  verificarResetWiFi(wm);


  // MOSTRA STATUS SERIAL
  static unsigned long ultimoPrint = 0;

  if(millis() - ultimoPrint >= 5000) {

    ultimoPrint = millis();

    Serial.print("WiFi: ");

    Serial.println(
      WiFi.SSID()
    );

    Serial.print("IP: ");

    Serial.println(
      WiFi.localIP()
    );
  }
}