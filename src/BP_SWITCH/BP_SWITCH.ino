// Multiplexer 16 kanálů CD74HC4067

// nastavení ovládacích pinů S0-S3
int pinS0 = 4;
int pinS1 = 5;
int pinS2 = 6;
int pinS3 = 7;


void setup(){
  // nastavení ovládacích pinů jako výstupních
  pinMode(pinS0, OUTPUT); 
  pinMode(pinS1, OUTPUT); 
  pinMode(pinS2, OUTPUT); 
  pinMode(pinS3, OUTPUT); 
  // komunikace po sériové lince rychlostí 9600 baud
  Serial.begin(9600);
}

int chan = 0;
void loop(){
    chan++;
    if(chan > 3) {chan = 0;}
    changeChannel(chan);

    Serial.print("Channel: ");
    Serial.println(chan);

  
  // pauza pro přehlednější výpis
  delay(3000);
}

int changeChannel(int channel){
  // pomocné pole s ovládacími piny pro jejich nastavení
  int ovladaciPiny[] = {pinS0, pinS1, pinS2, pinS3};
  // tabulka všech možných kombinací ovládacích pinů
  int kanaly[16][4]={
    {0,0,0,0},  // kanál  0
    {1,0,0,0},  // kanál  1
    {0,1,0,0},  // kanál  2
    {1,1,0,0},  // kanál  3
    {0,0,1,0},  // kanál  4
    {1,0,1,0},  // kanál  5
    {0,1,1,0},  // kanál  6
    {1,1,1,0},  // kanál  7
    {0,0,0,1},  // kanál  8
    {1,0,0,1},  // kanál  9
    {0,1,0,1},  // kanál 10
    {1,1,0,1},  // kanál 11
    {0,0,1,1},  // kanál 12
    {1,0,1,1},  // kanál 13
    {0,1,1,1},  // kanál 14
    {1,1,1,1}   // kanál 15
  };
  // nastavení kombinace ovládacích pinů
  // pomocí smyčky for
  for(int i = 0; i < 4; i ++){
    digitalWrite(ovladaciPiny[i], kanaly[channel][i]);
  }
}
