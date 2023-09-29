// LCD displej pres I2C
// navody.dratek.cz

// knihovny pro LCD přes I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// nastavení adresy I2C (0x27 v mém případě),
// a dále počtu znaků a řádků LCD, zde 20x4
LiquidCrystal_I2C lcd(0x27, 16, 2); // SDA -> D21; SCL -> D22; VCC -> VIN (5V)
//https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  Serial.println("LCD display - start");
  
  // inicializace LCD
  lcd.begin();
  // zapnutí podsvícení
  lcd.backlight();

  // veškeré číslování je od nuly, poslední znak je tedy 15, 1
  lcd.print("Test LCD - I2C");
  lcd.setCursor ( 0, 1);
  lcd.print("--------------------");
  lcd.setCursor ( 7, 1);
  lcd.print("!");
  delay(2000);
}

void loop()
{
  // nastavení kurzoru na devátý znak, druhý řádek
  lcd.setCursor(7, 1);
  // vytisknutí počtu sekund od začátku programu
  lcd.print(millis() / 1000);
  
  Serial.println("LCD display");
  delay(1000);
}
