#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include "dht11.h"
#include <Wire.h>  

long previousMillis = 0;
long interval = 30000;
char code[13]; 
int pos = 0; 

//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_CMD    "iot/ex/comando"   //tópico MQTT de escuta
#define TOPICO_TEMP   "iot/ex/temp"      //tópico MQTT de envio de informações para Broker
#define TOPICO_UMID   "iot/ex/umid"      //tópico MQTT de envio de informações para Broker
#define TOPICO_STATUS "iot/ex/status"     //tópico MQTT de envio de informações para Broker
#define ID_MQTT  "ExemploMQTT1234"     //id mqtt (para identificação de sessão)
 
//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1

#define GPIO2 2

#define RELAY1 D5
#define RELAY2 D6
#define DHT11PIN D3

dht11 DHT11;

// WIFI
const char* SSID = "SUA_REDE_WIFI"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "SUA_SENHA_WIFI"; // Senha da rede WI-FI que deseja se conectar
  
// MQTT
const char* BROKER_MQTT = "mqtt.eclipse.org"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
 
//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient


//Prototypes
bool initWiFi();
void initMQTT();
bool reconnectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
bool VerificaConexoesWiFIEMQTT(void);
 
/* 
 *  Implementações das funções
 */
void setup() 
{
    Serial.begin(9600);

    delay(3000);

    Serial.println();Serial.println();
    Serial.println("Exemplo MQTT");

    // prepare GPIO2
    //pinMode(GPIO2, OUTPUT);
    //digitalWrite(GPIO2, LOW);

    pinMode(RELAY1, OUTPUT);
    digitalWrite(RELAY1, LOW);

    pinMode(RELAY2, OUTPUT);
    digitalWrite(RELAY2, LOW);

    delay(1000);

    Serial.print("DHT11 LIBRARY VERSION: ");
    Serial.println(DHT11LIB_VERSION);
    Serial.println();

    //inicializações:
    if (initWiFi())
    {
        initMQTT();
    }
}
  
bool initWiFi() 
{
    delay(45000);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
     
    return reconnectWiFi();
}
  
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
  
//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos subscritos chega)
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string da payload recebida
    for(int i = 0; i < length; i++) 
    {
      char c = (char)payload[i];
      msg += c;
    }

    if (msg.equals("ACENDE"))
    {
      digitalWrite(LED_BUILTIN, LOW);
    }

    int espera = 2000;

    if (msg.startsWith("RELAY1") || msg.startsWith("RELAY2"))
    {
      int pos = msg.indexOf(",");

      if (pos>0)
      {
        String sEspera = msg.substring(pos+1);
        Serial.print("sEspera = ");
        Serial.println(sEspera);

        char sespera[3];

        sEspera.toCharArray(sespera, 3);

        Serial.print("sespera = ");
        Serial.println(sespera);

        int iespera = atoi(sespera);

        Serial.print("iespera = ");
        Serial.println(iespera);

        espera = iespera * 1000;
      }
    }
    
    if (msg.startsWith("RELAY1"))
    {
      digitalWrite(RELAY1, HIGH);
      delay(espera);
      digitalWrite(RELAY1, LOW);

      if ((WiFi.status() == WL_CONNECTED) && (MQTT.connected()))
      {
        char dh[30];
        String sdh = getTime();
  
        sdh.toCharArray(dh, 30);
        MQTT.publish(TOPICO_STATUS, dh);
        Serial.print(TOPICO_STATUS);
        Serial.print(" -> ");
        Serial.println(dh);
      }
    }

    if (msg.startsWith("RELAY2"))
    {
      digitalWrite(RELAY2, HIGH);
      delay(espera);
      digitalWrite(RELAY2, LOW);
      
      char dh[30];
      String sdh = getTime();

      sdh.toCharArray(dh, 30);
      MQTT.publish(TOPICO_STATUS, dh);
      Serial.print(TOPICO_STATUS);
      Serial.print(" -> ");
      Serial.println(dh);
    }

    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(msg);     
}
  
