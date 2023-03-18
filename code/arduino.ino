/* Connessione al cloud Blynk */
#define BLYNK_TEMPLATE_ID "***"
#define BLYNK_TEMPLATE_NAME "***"
#define BLYNK_AUTH_TOKEN "***"

// Credenziali WiFi
char ssid[] = "***" ;
char pass[] = "***" ;

// Distanza dal sensore ad ultrasuoni in CM
int emptyTankDistance = 70 ;  // Distanza quando il serbatoio è vuoto
int fullTankDistance =  30 ;  // Distanza quando il serbatoio è pieno

// Valore di attivazione in percentuale
int triggerPer =   10 ;  // L'allarme si attiva quando il livello dell'acqua scende sotto la soglia

#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <AceButton.h>

using namespace ace_button; 

// Collegamenti al sensore
#define TRIGPIN    27  //G27
#define ECHOPIN    26  //G26
#define wifiLed    2   //G2
#define ButtonPin1 4  //G4
#define BuzzerPin  13  //G13
#define GreenLed   14  //G14

// Pin virtuali per Blynk
#define VPIN_BUTTON_1    V1 
#define VPIN_BUTTON_2    V2

#define SCREEN_WIDTH 128 // Larghezza del display OLED in pixel
#define SCREEN_HEIGHT 32 // Altezza del display OLED in pixel

// Dichiarazione del display SSD1306 connesso tramite I2C (pin SDA, SCL)
#define OLED_RESET     -1 // Pin di reset
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


float duration;
float distance;
int   waterLevelPer;
bool  toggleBuzzer = HIGH;  //Variabile per ricordare lo stato di toggle

char auth[] = BLYNK_AUTH_TOKEN;

ButtonConfig config1;
AceButton button1(&config1);

void handleEvent1(AceButton*, uint8_t, uint8_t);

BlynkTimer timer;

void checkBlynkStatus() { // chiamata ogni 3 secondi da SimpleTimer

  bool isconnected = Blynk.connected();
  if (isconnected == false) {
    Serial.println("Blynk Not Connected");
    digitalWrite(wifiLed, LOW);
  }
  if (isconnected == true) {
    digitalWrite(wifiLed, HIGH);
    Serial.println("Blynk Connected");
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_BUTTON_1);
  Blynk.syncVirtual(VPIN_BUTTON_2);
}

void displayData(int value){
  display.clearDisplay();
  display.setTextSize(4);
  display.setCursor(8,2);
  display.print(value);
  display.print(" ");
  display.print("%");
  display.display();
}

/* 
  Metodo che serve a misurare la distanza in base al livello dell'acqua. 
*/
void measureDistance()
{
// Imposta il pin del trigger a basso per 2 uS
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
 
// Imposta il pin del trigger ad alto per 20 uS per inviare un impulso
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(20);
 
// Riporta il pin del trigger a basso
  digitalWrite(TRIGPIN, LOW);
 
// Misura la larghezza dell'impulso in ingresso
  duration = pulseIn(ECHOPIN, HIGH);
 
// Determina la distanza dalla durata
// Usa 343 metri al secondo come velocità del suono
// Dividi per 1000 in quanto si vuole la distanza in millimetri
 
  distance = ((duration / 2) * 0.343)/10;

  if (distance > (fullTankDistance - 10)  && distance < emptyTankDistance )
  {
    waterLevelPer = map((int)distance ,emptyTankDistance, fullTankDistance, 0, 100);
    displayData(waterLevelPer);
    Blynk.virtualWrite(VPIN_BUTTON_1, waterLevelPer);
    Blynk.virtualWrite(VPIN_BUTTON_2, (String(distance) + " cm"));

// Stampa il risultato nel monitor seriale
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (waterLevelPer < triggerPer)
    {
      digitalWrite(GreenLed, HIGH);
      if (toggleBuzzer == HIGH){
        digitalWrite(BuzzerPin, HIGH);
      }      
    }
    if (distance < fullTankDistance)
    {
      digitalWrite(GreenLed, LOW);
      if (toggleBuzzer == HIGH){
        digitalWrite(BuzzerPin, HIGH);
      } 
    }

    if (distance > (fullTankDistance + 5) && waterLevelPer > (triggerPer + 5))
    {
      toggleBuzzer = HIGH;
      digitalWrite(BuzzerPin, LOW);
    }        
}
  
// Attende prima di ripetere la misura
  delay(100);
}

