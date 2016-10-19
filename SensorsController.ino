#include <SPI.h>
#include <Ethernet.h>
#include <SimpleTimer.h>

#include <LiquidCrystal.h>
#include <Limits.h>

#include <G00101.h>
#include <G10101.h>


/*
 * Arduino parameters
 */
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ipArduino( 192, 168, 1, 65 );


/*
 * Server parameters
 */
//char server[] = "renanrfs.no-ip.org";
IPAddress server( 85, 241, 197, 200 );
unsigned serverPort = 8080;

EthernetClient client;

/*
 * Buffer message
 */
G00101 g001;
G10101 g101;

/**
 * Multitasking variables
 */
SimpleTimer timer;
const unsigned long stayAliveDelay = 15 * 60000;
const unsigned long sensorDelay = 1 * 60000;

/*
 * Sensor values
 */
unsigned int sensorCount = 0;

// light 
const unsigned int lightSensorA0 = 0;
unsigned int lightSensorValue = 0;
unsigned int sumLight = 0;

// Temp
const unsigned int temperatureSensorA1 = 1; //Pino analógico que o sensor de temperatura está conectado.
 
//Variáveis
int temperatureSensorValue = 0;  //usada para ler o valor do sensor de temperatura.
int menorValorTemp  = INT_MAX; //usada para armazenar o menor valor da temperat
unsigned int sumTemp = 0;

//Criando um objeto da classe LiquidCrystal e 
//inicializando com os pinos da interface.
LiquidCrystal lcd(9, 8, 5, 4, 3, 2);


/**
 * Send the G001 message
 */
void sendRequest() {

  client.println("POST /grlp/processor HTTP/1.1");
  client.println("Host: 85.241.197.200:8080");
  client.print("Content-Length: ");
  //client.println(g001.get_length());
  client.println(54);
  client.println("User-Agent: SensorControllerRequest/0.0.1");
  client.println("Connection: close");
  client.println();

  // write message
  client.println(g001.get_buffer());
  client.println();

  // Give the web browser time to receive the data
  delay(100);

  // stay in this loop until the server closes the connection
  while (client.connected()) {
    while (client.available()) {
      char read_char = client.read();
      //Serial.print(read_char);
    }
  }
}

/**
 * Send Stay Alive message:
 * 	request - G001;
 * 	response - G101;
 *
 * - send sensors informations
 */
void sendStayAliveMessage() {

  // if you get a connection, report back via serial:
  if (client.connect(server, serverPort)) {
    Serial.println("connected!");

    g001.init();
    g001.trace();
  
    // make a HTTP request
    g001.messageCode.set("G001");
    g001.messageVersion.set("01");    
    g001.growllerId.set(12);  
    g001.transactionDate.set("20160126114520");
    g001.token.set("token");
    g001.ipAddress.set("192.168.123.123");
    g001.lightSensor.set(sumLight / sensorCount);
    g001.temperatureSensor.set(sumTemp / sensorCount);
    g001.humiditySensor.set(333);    
    
    g001.trace();
    sendRequest();

    // Close the connection:
    client.stop();
    Serial.println("client closed!");
  } else {
    Serial.println("connection failed!");
  }

  // clean light sensor media
  sumLight = 0;
  sumTemp = 0;
  sensorCount = 0;
}

void sensorRead() {
    lightSensorValue = analogRead(lightSensorA0);

    //Para evitar as grandes variações de leitura do componente 
    //LM35 são feitas 8 leitura é o menor valor lido prevalece.  
    menorValorTemp  = INT_MAX; //Inicializando com o maior valor int possível
    for (int i = 1; i <= 8; i++) {
      //Lendo o valor do sensor de temperatura.
      temperatureSensorValue = analogRead(temperatureSensorA1);   
    
      //Transformando valor lido no sensor de temperatura em graus celsius aproximados.
      temperatureSensorValue *= 0.54 ; 
       
      //Mantendo sempre a menor temperatura lida
      if (temperatureSensorValue < menorValorTemp) {
        menorValorTemp = temperatureSensorValue;
      }
    
      delay(150); 
    }

    

    //Exibindo valor da leitura do sensor de temperatura no display LCD.
    lcd.clear();  //limpa o display do LCD.     
    lcd.print("Temp: ");  //imprime a string no display do LCD.                 
    lcd.print(menorValorTemp);
    lcd.write(B11011111); //Simbolo de graus celsius
    lcd.print("C");
    
    //Exibindo valor da leitura do sensor de luz no display LCD.
    lcd.setCursor(0,1);  //posiciona o cursor na coluna 0 linha 1 do LCD.
    lcd.print("Luz: ");  //imprime a string no display do LCD.       
    lcd.print(lightSensorValue);

    sumLight = sumLight + lightSensorValue;
    sumTemp = sumTemp + menorValorTemp;
    sensorCount++;

    
    Serial.print("Light Media: ");
    Serial.println(sumLight / sensorCount);
    Serial.print("Temp Media: ");
    Serial.println(sumTemp / sensorCount);
}

/**
 * The Arduino setup
 */
void setup() {

  Serial.begin(9600);

  //Inicializando o LCD e informando o tamanho de 16 colunas e 2 linhas
  //que é o tamanho do LCD JHD 162A usado neste projeto.
  lcd.begin(16, 2);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");

    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ipArduino);
  }

  // give the Ethernet shield a second to initialize:
  delay(1000);
  
  // timed actions setup
  timer.setInterval(sensorDelay, sensorRead);
  timer.setInterval(stayAliveDelay, sendStayAliveMessage);
}


/**
 * The main Arduino loop
 */
void loop() {
  // this is where the "polling" occurs
  timer.run();
}
