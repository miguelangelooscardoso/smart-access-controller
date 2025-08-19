#include <Key.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <MFRC522.h>
#include "EduIntro.h"
#include "Servo.h"

// fazer sem biblioteca keypad

#define LINHAS 4
#define COLUNAS 4
#define SS_PIN 53  // SDA
#define RST_PIN 5  // RST
#define LDR A0
#define servo1Pin A8
#define servo2Pin A9
#define servo3Pin A10
#define relay 47

const char hexaKeys[LINHAS][COLUNAS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

String variaveisMenu[4] = { "<  Temperature >", "<   Humidity   >", "<  Luminosity  >", "<     Exit     >" };
String variaveisMenu2[6] = { "<   Temp Real  >", "<   Set Temp   >", "<   Hum Real   >", "<   Set Hum    >", "<    Lum Real  >", "<    Set Lum   >" };

const byte pinLinhas[LINHAS] = { 2, 3, 4, 5 };
const byte pinColunas[COLUNAS] = { 6, 7, 8, 9 };

Keypad teclado = Keypad(makeKeymap(hexaKeys), pinLinhas, pinColunas, LINHAS, COLUNAS);
LiquidCrystal lcd(36, 37, 30, 31, 32, 33);
MFRC522 mfrc522(SS_PIN, RST_PIN);
// Tag UID: 0F A4 D6 C3
// Card UID: 66 8B F8 04
DHT11 dht11(10); //pin10
Servo myservo1;
Servo myservo2;
Servo myservo3;

int C;  //temperature in celsius
int H;  // humidity (relative percentage)
int L;
int variaveisMenu2_DHT11[6] = { C, 27, H, 35, L, 15 };
int optionButton = 22;
int selectButton = 24;

int red = 11;
int yellow = 12;
int green = 13;

String chave = "";
String chaveUser = "#";
String chaveAdmin = "1#";
String adminId = "fa4d6c3";
String userId = "668bf84";
int pos_underscore = 1;
int pos_numero = 0;
int ultimo_numero = 0;  // ms
int tentativas = 0;
bool isAdmin;
int lastCardRead = 0;
bool isDetected = false;
bool isValid = false;
int opcaoMenu = 1;
int opcaoMenu2 = 1;
int optionButtonState;
int option2ButtonState;
int selectButtonState;
unsigned long lastOptionButtonPressTime = 0;
unsigned long lastSelectButtonPressTime = 0;
unsigned long lastOption2ButtonPressTime = 0;
bool select = false;
bool isCoolerOn = false;
bool isHumidityLow = false;
int angle1 = 0;
int angle2 = 0;
int angle3 = 0;
// bool isLightUp = false;

void setup() {
  Serial.begin(9600);  // Inicia comunicação serial
  lcd.begin(16, 2);    // inicia lcd

  lcd.print("Starting System");
  delay(500);
  lcd.clear();
  lcd.print("System On");
  delay(500);
  lcd.clear();

  SPI.begin();         // iniciar SPI bus
  mfrc522.PCD_Init();  // iniciar MFRC522
  myservo1.attach(servo1Pin);
  myservo2.attach(servo2Pin);

  // Serial.println("Aproxime o cartão do leitor.");
  // Serial.println();
  // lcd.setCursor(0, 0);
  // lcd.print("Please pass");
  // lcd.setCursor(0, 1);
  // lcd.print("your card!");

  pinMode(optionButton, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);

  pinMode(LDR, OUTPUT);

  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);

  pinMode(relay, OUTPUT);
}

void loop() {

  // digitalWrite(red, HIGH);
  if (isCoolerOn) {
    servo1();
  }

  if (isHumidityLow) {
    servo2();
  }

  // if (isLightUp) {
  //  digitalWrite(relay, HIGH);
  // }
  // else {
  //   digitalWrite(relay, LOW);
  // }

  verificaCondicoes();

  dht11.update();

  variaveisMenu2_DHT11[0] = dht11.readCelsius();
  variaveisMenu2_DHT11[2] = dht11.readHumidity();

  int ldr = analogRead(LDR);
  variaveisMenu2_DHT11[4] = ldr;


  if (isValid && isDetected) {
    digitalWrite(red, LOW);
    digitalWrite(green, HIGH);

    if (!select) {
      optionButtonFunction();
    }
    selectButtonFunction();
    menu();


  } else {
    // isDetected = rfid();
    rfid();
    if (isDetected) {

      //Serial.print("Entrei no if validar pin");
      checkPin();
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Please pass");
      lcd.setCursor(0, 1);
      lcd.print("your card!");
    }
    digitalWrite(red, HIGH);
    digitalWrite(green, LOW);
  }
}

