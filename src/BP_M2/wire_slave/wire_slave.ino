#include <Wire.h>
#define BP_ESP_SLAVE_ID 8



// ==> Example A2DP Receiver which uses connection_state an audio_state callback
//Don’t install the 2.0 version. At the time of writing this tutorial, we recommend using the legacy version (1.8.19) with the ESP32. While version 2 works well with Arduino, there are still some bugs and some features that are not supported yet for the ESP32.
//https://randomnerdtutorials.com/getting-started-with-esp32/

#include <BluetoothA2DPSink.h>





BluetoothA2DPSink a2dp_sink;
bool BTon = false;

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


char BTname[256] = "MyMusic";


void setup() {
    Serial.begin(9600);
    while(!Serial);

    // I2C slave setup
    Wire.begin(BP_ESP_SLAVE_ID);                // join i2c bus with address #8
    Wire.onRequest(requestEvent); // register event
    Wire.onReceive(receiveEvent); // register event


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


    Serial.println("ESP slave ON!");
}



void loop() {
    delay(100);
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
    Wire.write("bt:"); // Bluetooth
    Wire.write((byte)BTon); // "OFF", "ON"
    Wire.write((byte)a2dp_sink.is_connected()); // "Disconnected", "Connected"
    Wire.write((byte)(a2dp_sink.get_audio_state() == 2)); // "Stopped", "Started"
    Wire.write((byte)a2dp_sink.get_volume()); // uint8_t (range 0 - 255) (0 - 127)
    Wire.write(";");
}




void receiveEvent(int bytesLength) { // bytes: 00[device][cmd][data...
    char firstB = Wire.read();
    char secondB = Wire.read();
    if (firstB == 0 && secondB == 0) {
        char device = Wire.read();
        char cmd = Wire.read();
        if (device == 'B') { // Bluetooth
            if(cmd == 'O') { // On/Off
                char state = Wire.read();
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
                while(0 < Wire.available() && i < 255) // loop through all but the last
                {
                    char c = Wire.read();
                    Serial.print(c);
                    BTname[i++] = c;
                }
                BTname[i] = '\0';
                Serial.println();
            }
        }
        
    }

    
    while(1 < Wire.available()) // loop through all but the last
    {
        char c = Wire.read(); // receive byte as a character
        Serial.print(c);         // print the character
    }
    if(Wire.available()){
        int x = Wire.read();    // receive byte as an integer
        Serial.println(x);         // print the integer
    }
}
