#include <Wire.h>

const int buttonPin = A0;          // Pin del pulsador para Morse
const int sendButtonPin = A1;      // Pin del botón para enviar datos I2C
const int ledPin = 2;              // Pin del LED externo
const int builtInLedPin = 13;      // Pin del LED incorporado

int buttonState = 0;               // Variable para leer el estado del pulsador
int lastButtonState = HIGH;        // Estado anterior del pulsador
unsigned long buttonPressTime = 0; // Tiempo en el que se presionó el pulsador
unsigned long buttonReleaseTime = 0; // Tiempo en el que se soltó el pulsador
unsigned long tiempo_inactivo=0;
bool mensaje_traducido=false; // indica si la palabra actual se ha traducido o no
// tiempos para puntos y líneas
const unsigned long DOT_MIN = 100;     // Tiempo mínimo para un punto (en ms)
const unsigned long DOT_MAX = 300;     // Tiempo máximo para un punto (en ms)
const unsigned long DASH_MIN = 600;    // Tiempo mínimo para una raya (en ms)
const unsigned long DASH_MAX = 800;    // Tiempo máximo para una raya (en ms)
const unsigned long LETTER_GAP = 450;  // Tiempo para detectar el fin de una letra (en ms)
const unsigned long WORD_GAP = 1400;   // Tiempo para detectar el fin de una palabra (en ms)

byte i2c_rcv;                          // Dato recibido desde el bus I2C
unsigned long startTime;
bool onpress = false;
unsigned long durationTime;

String morseCode = "";                 // Cadena para almacenar la secuencia de puntos y rayas
String decodificado = "";              // Cadena para almacenar la decodificacion y pasarlos a letras
String receivedMessage = "";           // Cadena para almacenar el mensaje completo
unsigned long lastPressTime = 0;       // Tiempo de la última pulsación
unsigned long lastLetterTime = 0;      // Tiempo de la última letra completada

int letterCount = 0;                   // Contador de letras enviadas
String mensaje = "";

unsigned long time_start = 0;          // Tiempo de inicio en milisegundos
int stat_LED = 0;                      // Estado del LED: 1 = ENCENDIDO, 0 = APAGADO

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);    // Configurar el pin del pulsador como entrada con resistencia pull-up interna
  pinMode(sendButtonPin, INPUT_PULLUP);// Configurar el pin del botón para enviar datos como entrada con resistencia pull-up interna
  pinMode(ledPin, OUTPUT);             // Configurar el pin del LED externo como salida
  pinMode(builtInLedPin, OUTPUT);      // Configurar el pin del LED incorporado como salida
  Serial.begin(9600);                  // Inicializar comunicación serie a 9600 bps
  Wire.begin();                        // Iniciar I2C como maestro
  Wire.onReceive(dataRcv);             // Registrar la función de evento para la recepción de datos I2C
}

void loop() {
  // Leer el estado del pulsador para Morse
  buttonState = digitalRead(buttonPin);
  byte v = analogRead(A0);
  Wire.beginTransmission(0x08);
  Wire.write(v);
  Wire.endTransmission();

  //Serial.println("Mandando Bytes: " + String(v));
  
  if (Serial.available() > 0) {
    String mensaje = Serial.readStringUntil('\n');
    enviarMensajeMorse(mensaje);

  }
  

  // Detectar cambio en el estado del pulsador
  if (buttonState != lastButtonState) {
    buttonState = digitalRead(buttonPin); // Leer nuevamente el estado del pulsador
    if (buttonState == HIGH && lastButtonState == LOW) {
      buttonPressTime = millis(); // Guardar el tiempo en el que se presionó el pulsador
      digitalWrite(ledPin, HIGH); // Encender LED externo
      digitalWrite(builtInLedPin, HIGH); // Encender LED incorporado
    } else if (buttonState == LOW && lastButtonState == HIGH) {
      durationTime = millis() - buttonPressTime; // Calcular la duración de la pulsación
      digitalWrite(ledPin, LOW); // Apagar LED externo
      digitalWrite(builtInLedPin, LOW); // Apagar LED incorporado

      // Interpretar duración como punto o raya (mostrar lo que manda el maestro al esclavo)
      //if (durationTime >= DOT_MIN && durationTime <= DOT_MAX) {
      //  Serial.println("   holaaa.");
      //  morseCode += ".";
      //} else if (durationTime >= DASH_MIN && durationTime <= DASH_MAX) {
      //  Serial.println("   chaooo-");
      //  morseCode += "-";
      //}
    }
    lastButtonState = buttonState; // Actualizar el estado anterior del pulsador
  }

  Wire.requestFrom(0x08, 1);
  if (Wire.available()) {
    i2c_rcv = Wire.read();
    // Manejo de datos recibidos
    
    if (i2c_rcv > 200 && !onpress) {
      tiempo_inactivo= millis();
      startTime = millis();
      onpress = true;
      digitalWrite(ledPin, HIGH);       // Encender LED externo
      digitalWrite(builtInLedPin, HIGH);// Encender LED incorporado
    } else if (i2c_rcv < 70 && onpress) {
      durationTime = millis() - startTime;
      onpress = false;
      digitalWrite(ledPin, LOW);        // Apagar LED externo
      digitalWrite(builtInLedPin, LOW); // Apagar LED incorporado

      // Interpretar duración como punto o raya
      if (durationTime >= DOT_MIN && durationTime <= DOT_MAX) {
        Serial.print(".");
        morseCode += ".";
        tiempo_inactivo= millis();
        mensaje_traducido=true;
       
      } else if (durationTime >= DASH_MIN && durationTime <= DASH_MAX) {
        Serial.print("-");
        morseCode += "-";
        tiempo_inactivo= millis();
        mensaje_traducido=true;

      }
    }
      //Serial.println(millis() - tiempo_inactivo >= 700);
      if (mensaje_traducido && (millis() - tiempo_inactivo >= 600)) {
        decodificado = morseToChar(morseCode);
        if (decodificado != '?') {
          Serial.println(" -> " + String(decodificado));
          mensaje += String(decodificado);
        }
        morseCode = "";
        tiempo_inactivo=millis();
        mensaje_traducido=false;
         
      }

      if (mensaje.length() == 5 || (mensaje.length() > 0 && (millis() - tiempo_inactivo >= 1400))) {
        Serial.println("Mensaje: " + String(mensaje));
        Serial.println();
        morseCode = "";
        tiempo_inactivo=millis();
        mensaje_traducido=false;
        mensaje = "";
         
      }
  }


  delay(50);
}