bool reconnectMQTT() 
{
    bool ret = false;

    if (MQTT.connected())
    {
      ret = true;
    }
    else
    {
      if (MQTT.connect(ID_MQTT)) 
      {
          Serial.println("Conectado com sucesso ao broker MQTT!");
          MQTT.subscribe(TOPICO_CMD);
          ret = true;
      } 
      else
      {
          Serial.println("Falha ao reconectar no broker.");
      }
    }
      
    return ret;
}
  
bool reconnectWiFi() 
{
    bool ret = false;
    int attemptstimeout = 25;
    int attempts = 0;
    
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }
         
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
        yield();

        attempts++;

        if (attempts>attemptstimeout)
        {
          ESP.restart();
          break;
        }
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.print("Conectado com sucesso na rede ");
        Serial.print(SSID);
        Serial.println("IP obtido: ");
        Serial.print("<");
        Serial.print(WiFi.localIP());
        Serial.println(">");
        ret = true;      
    }

    return ret;
}
 
bool VerificaConexoesWiFIEMQTT(void)
{
    bool ret = false;
    
    if (reconnectWiFi())
    {
        if (!MQTT.connected())
        {
            ret = reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

//Função: envia ao Broker a temperatura atual
void EnviaTempUmidMQTT(void)
{
    int chk = DHT11.read(DHT11PIN);
    char msgtemp[10];
    char msgumid[10];
    
    Serial.print("Read sensor: ");
    switch (chk)
    {
      case DHTLIB_OK: 
        itoa(DHT11.temperature, msgtemp, 10);
        itoa(DHT11.humidity, msgumid, 10);
        break;
      case DHTLIB_ERROR_CHECKSUM: 
        strcpy(msgtemp, "Error!");
        strcpy(msgumid, "Error!");
        break;
      case DHTLIB_ERROR_TIMEOUT: 
        strcpy(msgtemp, "Error!");
        strcpy(msgumid, "Error!");
        break;
      default: 
        strcpy(msgtemp, "Error!");
        strcpy(msgumid, "Error!");
        break;
    }

    Serial.print("Temp: ");
    Serial.print(msgtemp);
    Serial.print(" C");
    Serial.print(" | Umidade: ");
    Serial.print(msgumid);
    Serial.println("%");
    
    MQTT.publish(TOPICO_TEMP, msgtemp);
    MQTT.publish(TOPICO_UMID, msgumid);

    delay(1000);
}
 
 
//programa principal
void loop() 
{
    //garante funcionamento das conexões WiFi e ao broker MQTT
    unsigned long currentMillis = millis();
    
    if ((previousMillis==0) || (currentMillis - previousMillis > interval))
    {
      previousMillis = currentMillis;
//      Serial.println(currentMillis);
      if (VerificaConexoesWiFIEMQTT())
      {
        EnviaTempUmidMQTT();
      }    
    }
    
    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
}

String getTime() {
    WiFiClient client;
    int attempt = 0;
    String theDate = "#datetime#";
  
    while (!!!client.connect("google.com.br", 80)) {
      Serial.println("connection failed, retrying...");
  
      if (attempt==10)
        break;
    }
  
    if (attempt<10)
    {
      client.print("HEAD / HTTP/1.1\r\n\r\n");
     
      while(!!!client.available()) {
         yield();
      }
    
      while(client.available()){
        if (client.read() == '\n') {    
          if (client.read() == 'D') {    
            if (client.read() == 'a') {    
              if (client.read() == 't') {    
                if (client.read() == 'e') {    
                  if (client.read() == ':') {    
                    client.read();
                    theDate = client.readStringUntil('\r');
                    client.stop();
                    break;
                  }
                }
              }
            }
          }
        }
      }
    }
  
    return theDate;
}

