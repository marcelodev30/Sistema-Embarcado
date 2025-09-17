# Documentação do Servidor Backend - Sistema de Controle IR

Este documento descreve a arquitetura, as responsabilidades, a API REST e a lógica de negócio do servidor central do Sistema de Controle IR. O servidor atua como o cérebro da operação, orquestrando a comunicação entre os usuários (via API) e os dispositivos de hardware (via MQTT).

**Pilha Tecnológica Sugerida:**
* **Linguagem/Framework:** Node.js com Express.js ou NestJS
* **Banco de Dados:** PostgreSQL ou MySQL/MariaDB
* **Comunicação MQTT:** Biblioteca como `mqtt.js`
* **Agendamento de Tarefas:** `node-cron` ou similar

## 1. Arquitetura e Responsabilidades

O servidor é responsável por:
1.  **Expor uma API RESTful:** Fornecer endpoints seguros para que interfaces de usuário (web, mobile) possam gerenciar dispositivos, comandos e agendamentos.
2.  **Orquestrar a Comunicação MQTT:** Atuar como um cliente MQTT que se comunica com os Módulos Smart, enviando comandos e recebendo dados de telemetria.
3.  **Gerenciar a Lógica de Negócio:** Processar as requisições, tomar decisões e executar as automações agendadas.
4.  **Persistir os Dados:** Interagir com o banco de dados para salvar e recuperar todas as informações do sistema (dispositivos, logs, etc.).

## 2. API Endpoints (RESTful API)

A seguir, a especificação dos principais endpoints da API.

**Base URL:** `/api/v1`

---

### 2.1. Gerenciamento de Dispositivos (`/devices`)

| Método | Endpoint | Descrição |
| :--- | :--- | :--- |
| `GET` | `/devices` | Lista todos os dispositivos cadastrados. |
| `POST` | `/devices` | Cadastra um novo dispositivo físico. |
| `GET` | `/devices/{deviceId}` | Obtém detalhes de um dispositivo específico. |
| `PUT` | `/devices/{deviceId}` | Atualiza as informações de um dispositivo. |
| `DELETE` | `/devices/{deviceId}` | Remove um dispositivo. |

---

### 2.2. Gerenciamento de Comandos (`/devices/{deviceId}/commands`)

| Método | Endpoint | Descrição |
| :--- | :--- | :--- |
| `GET` | `/devices/{deviceId}/commands` | Lista todos os comandos IR aprendidos para um dispositivo. |
| `POST`| `/devices/{deviceId}/commands/learn` | **Inicia o processo de aprendizagem de um novo comando IR.** |
| `POST`| `/devices/{deviceId}/commands/execute`| **Executa um comando IR em um dispositivo.** |

#### **Endpoint Detalhado: `POST /devices/{deviceId}/commands/learn`**

- **Descrição:** Coloca o Módulo Smart associado ao dispositivo em modo de aprendizagem. A API responde imediatamente, mas o processo continua de forma assíncrona via MQTT.
- **Corpo da Requisição (JSON):**
  ```json
  {
    "name": "Ligar Ar 22C Ventilacao Max"
  }
  ```
- **Resposta de Sucesso (`202 Accepted`):**
  ```json
  {
    "message": "Learning process started. Point the remote at the module.",
    "command_uuid": "c9a2c2b1-5a3a-4f9d-8f0a-1b9b3b7c5e2d"
  }
  ```

#### **Endpoint Detalhado: `POST /devices/{deviceId}/commands/execute`**

- **Descrição:** Busca um comando IR no banco de dados e o envia para o Módulo Smart correspondente para execução.
- **Corpo da Requisição (JSON):**
  ```json
  {
    "name": "Ligar Ar 22C Ventilacao Max"
  }
  ```
- **Resposta de Sucesso (`200 OK`):**
  ```json
  {
    "message": "Command sent successfully.",
    "log_operation_uuid": "log-xyz-789"
  }
  ```

---

### 2.3. Gerenciamento de Módulos (`/modules`)

| Método | Endpoint | Descrição |
| :--- | :--- | :--- |
| `GET` | `/modules` | Lista todos os Módulos Smart e seu último estado conhecido (Online/Offline). |
| `GET` | `/modules/{moduleId}` | Obtém detalhes de um módulo específico, incluindo logs recentes. |

---

### 2.4. Gerenciamento de Agendamentos (`/schedules`)

| Método | Endpoint | Descrição |
| :--- | :--- | :--- |
| `GET` | `/schedules/device/{deviceId}`| Lista todos os agendamentos para um dispositivo. |
| `POST` | `/schedules` | Cria um novo agendamento. |
| `DELETE` | `/schedules/{scheduleId}` | Remove um agendamento. |

---

## 3. Lógica de Negócio e Interação MQTT

O servidor deve se conectar ao broker MQTT na inicialização e manter a conexão ativa.

### 3.1. Subscrições MQTT

O servidor deve se inscrever nos seguintes tópicos para receber informações de todos os módulos:

- **`ir/data/+/telemetry`**: Para receber resultados de operações e leituras de sensores de todos os módulos. O `+` é um wildcard de nível único.
- **`ir/status/+/state`**: Para receber atualizações de status (heartbeats) de todos os módulos.

### 3.2. Processamento de Mensagens Recebidas

O servidor deve ter um roteador de mensagens que analisa a `operation` de cada payload recebido e direciona para a lógica apropriada.

- **`operation: command:learn:result`**:
  1.  Recebe o resultado do processo de aprendizagem.
  2.  Usa o `command_uuid` para encontrar a requisição original.
  3.  Se o `status` for `success`, salva o `code` IR na tabela `ComandoIR`.
  4.  (Opcional) Notifica o cliente original via WebSockets que o processo foi concluído.

- **`operation: sensor:reading:result`**:
  1.  Recebe uma leitura de temperatura/umidade.
  2.  Valida os dados.
  3.  Insere um novo registro na tabela `LogTemperatura`.

- **`operation: module:status:update`**:
  1.  Recebe um heartbeat ou uma mensagem de status.
  2.  Atualiza a tabela `ModuloSmart` com o status (`Online`), o endereço IP e a versão do firmware.
  3.  Atualiza um timestamp `last_seen` para saber quando foi a última vez que o módulo se comunicou.

### 3.3. Lógica de Publicação MQTT

- **Ao receber `POST .../commands/learn`**:
  1.  Valida a requisição e o `deviceId`.
  2.  Busca no DB qual `ModuloSmart` está associado a este `deviceId`.
  3.  Gera um `command_uuid` único.
  4.  Publica uma mensagem com `operation: command:learn:request` no tópico `ir/control/{module_id}/command`.

- **Ao receber `POST .../commands/execute`**:
  1.  Valida a requisição.
  2.  Busca no DB o `ComandoIR` pelo `name` e `deviceId`.
  3.  Busca qual `ModuloSmart` controla o `deviceId`.
  4.  Cria um registro em `LogOperacaoIR`.
  5.  Publica uma mensagem com `operation: command:execute:request`, contendo o código IR, no tópico `ir/control/{module_id}/command`.

### 3.4. Lógica de Agendamento

1.  Um processo interno (usando `node-cron`) é executado a cada minuto.
2.  O processo verifica a tabela `Agendamento` por tarefas que devem ser executadas naquele minuto.
3.  Para cada tarefa encontrada, ele dispara a mesma lógica do endpoint `POST .../commands/execute`.
4.  Atualiza o status do agendamento na tabela.