void rfid() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    // Serial.print("Não foi detetado nenhum cartão, inicia novo loop!");
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    // Serial.println("O cartão foi detetado!");
    return;
  }


  if (millis() - lastCardRead > 500) {
    // Serial.print("UID tag :");
    String cardId = "";
    // Serial.println(mfrc522.uid.size);
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      // Serial.println(mfrc522.uid.uidByte[i], HEX);
      cardId.concat(String(mfrc522.uid.uidByte[i], HEX));
    }

    if (cardId == userId) {
      Serial.print("User ID: ");
      Serial.println(cardId);
      isAdmin = false;
      isDetected = true;
      // lastCardRead = millis();
      Serial.println("Granted Access!");
    } else if (cardId == adminId) {
      Serial.print("Admin ID: ");
      Serial.println(cardId);
      isAdmin = true;
      isDetected = true;
      // lastCardRead = millis();
      Serial.println("Granted Access!");
    } else {
      Serial.println("Access Denied!");
    }
    lastCardRead = millis();
  }
}

void checkPin() {

  char numero = teclado.getKey();  // Obtém a chave pressionada

  if (numero) {
    chave += numero;
    Serial.println(numero);  // Imprime o valor da chave no monitor serial
    pos_underscore++;
    pos_numero++;
    ultimo_numero = millis();
  }

  lcd.setCursor(0, 0);
  lcd.print("Enter your Pin:");
  lcd.setCursor(0, 1);
  lcd.print(">");
  lcd.setCursor(pos_numero, 1);  // 1


  if (numero) {
    lcd.print(chave.charAt(pos_numero - 1));  // 0
    lcd.setCursor(pos_numero - 1, 1);
    lcd.print('*');
  }

  if (millis() - ultimo_numero > 800 && ultimo_numero > 0) {
    lcd.setCursor(pos_numero, 1);
    lcd.print('*');
  }

  lcd.setCursor(pos_underscore, 1);  // 1
  lcd.print("_               ");

  if (numero == '#') {

    if (!isAdmin && chave != chaveUser) {
      // Serial.print("SOU USER");
      lcd.setCursor(1, 1);
      lcd.print("Access Denied!");
      delay(500);
      pos_underscore = 1;
      pos_numero = 0;
      ultimo_numero = 0;
      chave = "";
      tentativas++;
      if (tentativas > 2) {
        lcd.setCursor(0, 1);
        lcd.print("Account Blocked!");
        digitalWrite(red, LOW);
        digitalWrite(green, LOW);
        digitalWrite(yellow, HIGH);
        while (true) {
          // do nothing
        }
      }
    } else if (isAdmin && chave != chaveAdmin) {
      // Serial.print("SOU ADMIN e nao tenho a chave validade");
      lcd.setCursor(1, 1);
      lcd.print("Access Denied!");
      delay(500);
      pos_underscore = 1;
      pos_numero = 0;
      ultimo_numero = 0;
      chave = "";
      tentativas++;
      if (tentativas > 2) {
        lcd.setCursor(0, 1);
        lcd.print("Account Blocked!");
        digitalWrite(red, LOW);
        digitalWrite(green, LOW);
        digitalWrite(yellow, HIGH);
        while (true) {
          // do nothing
        }
      }
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Granted Access!");
      delay(500);
      isValid = true;
      chave = "";
      pos_underscore = 1;
      pos_numero = 0;
      ultimo_numero = 0;
    }
  }
}

void menu() {
  // lcd.clear();

  switch (opcaoMenu) {
    case 1:
      menu2();
      return;
    case 2:
      menu2();
      return;
    case 3:
      menu2();
      return;
    case 4:
      menu2();
      if (select) {
        isValid = false;
        isDetected = false;
        opcaoMenu = 1;
        select = !select;
      }
      return;
    default:
      opcaoMenu = 1;
      return;
  }
}

void menu2() {
  if (!select) {
    lcd.setCursor(0, 0);
    lcd.print("      Menu      ");
    lcd.setCursor(0, 1);
    // lcd.print("<" + variaveis[0] +">");
    lcd.print(variaveisMenu[opcaoMenu - 1]);
    // Serial.print(select);
    selectButtonFunction();
  } else {
    // Serial.println("Entrou");
    // Serial.print(select);
    optionButtonFunction2();

    switch (opcaoMenu2) {
      case 1:
        lcd.setCursor(0, 0);
        // lcd.print(variaveisMenu2[opcaoMenu2 + 0]);
        lcd.print(variaveisMenu2[(opcaoMenu - 1) * 2 + opcaoMenu2 - 1]);
        lcd.setCursor(0, 1);
        lcd.print("       ");
        lcd.print(variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 1]);
        lcd.print("        ");
        // opcaoMenu = 1;
        return;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print(variaveisMenu2[(opcaoMenu - 1) * 2 + opcaoMenu2 - 1]);
        lcd.setCursor(0, 1);
        lcd.print("       ");
        lcd.print(variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 1]);
        lcd.print("        ");
        if (selectButtonState == LOW && (millis() - lastSelectButtonPressTime) > 250 && isAdmin) {
          // Serial.print(selectButtonState);
          setValue((opcaoMenu - 1) * 2 + opcaoMenu2 - 1);
          lastSelectButtonPressTime = millis();
        }
        // opcaoMenu = 1;
        return;
      default:
        opcaoMenu2 = 1;
        return;
    }
  }
}

