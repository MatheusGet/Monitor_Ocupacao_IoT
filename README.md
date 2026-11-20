# Monitor de Ocupação IoT

Sistema de monitoramento de ocupação de vagas utilizando **ESP32**, três sensores ultrassônicos **HC-SR04** e comunicação **MQTT** para transmissão dos dados em formato JSON.

## Sobre o projeto

O Monitor de Ocupação IoT é um sistema desenvolvido para identificar e acompanhar a ocupação de três vagas de forma individual.

O projeto utiliza sensores ultrassônicos para medir a distância dos objetos em relação a cada sensor. A ESP32 processa essas medições, determina o estado de ocupação de cada vaga e transmite as informações por meio do protocolo MQTT.

A comunicação é realizada com um broker Mosquitto, permitindo que os dados sejam disponibilizados para outros dispositivos e aplicações conectados à rede.

## Funcionalidades

- Monitoramento individual de três vagas.
- Leitura de distância utilizando sensores ultrassônicos HC-SR04.
- Identificação dos estados ocupado e livre.
- Processamento independente das leituras dos sensores.
- Comunicação sem fio por Wi-Fi.
- Publicação dos dados utilizando o protocolo MQTT.
- Transmissão das informações em formato JSON.
- Integração com broker MQTT Mosquitto.

## Hardware utilizado

| Componente | Quantidade | Função |
|---|---:|---|
| ESP32 | 1 | Processamento e comunicação Wi-Fi |
| HC-SR04 | 3 | Medição de distância |
| Protoboard | 1 | Montagem do circuito |
| Jumpers | Conforme necessário | Conexões elétricas |

### Mapeamento dos pinos

| Sensor | TRIG | ECHO |
|---|---:|---:|
| Sensor 1 | GPIO 23 | GPIO 18 |
| Sensor 2 | GPIO 22 | GPIO 19 |
| Sensor 3 | GPIO 21 | GPIO 15 |

**Atenção:** o pino ECHO do HC-SR04 trabalha com sinal de aproximadamente 5 V. Como os GPIOs da ESP32 trabalham com lógica de 3,3 V, utilize um divisor de tensão ou conversor de nível lógico nas conexões ECHO.

## Arquitetura do sistema

O funcionamento do sistema segue o fluxo:

1. Os três sensores ultrassônicos realizam as medições de distância.
2. A ESP32 processa individualmente os valores recebidos.
3. A lógica do firmware determina se cada vaga está ocupada ou livre.
4. Os estados são organizados em uma estrutura de dados.
5. As informações são transmitidas em JSON por meio do MQTT.
6. O broker Mosquitto recebe as mensagens para distribuição aos clientes inscritos.

## Tecnologias utilizadas

| Tecnologia | Aplicação |
|---|---|
| C++ | Desenvolvimento do firmware |
| Arduino Framework | Desenvolvimento para ESP32 |
| PlatformIO | Ambiente de desenvolvimento e compilação |
| Wi-Fi | Conectividade da ESP32 |
| MQTT | Comunicação entre dispositivos |
| Mosquitto | Broker MQTT |
| JSON | Formato de transmissão dos dados |
| Git e GitHub | Versionamento e hospedagem do código |

## Estrutura do repositório

```text
Monitor_Ocupacao_IoT/
│
├── .vscode/
│
├── include/
│   ├── README
│   └── secrets.h          # Arquivo local, não versionado
│
├── lib/
│   └── README
│
├── src/
│   └── main.cpp           # Firmware principal
│
├── test/
│   └── README
│
├── .gitignore
└── platformio.ini         # Configuração do PlatformIO
```

## Configuração do ambiente

### Pré-requisitos

- Visual Studio Code.
- Extensão PlatformIO IDE.
- Git.
- Conta no GitHub.
- Placa ESP32 compatível com o ambiente configurado no PlatformIO.
- Broker MQTT Mosquitto acessível pela rede.

### Configuração das credenciais

As credenciais de Wi-Fi e as configurações do broker MQTT são mantidas em um arquivo separado chamado `secrets.h`.

Crie o arquivo `include/secrets.h` com a seguinte estrutura:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

// Credenciais Wi-Fi
#define WIFI_SSID "SEU_WIFI"
#define WIFI_PASSWORD "SUA_SENHA"

// Configurações MQTT
#define MQTT_SERVER "IP_DO_BROKER"
#define MQTT_PORT 1883

#endif
```

O firmware utiliza essas definições para configurar a conexão com a rede e o broker MQTT.

**Importante:** o arquivo `secrets.h` não deve ser enviado ao GitHub. As credenciais devem permanecer privadas.

### Compilação e gravação

1. Abra o projeto no Visual Studio Code.
2. Verifique a configuração da placa no `platformio.ini`.
3. Confira se o arquivo `include/secrets.h` está configurado.
4. Conecte a ESP32 ao computador.
5. Utilize o comando Build do PlatformIO para compilar o firmware.
6. Utilize Upload para gravar o programa na placa.

## Comunicação MQTT

O sistema utiliza o protocolo MQTT para transmitir os dados de ocupação ao broker Mosquitto.

Configuração utilizada no desenvolvimento:

- Broker: Mosquitto.
- Porta: 1883.
- Autenticação: não exigida na configuração atual.
- Formato dos dados: JSON.
- Transporte: Wi-Fi.

Os tópicos MQTT e o formato exato do payload devem ser consultados na implementação atual do firmware.

## Versionamento

O código-fonte é gerenciado utilizando Git e hospedado no GitHub.

Repositório: [Monitor_Ocupacao_IoT](https://github.com/MatheusGet/Monitor_Ocupacao_IoT)

O projeto utiliza commits para registrar a evolução do firmware e branches para desenvolver novas funcionalidades de forma organizada.

## Objetivo acadêmico

Este projeto tem como objetivo aplicar conceitos de Internet das Coisas (IoT), sistemas embarcados, sensoriamento ultrassônico e comunicação entre dispositivos por meio de protocolos de rede.

A implementação também permite estudar integração entre hardware e software, processamento de dados e organização de projetos utilizando ferramentas profissionais de desenvolvimento.
