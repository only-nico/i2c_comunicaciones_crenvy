#include <Wire.h>

int buttonState = 0;              // Variable para leer el estado del pulsador
const int buttonPin = A0;         // Pin del Pulsador y para Morse
const int ledPin = 4;             // Pin del LED externo
const int builtInLedPin = 13;     // Pin del LED incorporado

// Definición de constantes de tiempo para Morse
const unsigned long DOT_MIN = 150;     // Tiempo mínimo para un punto (en ms)
const unsigned long DOT_MAX = 250;     // Tiempo máximo para un punto (en ms)
const unsigned long DASH_MIN = 550;    // Tiempo mínimo para una raya (en ms)
const unsigned long DASH_MAX = 650;    // Tiempo máximo para una raya (en ms)
const unsigned long LETTER_GAP = 600;  // Tiempo para detectar el fin de una letra (en ms)
const unsigned long WORD_GAP = 1400;   // Tiempo para detectar el fin de una palabra (en ms)

byte i2c_rcv;                        // Dato recibido desde el bus I2C
unsigned long startTime;
bool onpress = false;
bool onpressLocal = false;
unsigned long durationTime;

byte i2c_rqst;

String morseCode = "";               // Cadena para almacenar la secuencia de puntos y rayas
String receivedMessage = "";         // Cadena para almacenar el mensaje completo
unsigned long lastPressTime = 0;     // Tiempo de la última pulsación
unsigned long lastLetterTime = 0;    // Tiempo de la última letra completada

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);   // Configurar el pin del pulsador como entrada con resistencia pull-up interna
  pinMode(ledPin, OUTPUT);            // Configurar el pin del LED externo como salida
  pinMode(builtInLedPin, OUTPUT);     // Configurar el pin del LED incorporado como salida
  Wire.begin(0x08);                   // Unirse al bus I2C con la dirección 0x08
  Wire.onReceive(dataRcv);            // Registrar la función de evento para la recepción de datos I2C
  Wire.onRequest(dataRqst);           // Registrar la función de evento para las solicitudes de datos I2C
  Serial.begin(9600);                 // Inicializar comunicación serie a 9600 bps
}

void loop() {
  // Leer el estado del pulsador para Morse
  buttonState = digitalRead(buttonPin);
  i2c_rqst = analogRead(buttonPin);
  // Si el botón para Morse está presionado
  if (Serial.available() > 0) {
    String mensaje = Serial.readStringUntil('\n');
    
    enviarMensajeMorse(mensaje);

  }
  if (buttonState == HIGH && onpressLocal == false) {
    // Leer valor analógico del pin A0
    digitalWrite(ledPin,HIGH);
    digitalWrite(builtInLedPin, LOW);
    onpressLocal = true;
  } else if (buttonState == LOW && onpressLocal == true) {
    digitalWrite(ledPin, LOW);
    digitalWrite(builtInLedPin, HIGH);
    onpressLocal = false;
  }

  // Detectar fin de una letra basado en el tiempo
  if (!onpress && millis() - lastPressTime > LETTER_GAP && morseCode.length() > 0) {
    char letra = morseToChar(morseCode);
    Serial.println(" -> Letra recibida: " + String(letra));
    receivedMessage += letra;         // Agregar la letra al mensaje recibido
    morseCode = "";                   // Reiniciar el código Morse para la siguiente letra
    lastPressTime = millis();        // Actualizar el tiempo de la última letra
    Serial.println();
  }

  if(receivedMessage.length() == 5){
    Serial.println("Palabra recibida: " + receivedMessage);
    receivedMessage = "";             // Reiniciar el mensaje recibido para la siguiente palabra 
  }
  // Detectar fin de una palabra basado en el tiempo y mostrar el mensaje completo
  else if (receivedMessage.length() > 0 && millis() - lastPressTime > WORD_GAP) {
    Serial.println("Palabra recibida: " + receivedMessage);
    receivedMessage = "";             // Reiniciar el mensaje recibido para la siguiente palabra
  }

}

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

void enviarMensajeMorse(String mensaje) {
  for (int i = 0; i < mensaje.length(); i++) {
    char c = toupper(mensaje.charAt(i));
    String morse = charToMorse(c);
    
    enviarMorse(morse);

    delay(LETTER_GAP);
  }
}

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

void dataRqst() {
  Wire.write(i2c_rqst);                // Enviar dato solicitado a través de I2C
  //Serial.println("Enviando: " + String(i2c_rqst));
}

// Función que se llama cuando se reciben datos I2C
void dataRcv(int numBytes) {
  while (Wire.available()) {
    i2c_rcv = Wire.read();
    // Serial.println("recibo: " + String(i2c_rcv));
    // Procesar datos recibidos como señales Morse
    if (i2c_rcv > 70 && !onpress) {
      //Serial.println("Presionado");
      startTime = millis();
      onpress = true;
      digitalWrite(ledPin, HIGH);       // Encender LED externo
      digitalWrite(builtInLedPin, HIGH);// Encender LED incorporado
      //lastPressTime = millis();
    } else if (i2c_rcv < 70 && onpress) {
      durationTime = millis() - startTime;
      onpress = false;
               // Actualizar el tiempo de la última pulsación
      digitalWrite(ledPin, LOW);        // Apagar LED externo
      digitalWrite(builtInLedPin, LOW); // Apagar LED incorporado

      // Interpretar duración como punto o raya
      if (durationTime >= DOT_MIN && durationTime <= DOT_MAX) {
        Serial.print("   .");
        morseCode += ".";
        lastPressTime = millis();
      } else if (durationTime >= DASH_MIN && durationTime <= DASH_MAX) {
        Serial.print("   -");
        morseCode += "-";
        lastPressTime = millis();
      }
    }
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
