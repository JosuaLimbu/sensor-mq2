#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>

// const char* ssid = "Legacy";
// const char* password = "hitung sendiri";
const char *ssid = "Legacy 3.0";
const char *password = "1sampai8";
const int gasPin = A0;
const int buzzerPin = D5;
const int servoPin = D1;
const int ledPin = D7;

const float R0 = 10000;
const float RL = 20000;
const float Ro = 10;
const float scale_factor = 400.0 / 259151347318784;
// const char* url = "http://api.callmebot.com/whatsapp.php?phone=<phone number>&text=Terjadi%20Kebocoran%20Gas!&apikey=9857754";
// const char* url = "http://api.callmebot.com/whatsapp.php?phone=<phone number>&text=Terjadi%20Kebocoran%20Gas!&apikey=8049248";

WiFiClient client;
ESP8266WebServer server(80);
Servo myservo;

unsigned long previousMillis = 0;
const long interval = 1000;
int buzzerLoop = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
    Serial.begin(115200);
    Wire.begin(D4, D3);
    lcd.init();
    lcd.backlight();

    pinMode(gasPin, INPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(ledPin, OUTPUT);

    WiFi.begin(ssid, password);
    Serial.print("Mencoba terhubung ke ");
    Serial.println(ssid);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi terhubung");
    Serial.println("Alamat IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/getPPM", HTTP_GET, handleGetPPM);
    server.on("/servo", HTTP_GET, handleServo);
    server.begin();

    myservo.attach(servoPin);

    myservo.write(0);
}

void loop()
{
    server.handleClient();

    float ppm = getGasConcentration();

    Serial.print("Konsentrasi Gas : ");
    Serial.print(ppm);
    Serial.println(" ppm");

    if (ppm >= 5000)
    {
        digitalWrite(ledPin, LOW);
        buzzerLoop++;
        // Serial.print("Cek yang ke : ");
        // Serial.println(buzzerLoop);

        for (int i = 0; i < 3; ++i)
        {
            tone(buzzerPin, 2200);
            delay(200);
            noTone(buzzerPin);
            delay(200);
        }

        if (buzzerLoop == 1)
        {
            kirim_wa(ppm);
        }
    }
    else
    {
        digitalWrite(ledPin, HIGH);
        buzzerLoop = 0;
    }

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(ppm);
    lcd.print(" ppm");

    lcd.setCursor(0, 0);
    if (ppm >= 5000)
    {
        lcd.print("Gas Bocor!!!");
    }
    else
    {
        lcd.print("Gas Aman");
    }

    delay(interval);
}

float getGasConcentration()
{
    int gasValue = analogRead(gasPin);
    float Vout = (gasValue * 3.3) / 1023.0;
    float resistance = (RL * (3.3 - Vout)) / Vout;
    float ppm = pow(10, ((log10(resistance / R0)) - Ro) / -0.64);
    ppm *= scale_factor;
    return ppm;
}

void handleRoot()
{
    String content = "<!DOCTYPE html><html><head><title>Konsentrasi Gas</title><script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js\"></script></head><body><h1>Konsentrasi Gas</h1><div id=\"gas-concentration\"></div><button onclick=\"openRegulator()\">Buka Regulator</button><button onclick=\"closeRegulator()\">Kembali</button><script>function openRegulator() {fetch('http://192.168.137.165/servo?value=180');}function closeRegulator() {fetch('http://192.168.137.165/servo?value=0');}function updateGasConcentration() {fetch('/getPPM').then(response => response.json()).then(data => {document.getElementById('gas-concentration').innerHTML = 'Konsentrasi Gas: ' + data.ppm + ' ppm'; if(data.ppm > 5000) {alert('Terdeteksi Gas Bocor!');} }); }setInterval(updateGasConcentration, 1000);</script></body></html>";
    server.send(200, "text/html", content);
}

void kirim_wa(float ppm)
{
    postData(ppm);
}

void postData(float ppm)
{
    int httpCode;
    HTTPClient http;

    String message = "Terjadi%20Kebocoran%20Gas!!!%20Dengan%20Konsentrasi%20Gas%20:%20";
    message += String(ppm);
    message += "%20ppm";

    String url = "http://api.callmebot.com/whatsapp.php?phone=<phone number>&text=";
    url += message;
    url += "&apikey=9857754";

    http.begin(client, url);
    httpCode = http.POST("");

    if (httpCode == 200)
    {
        Serial.println("Berhasil");
    }
    else
    {
        Serial.println("Gagal");
    }
    http.end();
}

void handleGetPPM()
{
    float ppm = getGasConcentration();

    if (isnan(ppm))
    {
        server.send(500, "text/plain", "Error: Gagal membaca ppm dari sensor gas");
        return;
    }

    String json = "{\"ppm\": ";
    json += ppm;
    json += "}";

    server.send(200, "application/json", json);
}

void handleServo()
{
    if (server.hasArg("value"))
    {
        int angle = server.arg("value").toInt();
        if (angle >= 0 && angle <= 180)
        {
            myservo.write(angle);
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(400, "text/plain", "Bad Request");
}