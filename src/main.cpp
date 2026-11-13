#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
////////////////////////////////////////////////////////// BLOCO DE VARIÁVEIS GLOBAIS ////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned long ultimaLeitura = 0; // Responsável por armazenar o tempo da última leitura do sensor HCSR04
unsigned long ultimaPublicacao = 0; // Responsável por armazenar o tempo da última publicação MQTT

unsigned long ultimaTentativaMQTT = 0; // Responsável por armazenar o tempo da última tentativa de conexão MQTT
const unsigned long intervaloReconexaoMQTT = 5000; // Tempo por tentativa de reconexão MQTT (5 segundos)

float leituras[5]; // Vetor que armazena as últimas 5 leituras válidas do sensor HCSR04
int indiceLeitura = 0; // Índice que indica a posição atual no vetor de leituras
int quantidadeLeituras = 0; // Contador que indica quantas leituras válidas foram armazenadas no vetor (máximo de 5)

// Nossas funções
void lerDistancia(); // Função que realiza a leitura do sensor HCSR04 e armazena no vetor de leituras
void ordenarLeituras(float vetor[]); // Função que ordena o vetor de leituras válidas utilizando o algoritmo Bubble Sort
float calcularMediana(); // Função que calcula a mediana das últimas 5 leituras válidas do sensor HCSR04
void verificarOcupacao(); // Função que verifica se o espaço está ocupado ou livre com base na distância filtrada
void conectarWiFi(); // Função que realiza a conexão do ESP32 à rede Wi-Fi
void conectarMQTT(); // Função que realiza a conexão do ESP32 ao broker MQTT e publica o status online/offline
bool publicarMQTT(); // Função que publica os dados de ocupação no broker MQTT

float distanciaFiltrada = 0; // Variável global que armazena a distância filtrada (mediana) das últimas 5 leituras válidas do sensor HCSR04
bool filtroPronto = false; // Variável global que indica se o filtro de mediana está pronto (ou seja, se já foram feitas 5 leituras válidas)
bool publicacaoPendente = false; // Variável global que indica se há uma publicação pendente no broker MQTT

// CONTROLE DO ESTADO DE OCUPAÇÃO
bool ocupado = false; // estado atual do espaço.

int contadorOcupado = 0; // confirmações consecutivas de ocupação.
int contadorLivre = 0; // confirmações consecutivas de liberação.
float limiteOcupado = 30.0; // limite inferior de distância.
float limiteLivre = 35.0; // limite superior de distância.
bool estadoAnterior = false;

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

///////////////////////////////////////////////////////////////// DEFINIÇÃO DOS PINOS DO SENSOR HCSR04 ////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TRIG 23 
#define ECHO 18

////////////////////////////////////////////////////////// FUNÇÕES DE LEITURA DO SENSOR /////////////////////////////////////////
void lerDistancia()
{
digitalWrite(TRIG, LOW);
delayMicroseconds(2);

digitalWrite(TRIG, HIGH);
delayMicroseconds(10);

digitalWrite(TRIG, LOW);

unsigned long duracao = pulseIn(ECHO, HIGH, 30000); // Emissão do pulso e medição do tempo de retorno do eco (em microsegundos)

if (duracao == 0)
{
    Serial.println("Nenhum eco recebido!");
    return;
}

float distanciaCm = (duracao * 0.0343) / 2; // Calcula a distância em centímetros (velocidade do som = 343 m/s)

// Armazenamento da leitura no vetor
leituras[indiceLeitura] = distanciaCm;
indiceLeitura++;

if (quantidadeLeituras < 5)
{
    quantidadeLeituras++;
}

filtroPronto = (quantidadeLeituras == 5);

if (indiceLeitura >= 5)
{
    indiceLeitura = 0;
}

// Distancia no monitor serial individual
Serial.print("Distancia: ");
Serial.print(distanciaCm);
Serial.println(" cm");

// Distancia filtrada no monitor serial
if (filtroPronto)
{
    distanciaFiltrada = calcularMediana();

    Serial.print("Distancia filtrada: ");
    Serial.print(distanciaFiltrada);
    Serial.println(" cm");

    verificarOcupacao();

if (ocupado != estadoAnterior)
{
    if (ocupado)
    {
        Serial.println("Estado: OCUPADO");
    }
    else
    {
        Serial.println("Estado: LIVRE");
    }

    publicacaoPendente = true;

    estadoAnterior = ocupado;
}
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
float calcularMediana()
{
    float copiaLeituras[5];

    for (int i = 0; i < 5; i++)
    {
        copiaLeituras[i] = leituras[i];
    }

    ordenarLeituras(copiaLeituras);

    return copiaLeituras[2];
}

///////////////////////////////////////////// FUNÇÃO PARA VERIFICAR A OCUPAÇÃO /////////////////////////////////////
void verificarOcupacao()
{
    if (!ocupado)
    {
        // Verifica se existe uma possível ocupação
        if (distanciaFiltrada < limiteOcupado)
        {
            contadorOcupado++;
        }
        else
        {
            contadorOcupado = 0;
        }

        // Confirma a ocupação após 3 leituras consecutivas
        if (contadorOcupado >= 3)
        {
            ocupado = true;
            contadorOcupado = 0;
            contadorLivre = 0;
        }
    }
    else
    {
        // Verifica se existe uma possível liberação
        if (distanciaFiltrada > limiteLivre)
        {
            contadorLivre++;
        }
        else
        {
            contadorLivre = 0;
        }

        // Confirma a liberação após 3 leituras consecutivas
        if (contadorLivre >= 3)
        {
            ocupado = false;
            contadorLivre = 0;
            contadorOcupado = 0;
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

//////////////////////////////////// FUNÇÃO PARA PUBLICAR DADOS VIA MQTT ////////////////////////////////
bool publicarMQTT()
{
    char mensagem[150];

    snprintf(mensagem, sizeof(mensagem),
        "{\"dispositivo\":\"esp32\",\"projeto\":\"ocupacao\",\"leituras\":{\"distancia_cm\":%.2f,\"ocupado\":%d}}",
        distanciaFiltrada,
        ocupado ? 1 : 0);

    if (mqttClient.connected())
    {
        if (mqttClient.publish(topicoDados, mensagem))
        {
            Serial.println("Dados publicados com sucesso!");
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

    // Configuração dos pinos
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

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

    // Leitura do sensor a cada 500 ms
    if (millis() - ultimaLeitura >= 500)
    {
        ultimaLeitura = millis();
        lerDistancia();
    }
}
