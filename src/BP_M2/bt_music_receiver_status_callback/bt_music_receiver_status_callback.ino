/*
  Streaming Music from Bluetooth
  
  Copyright (C) 2020 Phil Schatzmann
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// ==> Example A2DP Receiver which uses connection_state an audio_state callback
//Don’t install the 2.0 version. At the time of writing this tutorial, we recommend using the legacy version (1.8.19) with the ESP32. While version 2 works well with Arduino, there are still some bugs and some features that are not supported yet for the ESP32.
//https://randomnerdtutorials.com/getting-started-with-esp32/

#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

// for esp_a2d_connection_state_t see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_a2dp.html#_CPPv426esp_a2d_connection_state_t
void connection_state_changed(esp_a2d_connection_state_t state, void *ptr){
  Serial.println(a2dp_sink.to_str(state));
}

// for esp_a2d_audio_state_t see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_a2dp.html#_CPPv421esp_a2d_audio_state_t
void audio_state_changed(esp_a2d_audio_state_t state, void *ptr){
  Serial.println(a2dp_sink.to_str(state));
}

void audio_volume_changed(int volume) {
  Serial.print("Volume: ");
  Serial.println(volume);
}

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
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
  a2dp_sink.start("MyMusic");
  Serial.println("Bluetooth started!");

  
  int vol = a2dp_sink.get_volume();
  Serial.print("Volume: ");
  Serial.println(vol);
  //a2dp_sink.set_volume(127);// uint8_t (range 0 - 255) (0 - 127)
  /* 
    play();
    pause();
    stop();
    next();
    previous();
    fast_forward();
    rewind();

    is_connected ()

    end(bool release_memory=false); // ends the I2S bluetooth sink with the indicated name - if you release the memory a future start is not possible 
 */
}


void loop() {
  delay(1000); // do nothing
}
