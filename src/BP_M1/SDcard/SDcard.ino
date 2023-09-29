#include <SPI.h>
#include <SD.h>

//https://www.hwkitchen.cz/navody-hwkitchen/navod-na-pouziti-modulu-sd-karty-arduino-navody/
//https://randomnerdtutorials.com/esp32-spi-communication-arduino/

File myFile;
int const pinSS = 18;
int const pinCS = 5;

void setup()
{
  // i když se pin nepoužívá, musí být definován jako výstup!
  pinMode(pinSS, OUTPUT);
  // začátek sériové komunikace
  Serial.begin(115200);
  while(!Serial);
  Serial.print("inicializace SD karty...");

  // pokud není karta dostupná, tak:
  if (!SD.begin(pinCS))
  {
    Serial.println(" inicializace selhala!");
    while (1);
  }
  Serial.println(" inicializace uspesna!");

  // soubor, který chceme otevřít a psát do něj (FILE_WRITE)
  // se jmenuje test.txt (popřípadě vložte celou cestu k souboru)
  myFile = SD.open("/test.txt", FILE_WRITE);

  // pokud se soubor načte a otevře, tak:
  if (myFile)
  {
    Serial.println("Zapisuji do test.txt");
    // zapiš do souboru
    myFile.println("dneska sviti slunce");
    // zavři soubor
    myFile.close();
    Serial.println("uspesne zapsano");
  } 
  // pokud se nepodaří soubor načíst a otevřít, tak:
  else
  {
    Serial.println("soubor se nepodarilo otevrit");
  }

  // přečtění souboru:
  Serial.println("\ncteni ze souboru\n");
  // otevři soubor test.txt pro čtení, mód je defaultně nastavený na FILE_READ
  myFile = SD.open("/test.txt");
  // pokud se soubor načte a otevře, tak:
  if (myFile)
  {
    Serial.println("soubor obsahuje: ");

    // čti ze souboru, dokud je co
    while (myFile.available())
    {
      Serial.write(myFile.read());
    }
    // zavři soubor
    myFile.close();
  }
  // pokud se nepodaří soubor načíst a otevřít, tak:
  else
  {
    Serial.println("soubor se nepodarilo otevrit");
  }
}

void loop()
{
  // jelikož chceme, aby kód proběhl jenom jednou, tak zde nic nebude
}