void optionButtonFunction() {
  optionButtonState = digitalRead(optionButton);
  if (optionButtonState == LOW && (millis() - lastOptionButtonPressTime) > 500) {
    opcaoMenu++;
    if (opcaoMenu > 4) {
      opcaoMenu = 1;
    }
    lastOptionButtonPressTime = millis();
  }
}

void optionButtonFunction2() {
  option2ButtonState = digitalRead(optionButton);
  if (option2ButtonState == LOW && (millis() - lastOption2ButtonPressTime) > 500) {
    opcaoMenu2++;
    // opcaoMenu--;
    if (opcaoMenu2 > 3) {
      opcaoMenu2 = 1;
    }
    lastOption2ButtonPressTime = millis();
  }
}

void selectButtonFunction() {
  selectButtonState = digitalRead(selectButton);
  if (selectButtonState == LOW && (millis() - lastSelectButtonPressTime) > 500) {
    select = !select;
    lastSelectButtonPressTime = millis();
  }
}

void setValue(int index) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Set Value:   ");
  lcd.setCursor(0, 1);
  String input = "";
  char key;
  while (true) {
    key = teclado.getKey();
    if (key) {
      if (key == '#') {
        break;
      } else if (key == '*') {
        if (input.length() > 0) {
          input.remove(input.length() - 1);
          lcd.setCursor(input.length(), 1);
          lcd.print(" ");
        }
      } else if (key >= '0' && key <= '9') {
        input += key;
        lcd.setCursor(input.length() - 1, 1);
        lcd.print(key);
      }
    }
  }
  if (input.length() > 0) {
    variaveisMenu2_DHT11[index] = input.toInt();
  }
  lcd.clear();
}

void servo1() {

  // for (int angle = 0; angle <= 180; angle++) {
  //   myservo.write(angle);
  //   // delay(10);
  // }

  // for (int angle = 180; angle >= 0; angle--) {
  //   myservo.write(angle);
  //   // delay(10);
  // }

  if (angle1 <= 180) {
    myservo1.write(angle1);
    angle1 += 10;
    //delay(15);
  } else {
    angle1 = 0;
    myservo1.write(angle1);
  }


  // if (angle > 0 && angle <= 180) {
  //   myservo.write(angle);
  //   angle--;
  //   delay(15);
  // }
}

void servo2() {

  if (angle2 <= 180) {
    myservo2.write(angle2);
    angle2 += 10;
    //delay(15);
  } else {
    angle2 = 0;
    myservo2.write(angle2);
  }
}

void verificaCondicoes() {

  // if (variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 2] > variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 1]) {
  if (variaveisMenu2_DHT11[0] > variaveisMenu2_DHT11[1]) {
    Serial.print("Temp real: ");
    // Serial.println(variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 2]);
    Serial.println(variaveisMenu2_DHT11[0]);
    Serial.print("Set temp ");
    // Serial.println(variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 1]);
    Serial.println(variaveisMenu2_DHT11[1]);
    isCoolerOn = true;

  } else {
    isCoolerOn = false;
  }

  if (variaveisMenu2_DHT11[2] > variaveisMenu2_DHT11[3]) {
    // Serial.print("Hum real: ");
    // // Serial.println(variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 2]);
    // Serial.println(variaveisMenu2_DHT11[0]);
    // Serial.print("Hum temp ");
    // // Serial.println(variaveisMenu2_DHT11[(opcaoMenu - 1) * 2 + opcaoMenu2 - 1]);
    // Serial.println(variaveisMenu2_DHT11[1]);
    isHumidityLow = true;

  } else {
    isHumidityLow = false;
  }

  if (variaveisMenu2_DHT11[4] < variaveisMenu2_DHT11[5]) {
    // isLightUp = true;
    digitalWrite(relay, HIGH);
  } else {
    // isLightUp = false;
    digitalWrite(relay, LOW);
  }
}