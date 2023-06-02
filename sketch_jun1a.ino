#include <ESP_Mail_Client.h>
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

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "iotproject605@gmail.com"
#define AUTHOR_PASSWORD "zrmsrlghwsoyqpfu"

#define RECIPIENT_EMAIL "bringtogetherofficial@gmail.com"

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);
ESP_Mail_Session session;
long duration;
float distanceCm;
float distanceInch;
float lastDistance;
const int trigPin = 5;
const int echoPin = 18;
const int PIR_SENSOR_OUTPUT_PIN = 13;
const char* ssid = "WIN-27N93FMHVKP 7722";
const char* password = "rockrocks";
bool signupOK = false;
int flagFirstTime = 0;

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
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */


  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
}

void sendEmail() {

 
    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "ESP";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "ESP Motion Detected";
    message.addRecipient("Stela", RECIPIENT_EMAIL);

    /*Send HTML message*/
    String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Motion was detected and somebody is in the room</h1><p>- Sent from ESP board</p></div>";
    message.html.content = htmlMsg.c_str();
    message.html.content = htmlMsg.c_str();
    message.text.charSet = "us-ascii";
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

void smtpCallback(SMTP_Status status) {
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()) {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
  }
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

  if (Firebase.ready() && signupOK) {

    if (Firebase.RTDB.setInt(&fbdo, "IoT/distantaCm", distanceCm)) {
      Serial.println("SENT TO DB");
    } else {
      Serial.println("FAILED & REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "IoT/distantaInch", distanceInch)) {
      Serial.println("SENT TO DB");
    } else {
      Serial.println("FAILED & REASON: " + fbdo.errorReason());
    }
  }

  sendEmail();
}





void loop() {

  int sensor_output;
  sensor_output = digitalRead(PIR_SENSOR_OUTPUT_PIN);

  if (sensor_output != LOW) {
    func();
    delay(100);
  }
}
