#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "ESP8266HTTPClient.h"
#include "secrets.h"


/*
   The format of our output.

   Set this to PROCESSING_VISUALIZER if you're going to run
    the Processing Visualizer Sketch.
    See https://github.com/WorldFamousElectronics/PulseSensor_Amped_Processing_Visualizer

   Set this to SERIAL_PLOTTER if you're going to run
    the Arduino IDE's Serial Plotter.
*/
const int OUTPUT_TYPE = PROCESSING_VISUALIZER;

WiFiClient wificlient;
/*
   Pinout:
     PULSE_INPUT = Analog Input. Connected to the pulse sensor
      purple (signal) wire.
     PULSE_BLINK = digital Output. Connected to an LED (and 1K resistor)
      that will flash on each detected pulse.
     PULSE_FADE = digital Output. PWM pin onnected to an LED (and 1K resistor)
      that will smoothly fade with each pulse.
      NOTE: PULSE_FADE must be a pin that supports PWM.
       If USE_INTERRUPTS is true, Do not use pin 9 or 10 for PULSE_FADE,
       because those pins' PWM interferes with the sample timer.
*/
const int PULSE_INPUT = 0;
const int PULSE_BLINK = LED_BUILTIN;
const int PULSE_FADE = 5;
const int THRESHOLD = 560;   // Adjust this number to avoid noise when idle

// RTC_DATA_ATTR int bootCount = 0;
// RTC_DATA_ATTR char name[15] = CLIENT;

/*
   samplesUntilReport = the number of samples remaining to read
   until we want to report a sample over the serial connection.

   We want to report a sample value over the serial port
   only once every 20 milliseconds (10 samples) to avoid
   doing Serial output faster than the Arduino can send.
*/
byte samplesUntilReport;
const byte SAMPLES_PER_SERIAL_SAMPLE = 30;

StaticJsonDocument<500> doc;
/*
   All the PulseSensor Playground functions.
*/
PulseSensorPlayground pulseSensor;

void setup() {
  /*
     Use 115200 baud because that's what the Processing Sketch expects to read,
     and because that speed provides about 11 bytes per millisecond.

     If we used a slower baud rate, we'd likely write bytes faster than
     they can be transmitted, which would mess up the timing
     of readSensor() calls, which would make the pulse measurement
     not work properly.
  */
  Serial.begin(115200);

  // Configure the PulseSensor manager.
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);
  pulseSensor.fadeOnPulse(PULSE_FADE);

  pulseSensor.setSerial(Serial);
  pulseSensor.setOutputType(OUTPUT_TYPE);
  pulseSensor.setThreshold(THRESHOLD);

  //comfigure the WiFi credentails
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, LOW);

  // Skip the first SAMPLES_PER_SERIAL_SAMPLE in the loop().
  samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

  // Now that everything is ready, start reading the PulseSensor signal.
  if (!pulseSensor.begin()) {
    /*
       PulseSensor initialization failed,
       likely because our Arduino platform interrupts
       aren't supported yet.

       If your Sketch hangs here, try changing USE_PS_INTERRUPT to false.
    */
    for(;;) {
      // Flash the led to show things didn't work.
      digitalWrite(PULSE_BLINK, LOW);
      delay(50);
      digitalWrite(PULSE_BLINK, HIGH);
      delay(50);
    }
  }
}

void loop() {

  /*
     See if a sample is ready from the PulseSensor.

     If USE_INTERRUPTS is true, the PulseSensor Playground
     will automatically read and process samples from
     the PulseSensor.

     If USE_INTERRUPTS is false, this call to sawNewSample()
     will, if enough time has passed, read and process a
     sample (analog voltage) from the PulseSensor.
  */
  if (pulseSensor.sawNewSample()) {
    /*
       Every so often, send the latest Sample.
       We don't print every sample, because our baud rate
       won't support that much I/O.
    */
    if (--samplesUntilReport == (byte) 0) {
      samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

      pulseSensor.outputSample();

      /*
         At about the beginning of every heartbeat,
         report the heart rate and inter-beat-interval.
      */
      if (pulseSensor.sawStartOfBeat()) {
        pulseSensor.outputBeat();
      }
    }
    
    senseBMP();
    

    // getDevice();
    delay(10000);
    Serial.println("Posting...");
    POSTData();
    serializeJsonPretty(doc, Serial);
    Serial.println("\nDone.");
  }

  /******
     Don't add code here, because it could slow the sampling
     from the PulseSensor.
  ******/
}

// void getDevice(){
//       esp_sleep_wakeup_cause_t wakeup_reason;
//     wakeup_reason = esp_sleep_get_wakeup_cause();

//     uint64_t chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
//     Serial.printf("***ESP32 Chip ID = %04X%08X\n",(uint16_t)(chipid>>32),(uint32_t)chipid);//print High 2 bytes
//     char buffer[200];
//     sprintf(buffer, "%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);
//     //sprintf(buffer, "esp32%" PRIu64, ESP.getEfuseMac());

//     // int vbatt_raw = 0;
//     // for (int i=0;i<SAMPLES;i++)
//     // {
//     //    vbatt_raw += analogRead(PIN_POWER);
//     //    delay(100);
//     // }
//     // vbatt_raw = vbatt_raw/SAMPLES;
//     //float vbatt = map(vbatt_raw, 0, 4096, 0, 4200);

//     doc["device"]["IP"] = WiFi.localIP().toString();
//     doc["device"]["RSSI"] = String(WiFi.RSSI());
//     doc["device"]["type"] = TYPE;
//     doc["device"]["name"] = name;
//     doc["device"]["chipid"] = buffer;
//     doc["device"]["bootCount"] = bootCount;
//     doc["device"]["wakeup_reason"] = String(wakeup_reason);
    //doc["device"]["vbatt_raw"] = vbatt_raw;
    //doc["device"]["vbatt"] = map(vbatt_raw, 0, 4096, 0, 4200);
// }

void POSTData()
{
      
      if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      http.begin(wificlient, serverName);

      http.addHeader("Content-Type", "application/json");

      String json;
      serializeJson(doc, json);

      Serial.println(json);
      int httpResponseCode = http.POST(json);
      Serial.println(httpResponseCode);
      }

}

void senseBMP(){
    int myBPM = pulseSensor.getBeatsPerMinute();
    Serial.print("Here is my beatsperminute : ");
    Serial.println(myBPM);
      doc["keyCode"] = myBPM;
      doc["active"] = true;
      doc["description"] = "helleuuu";}
