
#ifdef ESP32
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Firebase_ESP_Client.h>
#else
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#endif
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

#define API_KEY "AIzaSyDBIXZPgYiw1Q1kpZYFsOkjzyOPPMtSB20"


#define DATABASE_URL "https://esp32-firebase-d51c6-default-rtdb.europe-west1.firebasedatabase.app/" 


long duration;
float distanceCm;
float distanceInch;
const int trigPin = 5;
const int echoPin = 18;
const int PIR_SENSOR_OUTPUT_PIN = 13; 
const char* ssid = "WIN-27N93FMHVKP 7722";
const char* password = "rockrocks";
bool signupOK = false;

AsyncWebServer server(80);
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Distanta pana la cel mai apropia obiect :</h2>
  <p>
    <i class="fas fa-people-arrows" style="color:#059e8a;"></i> 
    <span class="ds-labels">Distanta Centimetrii</span> 
    <span id="distanceCm">%distanceCm%</span>
    <sup class="units">cm</sup>
  </p>
  <p>
    <i class="fas fa-people-arrows" style="color:#059e8a;"></i> 
    <span class="ds-labels">Distanta Inch</span>
    <span id="distanceInch">%distanceInch%</span>
    <sup class="units">inch</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distanceCm").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/distanceCm", true);
  xhttp.send();
}, 100) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distanceInch").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/distanceInch", true);
  xhttp.send();
}, 100) ;
</script>
</html>)rawliteral";


String processor(const String& var) {
  //Serial.println(var);
  if (var == "distanceCm") {
    return String(distanceCm);
  } else if (var == "distanceInch") {
    return String(distanceInch);
  }
  return String();
}


void setup() {
  
  pinMode(PIR_SENSOR_OUTPUT_PIN, INPUT);

  delay(20000);

  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println();


  Serial.println(WiFi.localIP());


  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/distanceCm", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", String(distanceCm).c_str());
  });
  server.on("/distanceInch", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", String(distanceInch).c_str());
  });

  server.begin();
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void func() {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin, LOW);


  duration = pulseIn(echoPin, HIGH);


  distanceCm = duration * SOUND_SPEED / 2;


  distanceInch = distanceCm * CM_TO_INCH;

  Serial.print("Distanta (cm): ");
  Serial.println(distanceCm);
  Serial.print("Distanta (inch): ");
  Serial.println(distanceInch);

   if (Firebase.ready() && signupOK ){
    
    if (Firebase.RTDB.setInt(&fbdo, "IoT/distantaCm", distanceCm)){
      Serial.println("SENT TO DB");
    }
    else {
      Serial.println("FAILED & REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "IoT/distantaInch", distanceInch)){
      Serial.println("SENT TO DB");
    }
    else {
      Serial.println("FAILED & REASON: " + fbdo.errorReason());
    }

   }

}





void loop() {

  int sensor_output;
  sensor_output = digitalRead(PIR_SENSOR_OUTPUT_PIN);

  if (sensor_output != LOW) {
    func();
    delay(100);
  }
}
