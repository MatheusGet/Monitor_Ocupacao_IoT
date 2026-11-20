// Projeto: Monitoramento de ocupacao com ESP32
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

///////////////////////////////////////////////////////////////// DEFINIÇÃO DOS PINOS DO SENSOR HCSR04 ////////////////////////////////////////////////////////////////////////////////////////////////////////
// SENSOR 1
#define TRIG1 23
#define ECHO1 18

// SENSOR 2
#define TRIG2 22
#define ECHO2 19

// SENSOR 3
#define TRIG3 21
#define ECHO3 15

///////////////////////////////////////////////////////////// ESTRUTURA DE DADOS DOS SENSORES /////////////////////////////////////////////////////////////
struct Sensor
{
    int numero;
    int trig;
    int echo;

    float leituras[5];
    int indiceLeitura = 0;
    int quantidadeLeituras = 0;

    float distanciaFiltrada = 0;
    bool filtroPronto = false;

    bool ocupado = false;

    int contadorOcupado = 0;
    int contadorLivre = 0;

    bool estadoAnterior = false;
};

// INSTÂNCIAS DOS TRÊS SENSORES
Sensor sensor1;
Sensor sensor2;
Sensor sensor3;

//////////////////////////////////////////// CONFIGURAÇÃO DOS PINOS DE CADA SENSOR ////////////////////////////////////////////
void configurarSensores()
{
    sensor1.numero = 1;
    sensor1.trig = TRIG1;
    sensor1.echo = ECHO1;

    sensor2.numero = 2;
    sensor2.trig = TRIG2;
    sensor2.echo = ECHO2;

    sensor3.numero = 3;
    sensor3.trig = TRIG3;
    sensor3.echo = ECHO3;
}
////////////////////////////////////////////////////////// BLOCO DE VARIÁVEIS GLOBAIS ////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned long ultimaLeitura = 0; // Responsável por armazenar o tempo da última leitura do sensor HCSR04
unsigned long ultimaPublicacao = 0; // Responsável por armazenar o tempo da última publicação MQTT

unsigned long ultimaTentativaMQTT = 0; // Responsável por armazenar o tempo da última tentativa de conexão MQTT
const unsigned long intervaloReconexaoMQTT = 5000; // Tempo por tentativa de reconexão MQTT (5 segundos)

// Nossas funções
void lerDistancia(Sensor &sensor); // Função que realiza a leitura do sensor HCSR04 e armazena no vetor de leituras
void ordenarLeituras(float vetor[]); // Função que ordena o vetor de leituras válidas utilizando o algoritmo Bubble Sort
float calcularMediana(Sensor &sensor); // Função que calcula a mediana das últimas 5 leituras válidas do sensor HCSR04
void verificarOcupacao(Sensor &sensor); // Função que verifica se o espaço está ocupado ou livre com base na distância filtrada
void conectarWiFi(); // Função que realiza a conexão do ESP32 à rede Wi-Fi
void conectarMQTT(); // Função que realiza a conexão do ESP32 ao broker MQTT e publica o status online/offline
bool publicarMQTT(); // Função que publica os dados de ocupação no broker MQTT

bool publicacaoPendente = false; // Variável global que indica se há uma publicação pendente no broker MQTT

// CONTROLE DO ESTADO DE OCUPAÇÃO

float limiteOcupado = 30.0; // limite inferior de distância.
float limiteLivre = 35.0; // limite superior de distância.

// CONTROLE DE LEITURA DOS TRÊS SENSORES
int proximoSensor = 0;
const unsigned long intervaloLeituraSensor = 166;
///////////////////////////////////////////////////////////////////////////////////////////////////////////// CONFIGURAÇÕES WI-FI E MQTT ////////////////////////////////////////////////////////////////////////////////////////////////////////

// CONFIGURAÇÕES WI-FI
const char* ssid = WIFI_SSID;
const char* senhaWifi = WIFI_PASSWORD;

// CONFIGURAÇÕES MQTT
const char* mqttServidor = MQTT_SERVER;
const int mqttPorta = MQTT_PORT;


const char* topicoDados = "inatel/esp32/dados";
const char* topicoStatus = "inatel/esp32/status";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

