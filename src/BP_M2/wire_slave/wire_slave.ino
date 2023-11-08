#include <Wire.h>
#define BP_ESP_SLAVE_ID 8

#include <Si4703_Breakout.h> // FM radio

#include <Adafruit_Si4713.h>// FM transmitter

#include <BluetoothA2DPSink.h> // Bluetooth




/* Bluetooth */
// ==> Example A2DP Receiver which uses connection_state an audio_state callback
//Don’t install the 2.0 version. At the time of writing this tutorial, we recommend using the legacy version (1.8.19) with the ESP32. While version 2 works well with Arduino, there are still some bugs and some features that are not supported yet for the ESP32.
//https://randomnerdtutorials.com/getting-started-with-esp32/

BluetoothA2DPSink a2dp_sink;
bool BTon = false;
char BTname[256] = "MyMusic";

// for esp_a2d_connection_state_t see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_a2dp.html#_CPPv426esp_a2d_connection_state_t
void connection_state_changed(esp_a2d_connection_state_t state, void *ptr){ // {"Disconnected", "Connecting", "Connected", "Disconnecting"}
    Serial.println(a2dp_sink.to_str(state));    
}

// for esp_a2d_audio_state_t see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_a2dp.html#_CPPv421esp_a2d_audio_state_t
void audio_state_changed(esp_a2d_audio_state_t state, void *ptr){ // {"Suspended", "Stopped", "Started"}
    Serial.println(a2dp_sink.to_str(state));
}

void audio_volume_changed(int volume) {
  Serial.print("Volume: ");
  Serial.println(volume);
}






/* FM radio */
#define resetPin 18
#define SDIO 21
#define SCLK 22

Si4703_Breakout radio(resetPin, SDIO, SCLK);
int channel = 0;
int volume = 0;
char rdsBuffer[256] = {0};

void RadioDisplayInfo()
{
   Serial.print("Channel:"); Serial.print(channel); 
   Serial.print(" Volume:"); Serial.println(volume); 
}





/* FM transmitter */
#define RESETPIN 5
#define FMSTATION 9240      // 10230 == 102.30 MHz
Adafruit_Si4713 FMtrans = Adafruit_Si4713(RESETPIN);





void setup() {
    Serial.begin(9600);
    while(!Serial);

    // I2C slave setup
    Wire1.setPins(16, 17);
    Wire1.begin(BP_ESP_SLAVE_ID);                // join i2c bus with address #8
    Wire1.onRequest(requestEvent); // register event
    Wire1.onReceive(receiveEvent); // register event


    // Bluetooth setup
    a2dp_sink.set_on_connection_state_changed(connection_state_changed);
    a2dp_sink.set_on_audio_state_changed(audio_state_changed);
    a2dp_sink.set_on_volumechange(audio_volume_changed);
    i2s_pin_config_t my_pin_config = {
      .bck_io_num = 26,
      .ws_io_num = 25,
      .data_out_num = 27,
      .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(my_pin_config);



    // Radio setup
    Serial.println("Radio ON!");
    radio.powerOn();
    Serial.println("Volume 0!");
    radio.setVolume(0);






    /* FM transmitter */
    if (!FMtrans.begin()) {  // begin with address 0x63 (CS high default)
        Serial.println("Couldn't find radio?");
    }
    delay(2000);  // maybe needed for initial power???
  
    // Uncomment to scan power of entire range from 87.5 to 108.0 MHz
    /*
    for (uint16_t f  = 8750; f<10800; f+=10) {
     radio.readTuneMeasure(f);
     Serial.print("Measuring "); Serial.print(f); Serial.print("...");
     radio.readTuneStatus();
     Serial.println(radio.currNoiseLevel);
     }*/
     
  
    Serial.print("\nSet TX power");
    FMtrans.setTXpower(115);  // dBuV, 88-115 max
  
    Serial.print("\nTuning into "); 
    Serial.print(FMSTATION/100); 
    Serial.print('.'); 
    Serial.println(FMSTATION % 100);
    FMtrans.tuneFM(FMSTATION); // 102.3 mhz
  
    // This will tell you the status in case you want to read it from the chip
    FMtrans.readTuneStatus();
    Serial.print("\tCurr freq: "); 
    Serial.println(FMtrans.currFreq);
    Serial.print("\tCurr freqdBuV:"); 
    Serial.println(FMtrans.currdBuV);
    Serial.print("\tCurr ANTcap:"); 
    Serial.println(FMtrans.currAntCap);
  
    // begin the RDS/RDBS transmission
    FMtrans.beginRDS();
    FMtrans.setRDSstation((char *)"HistoR"); // max 8 chars
    FMtrans.setRDSbuffer((char *)"HistoRadio FM Live!");
    Serial.println("RDS on!");  


    

  

    Serial.println("ESP slave ON!");
}



void loop() {
    // FM transmitter
    Serial.print("Tuned into "); 
    Serial.print(FMSTATION/100); 
    Serial.print('.'); 
    Serial.print(FMSTATION % 100);
    Serial.println(" FM");
    
    FMtrans.readASQ(); // audio signal quality status
    Serial.print("\tCurr ASQ: 0x"); 
    Serial.print(FMtrans.currASQ, HEX);
    Serial.print(" (");
    printBits(FMtrans.currASQ);
    Serial.println(") [Overmodulation; High Audio; Low Audio] 0 = OK");
    
    Serial.print("\tCurr InLevel (input audio volume range: from 0 to about -10 (dB)):"); 
    Serial.println(FMtrans.currInLevel);



    delay(3000);
}


void printBits(byte myByte){
    for(byte mask = 0x80; mask; mask >>= 1){
        if(mask & myByte)
            Serial.print('1');
        else
            Serial.print('0');
    }
}




// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
    Wire1.write("bt:"); // Bluetooth
    Wire1.write((byte)BTon); // "OFF", "ON"
    Wire1.write((byte)a2dp_sink.is_connected()); // "Disconnected", "Connected"
    Wire1.write((byte)(a2dp_sink.get_audio_state() == 2)); // "Stopped", "Started"
    Wire1.write((byte)a2dp_sink.get_volume()); // uint8_t (range 0 - 255) (0 - 127)
    Wire1.write(";r:");//10
    Wire1.write((byte)volume);
    Wire1.write((byte)(channel-900));
    Wire1.write((byte)(rdsBuffer[0] != 0));
    Wire1.write(";");//14
}


