// Multiplexer 16 kanálů CD74HC4067
//https://electronics.stackexchange.com/questions/85013/multiplexers-used-as-a-switch-seem-to-leak-when-power-is-cut
//https://electronics.stackexchange.com/questions/66332/mosfets-or-bjts-for-using-as-switch-for-audio-signals
//https://www.youtube.com/watch?v=-5JOKztteVU
// CD74HC4051 - ANALOG MULTIPLEXER/DEMULTIPLEXER
// SN74HC4066/CD4066B - QUADRUPLE BILATERAL ANALOG SWITCH

// nastavení ovládacích pinů S0-S3
int pinI0 = 5;
int pinI1 = 6;
int pinI2 = 7;

int pinO0 = 3;
int pinO1 = 4;

bool state = HIGH;

void setup(){
  // nastavení ovládacích pinů jako výstupních
  pinMode(pinI0, OUTPUT);
  pinMode(pinI1, OUTPUT);
  pinMode(pinI2, OUTPUT);
  
  pinMode(pinO0, OUTPUT);
  pinMode(pinO1, OUTPUT);

  // komunikace po sériové lince rychlostí 9600 baud
  Serial.begin(9600);
  Serial.setTimeout(10);
}

void loop(){
    if (Serial.available() > 0) {
        String str = Serial.readString();
        str.trim();
        Serial.println("<"+str+">");

        if (str.charAt(0) == 'O') {
            int pinID = 0;
            if (str.charAt(1) == '0') {
                pinID = 3;
            } else if (str.charAt(1) == '1') {
                pinID = 4;
            }
            if (str.charAt(2) == '0') {
                digitalWrite(pinID, LOW);
                Serial.println("Output pin "+String((int)(str.charAt(1) - '0'))+" changed to: LOW");
            } else if (str.charAt(2) == '1') {
                digitalWrite(pinID, HIGH);
                Serial.println("Output pin "+String((int)(str.charAt(1) - '0'))+" changed to: HIGH");
            }
        } else if (str.charAt(0) == 'I') {
            changeChannel((int)(str.charAt(1) - '0'));
            Serial.println("Input channel is now: "+String((int)(str.charAt(1) - '0')));
        }
    }

    delay(30);
}

int changeChannel(int channel){
  // pomocné pole s ovládacími piny pro jejich nastavení
  int ovladaciPiny[] = {pinI0, pinI1, pinI2};
  // tabulka všech možných kombinací ovládacích pinů
  int kanaly[8][3]={
    {0,0,0},  // kanál  0
    {0,0,1},  // kanál  1
    {0,1,0},  // kanál  2
    {0,1,1},  // kanál  3
    {1,0,0},  // kanál  4
    {1,0,1},  // kanál  5
    {1,1,0},  // kanál  6
    {1,1,1},  // kanál  7
  };
  // nastavení kombinace ovládacích pinů
  // pomocí smyčky for
  for(int i = 0; i < 3; i ++){
    digitalWrite(ovladaciPiny[i], kanaly[channel][i]);
  }
}