//////////////////////////////////////////////////////////////// FUNÇÃO PARA LER A DISTÂNCIA DO SENSOR HCSR04 ////////////////////////////////////////////////////////////////////////////////////////////////////////
void lerDistancia(Sensor &sensor)
{
    // Disparo do sensor ultrassônico
    digitalWrite(sensor.trig, LOW);
    delayMicroseconds(2);

    digitalWrite(sensor.trig, HIGH);
    delayMicroseconds(10);

    digitalWrite(sensor.trig, LOW);

    // Medição do tempo de retorno do eco
    unsigned long duracao = pulseIn(sensor.echo, HIGH, 30000);

    if (duracao == 0)
    {
        Serial.println("Nenhum eco recebido!");
        return;
    }

    // Cálculo da distância em centímetros
    float distanciaCm = (duracao * 0.0343) / 2;

    // Armazenamento da leitura no vetor do sensor
    sensor.leituras[sensor.indiceLeitura] = distanciaCm;
    sensor.indiceLeitura++;

    if (sensor.quantidadeLeituras < 5)
    {
        sensor.quantidadeLeituras++;
    }

    sensor.filtroPronto = (sensor.quantidadeLeituras == 5);

    if (sensor.indiceLeitura >= 5)
    {
        sensor.indiceLeitura = 0;
    }

    // Distância no monitor serial
    Serial.print("Sensor ");
    Serial.print(sensor.numero);
    Serial.print(" - Distancia: ");
    Serial.print(distanciaCm);
    Serial.println(" cm");

    // Processamento da distância filtrada
    if (sensor.filtroPronto)
    {
        sensor.distanciaFiltrada = calcularMediana(sensor);

        Serial.print("Sensor ");
        Serial.print(sensor.numero);
        Serial.print(" - Distancia filtrada: ");
        Serial.print(sensor.distanciaFiltrada);
        Serial.println(" cm");

        verificarOcupacao(sensor);
    }

    // Verificação de mudança no estado de ocupação
    if (sensor.ocupado)
    {
        Serial.print("Sensor ");
        Serial.print(sensor.numero);
        Serial.println(" - Estado: OCUPADO");
    }
    else
    {
        Serial.print("Sensor ");
        Serial.print(sensor.numero);
        Serial.println(" - Estado: LIVRE");
    }
}


/////////////////////////// FUNÇÃO BUBBLESORT PARA ORDENAR O VETOR DAS LEITUAS VALIDAS ///////////////////////////////////
void ordenarLeituras(float vetor[])
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4 - i; j++)
        {
            if (vetor[j] > vetor[j + 1])
            {
                float temporaria = vetor[j];

                vetor[j] = vetor[j + 1];

                vetor[j + 1] = temporaria;
            }
        }
    }
}

/////////////////////////////////// FUNÇÃO PARA CALCULAR A MEDIANA /////////////////////////////////////////
float calcularMediana(Sensor &sensor)
{
    float copiaLeituras[5];

    for (int i = 0; i < 5; i++)
    {
        copiaLeituras[i] = sensor.leituras[i];
    }

    ordenarLeituras(copiaLeituras);

    return copiaLeituras[2];
}

///////////////////////////////////////////// FUNÇÃO PARA VERIFICAR A OCUPAÇÃO /////////////////////////////////////
void verificarOcupacao(Sensor &sensor)
{
    if (!sensor.ocupado)
    {
        // Verifica se existe uma possível ocupação
        if (sensor.distanciaFiltrada < limiteOcupado)
        {
            sensor.contadorOcupado++;
        }
        else
        {
            sensor.contadorOcupado = 0;
        }

        // Confirma a ocupação após 3 leituras consecutivas
        if (sensor.contadorOcupado >= 3)
        {
            sensor.ocupado = true;
            sensor.contadorOcupado = 0;
            sensor.contadorLivre = 0;
        }
    }
    else
    {
        // Verifica se existe uma possível liberação
        if (sensor.distanciaFiltrada > limiteLivre)
        {
            sensor.contadorLivre++;
        }
        else
        {
            sensor.contadorLivre = 0;
        }

        // Confirma a liberação após 3 leituras consecutivas
        if (sensor.contadorLivre >= 3)
        {
            sensor.ocupado = false;
            sensor.contadorLivre = 0;
            sensor.contadorOcupado = 0;
        }
    }
}


//////////////////////////////////////////////////// FUNÇÃO PARA CONECTAR AO WI-FI ///////////////////////////////////
void conectarWiFi()
{
    Serial.println("Conectando ao Wi-Fi...");

    WiFi.begin(ssid, senhaWifi);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi conectado!");

    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());
}