void RDSlisten(void * param)
{
    Serial.println("RDS listening");
    radio.readRDS(rdsBuffer, 15000);
    Serial.print("RDS heard:");
    Serial.println(rdsBuffer); 
}

void receiveEvent(int bytesLength) { // bytes: 00[device][cmd][data...
    char firstB = Wire1.read();
    char secondB = Wire1.read();
    if (firstB == 0 && secondB == 0) {
        char device = Wire1.read();
        char cmd = Wire1.read();
        if (device == 'B') { // Bluetooth
            if(cmd == 'O') { // On/Off
                char state = Wire1.read();
                if(state == 0) {
                    Serial.println("Bluetooth OFF!");
                    if(BTon) { 
                        a2dp_sink.end(false); // (bool release_memory=false) ends the I2S bluetooth sink with the indicated name - if you release the memory a future start is not possible
                        BTon = false;
                    }
                } else if (state == 1) {
                    if(BTon) {
                        a2dp_sink.end(false); // (bool release_memory=false) ends the I2S bluetooth sink with the indicated name - if you release the memory a future start is not possible 
                    }
                    BTon = true;
                    Serial.println("Bluetooth ON!");
                    Serial.println(BTname);
                    a2dp_sink.start(BTname);
                    int vol = a2dp_sink.get_volume();
                    Serial.print("Volume: ");
                    Serial.println(vol);
                    a2dp_sink.set_volume(127);// uint8_t (range 0 - 255) (0 - 127)
                    a2dp_sink.reconnect();
                }
            } else if (cmd == 'N') { // Name
                Serial.print("Bluetooth name: ");   
                int i = 0;
                while(0 < Wire1.available() && i < 255) // loop through all but the last
                {
                    char c = Wire1.read();
                    Serial.print(c);
                    BTname[i++] = c;
                }
                BTname[i] = '\0';
                Serial.println();
            }
        } else if (device == 'R') { // Radio
            if(radio.getFirmware() == 0 && radio.getRSSI() == 0) { // FM radio reset check
                radio.powerOn();
                radio.setVolume(volume);
                radio.setChannel(channel);
            }
            if(cmd == 'T') { // Tune
                channel = Wire1.read()+900;
                radio.setChannel(channel);
            
            } else if(cmd == 'U') { // Seek Up
                channel = radio.seekUp();
            
            } else if(cmd == 'D') { // Seek Down
                channel = radio.seekDown();
            
            } else if(cmd == 'V') { // Volume
                int vol = Wire1.read();
                if (vol >= 0 && vol <= 15) { // Volume: 0 - 15
                    volume = vol;
                    radio.setVolume(volume);
                }
            } else if(cmd == 'R') { // RDS info
                xTaskCreatePinnedToCore(
                    RDSlisten,             /* Function to implement the task */
                    "RDSlisten",           /* Name of the task */
                    3000,                  /* Stack size in bytes */
                    NULL,                  /* Task input parameter */
                    2 | portPRIVILEGE_BIT, /* Priority of the task */
                    NULL,                  /* Task handle. */
                    0                      /* Core where the task should run */
                );
            }
            RadioDisplayInfo();
        }
    }

    
    while(1 < Wire1.available()) // loop through all but the last
    {
        char c = Wire1.read(); // receive byte as a character
        Serial.print(c);         // print the character
    }
    if(Wire1.available()){
        int x = Wire1.read();    // receive byte as an integer
        Serial.println(x);         // print the integer
    }
}