/*
   Questa funzione viene eseguita una sola volta all'avvio del microcontrollore e si occupa di inizializzare il
   programma.
   
   Vengono impostati i pin per le connessioni del sensore ad ultrasuoni, del LED wifi e del pulsante.
   
   Viene inoltre impostata la modalità di funzionamento del pulsante e associata una funzione di gestione degli eventi
   (in questo caso 'button1Handler').
   
   Viene quindi inizializzato il display OLED e avviata la connessione WiFi con le credenziali specificate.
   
   Viene poi configurato il timer 'SimpleTimer' per controllare la connessione al server Blynk ogni 2 secondi, e infine
   viene configurato il server Blynk con l'autenticazione.
   
   Questa funzione è importante perché permette di inizializzare correttamente il programma e di impostare i parametri
   necessari per il corretto funzionamento del sensore ad ultrasuoni, del display OLED, del pulsante e della connessione
   WiFi con il server Blynk.
*/
void setup() 
{
// Imposta il monitor seriale
  Serial.begin(115200);
 
// Imposta le modalità dei pin per le connessioni del sensore
  pinMode(ECHOPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(wifiLed, OUTPUT);
  pinMode(GreenLed, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);

  pinMode(ButtonPin1, INPUT_PULLUP);

  digitalWrite(wifiLed, LOW);
  digitalWrite(GreenLed, LOW);
  digitalWrite(BuzzerPin, LOW);

  config1.setEventHandler(button1Handler);
  
  button1.init(ButtonPin1);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(5000);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();

  WiFi.begin(ssid, pass);
  timer.setInterval(2000L, checkBlynkStatus); // Controlla la connessione al server Blynk ogni 2 secondi
  Blynk.config(auth);
  delay(1000);
 
}

/*
   Questa funzione viene eseguita continuamente dal microcontrollore e si occupa di chiamare le funzioni 
   necessarie per il funzionamento del programma. 
   
   In particolare, viene eseguita la funzione 'measureDistance()' per misurare la distanza con un sensore ad ultrasuoni
   e aggiornare il valore della distanza all'interno del programma.
   
   Viene poi chiamata la funzione 'Blynk.run()' per mantenere attiva la connessione al server Blynk e gestire la comunicazione.
   
   Infine, viene eseguita la funzione 'timer.run()' per avviare il timer 'SimpleTimer' che eseguirà le funzioni
   registrate quando è stato configurato.
   
   La funzione 'button1.check()' verifica inoltre se il pulsante è stato premuto e, se sì, esegue la funzione di gestione
   dell'evento associata.
*/
 void loop() 
 {
  measureDistance();

  Blynk.run();
  timer.run(); // Avvia SimpleTimer

  button1.check();
}

/*
   Questa funzione gestisce gli eventi relativi ad un pulsante. 
   Riceve in input un puntatore ad un oggetto AceButton, un intero senza segno a 8 bit che rappresenta
   il tipo di evento verificatosi (ad esempio, pulsante premuto, pulsante rilasciato, pulsante tenuto premuto)
   e un intero senza segno a 8 bit che rappresenta lo stato corrente del pulsante.
   Se l'evento è il rilascio del pulsante, la funzione spegne un buzzer e imposta un flag su LOW.
*/
void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) 
{
  Serial.println("EVENT1");

  switch (eventType) {
    case AceButton::kEventReleased:
      //Serial.println("kEventReleased");
      digitalWrite(BuzzerPin, LOW);
      toggleBuzzer = LOW;
      break;
  }
}