// Función que se llama cuando se reciben datos I2C
void dataRcv(int numBytes) {
  while (Wire.available()) {
    i2c_rcv = Wire.read();
  }
}

// Función para traducir una secuencia de código Morse a una letra
char morseToChar(String morse) {
  if (morse == ".-") return 'A';
  else if (morse == "-...") return 'B';
  else if (morse == "-.-.") return 'C';
  else if (morse == "-..") return 'D';
  else if (morse == ".") return 'E';
  else if (morse == "..-.") return 'F';
  else if (morse == "--.") return 'G';
  else if (morse == "....") return 'H';
  else if (morse == "..") return 'I';
  else if (morse == ".---") return 'J';
  else if (morse == "-.-") return 'K';
  else if (morse == ".-..") return 'L';
  else if (morse == "--") return 'M';
  else if (morse == "-.") return 'N';
  else if (morse == "---") return 'O';
  else if (morse == ".--.") return 'P';
  else if (morse == "--.-") return 'Q';
  else if (morse == ".-.") return 'R';
  else if (morse == "...") return 'S';
  else if (morse == "-") return 'T';
  else if (morse == "..-") return 'U';
  else if (morse == "...-") return 'V';
  else if (morse == ".--") return 'W';
  else if (morse == "-..-") return 'X';
  else if (morse == "-.--") return 'Y';
  else if (morse == "--..") return 'Z';
  else return '?'; // Retornar '?' si la secuencia no coincide con ninguna letra
}


// Función para enviar un mensaje en código Morse a través de I2C
void enviarMensajeMorse(String mensaje) {
  for (int i = 0; i < mensaje.length(); i++) {
    char c = toupper(mensaje.charAt(i));
    String morse = charToMorse(c);
    enviarMorse(morse);
    delay(LETTER_GAP);
  }
}


// Función para convertir un carácter a código Morse
String charToMorse(char c) {
  switch (c) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    default: return "";
  }
}


// Función para enviar código Morse
void enviarMorse(String morse) {
  for (int i = 0; i < morse.length(); i++) {
    char c = morse.charAt(i);
    if (c == '.') {
      // Enviar una señal durante 200 ms
      Wire.beginTransmission(0x08);
      Wire.write(250);  // Envía el dato correspondiente al punto
      Wire.endTransmission();
      delay(200);  // Espera 200 ms para representar un punto
    } else if (c == '-') {
      // Enviar una señal durante 600 ms
      Wire.beginTransmission(0x08);
      Wire.write(250);  // Envía el dato correspondiente a la raya
      Wire.endTransmission();
      delay(600);  // Espera 600 ms para representar una raya
    }
    Wire.beginTransmission(0x08);
    Wire.write(0);  // Envía un byte 0 para indicar el fin de la señal
    Wire.endTransmission();
    delay(DOT_MAX);  // Espera antes de enviar el siguiente elemento Morse
  }
  delay(LETTER_GAP);  // Espera entre letras según la secuencia Morse
}