////////////////////////////////////////////// FUNÇÃO PARA CONECTAR AO BROKER MQTT /////////////////////////////
void conectarMQTT()
{
    Serial.println("Tentando conectar ao MQTT...");

    if (mqttClient.connect(
        "esp32-monitor-ocupacao",
        topicoStatus,
        1,
        true,
        "{\"dispositivo\":\"esp32\",\"status\":\"desconectado\"}"
    ))
    {
        Serial.println("Conectado ao broker MQTT!");

        mqttClient.publish(
            topicoStatus,
            "{\"dispositivo\":\"esp32\",\"status\":\"conectado\"}",
            true
        );

        // Sinaliza que os dados atuais precisam ser publicados
        publicacaoPendente = true;
    }
    else
    {
        Serial.print("Falha na conexao. Estado: ");
        Serial.println(mqttClient.state());
    }
}

/////////////////////////////////////////////////////////////// FUNÇÃO PARA PUBLICAR OS DADOS NO BROKER MQTT DOS TRÊS SENSORES /////////////////////////////
bool publicarMQTT()
{
    // Buffer para armazenar o JSON dos três sensores
    char mensagem[300];

    // Montagem da mensagem JSON
    int tamanho = snprintf(mensagem, sizeof(mensagem),
        "{\"dispositivo\":\"esp32\",\"projeto\":\"ocupacao\",\"sensores\":{"
        "\"sensor1\":{\"distancia_cm\":%.2f,\"ocupado\":%d},"
        "\"sensor2\":{\"distancia_cm\":%.2f,\"ocupado\":%d},"
        "\"sensor3\":{\"distancia_cm\":%.2f,\"ocupado\":%d}"
        "}}",
        sensor1.distanciaFiltrada, sensor1.ocupado ? 1 : 0,
        sensor2.distanciaFiltrada, sensor2.ocupado ? 1 : 0,
        sensor3.distanciaFiltrada, sensor3.ocupado ? 1 : 0
    );

    // Verifica se o JSON foi montado corretamente
    if (tamanho < 0 || tamanho >= sizeof(mensagem))
    {
        Serial.println("Erro ao montar mensagem JSON!");
        return false;
    }

    // Verifica a conexão MQTT
    if (mqttClient.connected())
    {
        if (mqttClient.publish(topicoDados, mensagem))
        {
            Serial.println("Dados dos tres sensores publicados com sucesso!");
            return true;
        }
        else
        {
            Serial.println("Falha ao publicar dados MQTT!");
            return false;
        }
    }

    return false;
}

void setup()
{
    Serial.begin(115200);

// Configuração dos três sensores
configurarSensores();

pinMode(sensor1.trig, OUTPUT);
pinMode(sensor1.echo, INPUT);

pinMode(sensor2.trig, OUTPUT);
pinMode(sensor2.echo, INPUT);

pinMode(sensor3.trig, OUTPUT);
pinMode(sensor3.echo, INPUT);

    // Conexão Wi-Fi
    conectarWiFi();

    // Configuração e conexão MQTT
    mqttClient.setServer(mqttServidor, mqttPorta);
    conectarMQTT();
}

void loop()
{
    // Verifica a conexão MQTT
    if (mqttClient.connected())
    {
        mqttClient.loop();

        // Verifica se existe uma publicação pendente
        if (publicacaoPendente)
        {
            if (publicarMQTT())
            {
                publicacaoPendente = false;
            }
        }

        // Publicação periódica a cada 10 segundos
        if (!publicacaoPendente && millis() - ultimaPublicacao >= 10000)
        {
            if (publicarMQTT())
        {
          ultimaPublicacao = millis();
        }
        }
    }
    else
    {
        // Tenta reconectar a cada 5 segundos
        if (millis() - ultimaTentativaMQTT >= intervaloReconexaoMQTT)
        {
            ultimaTentativaMQTT = millis();
            conectarMQTT();
        }
    }


// Leitura sequencial dos três sensores
if (millis() - ultimaLeitura >= intervaloLeituraSensor)
{
    ultimaLeitura = millis();

    switch (proximoSensor)
    {
        case 0:
            lerDistancia(sensor1);
            break;

        case 1:
            lerDistancia(sensor2);
            break;

        case 2:
            lerDistancia(sensor3);
            break;
    }

    // Avança para o próximo sensor
    proximoSensor++;

    if (proximoSensor >= 3)
    {
        proximoSensor = 0;
    }
}
}
