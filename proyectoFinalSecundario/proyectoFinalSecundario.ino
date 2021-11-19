#include <LiquidCrystal.h>
LiquidCrystal lcd(15, 2, 18, 19, 22, 23);

#include <HardwareSerial.h>
HardwareSerial serial( 2 );
String s1, s2;
void setup() {
  lcd.begin(16, 2);
  serial.begin(115200);
}
void loop() {
  lee();
}

// El caracter de control de fin de linea es '#'
// Si la cadena comienza con '0' es para la fila superior
// Si la cadena comienza con '0' es para la fila inferior
void lee() {
  delay(1);
  if (serial.available() > 0) {
    // Delay para leer bien (Si no se come algunos bytes, ni idea por que)
    bool toS1;
    delay(1);
    char control = serial.read();
    if (control == '0') {
      toS1 = true;
      s1 = "";
    }
    if (control == '1') {
      toS1 = false;
      s2 = "";
    }
    while (serial.available() > 0) {
      delay(1);
      char c = serial.read();
      if (c == '#') break; // EOL
      if (toS1)  s1 += c;
      if (!toS1) s2 += c;
    }
    print2LCD(s1, s2);
  }
}
// Obtiene la cadena con espacios
// Limpia caracteres en caso de que la longitud sea distinta
String fillSpace(String str) {
  while (str.length() < 16) {
    str += " ";
  }
  return str;
}

// Imprime dos cadenas en el LCD agregándoles los espacios requeridos
void print2LCD(String s1, String s2) {
  s1 = fillSpace(s1);
  s2 = fillSpace(s2);
  lcd.setCursor(0, 0);
  lcd.print(s1);
  lcd.setCursor(0, 1);
  lcd.print(s2);
}
