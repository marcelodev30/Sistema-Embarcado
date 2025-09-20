# Documentação do Servidor Backend - Sistema de Controle IR 

Este documento descreve a arquitetura, as responsabilidades, a API REST e a lógica de negócio do servidor central do Sistema de Controle IR. O servidor atua como o cérebro da operação, orquestrando a comunicação entre os usuários (via API) e os dispositivos de hardware (via MQTT).

**Pilha Tecnológica Sugerida:**
* **Linguagem/Framework:** Node.js com Express.js e Prisma(ORM)
* **Banco de Dados:** PostgreSQL 
* **Comunicação MQTT:** Biblioteca como `mqtt.js`
* **Agendamento de Tarefas:** `node-cron` ou similar

## 1. Arquitetura e Responsabilidades

O servidor é responsável por:
1.  **Expor uma API RESTful:** Fornecer endpoints seguros para que interfaces de usuário (web, mobile) possam gerenciar o sistema.
2.  **Orquestrar a Comunicação MQTT:** Atuar como um cliente MQTT, enviando comandos para os Módulos Smart e recebendo dados de telemetria.
3.  **Gerenciar a Lógica de Negócio:** Processar as requisições, tomar decisões e executar as automações agendadas.
4.  **Persistir os Dados:** Interagir com o banco de dados para salvar e recuperar todas as informações.

## 2. API Endpoints (RESTful API)

A API segue os princípios REST, utilizando métodos HTTP para ações e URLs baseadas em substantivos para recursos.

**Base URL:** `/api/v1`

---

### 🏛️ Autenticação e Usuários

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Obter Token de Acesso** | `POST` | `/auth/token` | Envia `username` e `password` para gerar um token de autenticação (login). |
| **Listar/Criar Usuários** | `GET`, `POST` | `/users` | `GET` lista todos os usuários. `POST` cria um novo usuário. |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/users/{id}` | Gerencia um usuário específico. |
| **Atualização Parcial** | `PATCH` | `/users/{id}` | Atualiza informações parciais, como o nível de acesso. |

---

### 📍 Localização (Setores e Salas)

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Listar/Criar Setores** | `GET`, `POST` | `/sectors` | Gerencia a coleção de setores (ex: "Bloco A"). |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/sectors/{id}` | Gerencia um setor específico. |
| **Listar/Criar Salas** | `GET`, `POST` | `/rooms` | Gerencia a coleção de salas (ex: "Sala 101"). |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/rooms/{id}` | Gerencia uma sala específica. |

---

### 💡 Dispositivos, Modelos e Comandos

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Listar/Criar Modelos** | `GET`, `POST` | `/device-models` | Gerencia os "tipos" de dispositivos (ex: "Ar Condicionado LG"). |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/device-models/{id}` | Gerencia um modelo específico. |
| **Listar/Criar Dispositivos**| `GET`, `POST` | `/devices` | Gerencia os aparelhos físicos instalados. |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/devices/{id}` | Gerencia um aparelho físico específico. |
| **Controlar um Dispositivo** | `PATCH` | `/devices/{id}/state` | **Rota principal de controle.** Envia um JSON para mudar o estado. |
| **Aprender Comando** | `POST`| `/devices/{id}/commands/learn` | **Inicia o processo de aprendizagem de um novo comando IR.** |
| **Listar Comandos** | `GET` | `/devices/{id}/commands` | Lista os comandos IR aprendidos para um dispositivo. |

#### **Endpoint Detalhado: `POST /devices/{id}/commands/learn`**
-   **Descrição:** Inicia o processo de aprendizagem de um novo comando IR a partir de um conjunto estruturado de propriedades. A API responde imediatamente (`202 Accepted`), e o processo continua de forma assíncrona.
-   **Corpo da Requisição (JSON):**
    ```json
    {
      "label": "Ligar Frio 22°C (Auto)",
      "properties": { "power": "on", "mode": "cool", "temperature": 22, "fan_speed": "auto" }
    }
    ```
-   **Resposta de Sucesso (`202 Accepted`):**
    ```json
    { "message": "Learning process started...", "command_uuid": "c9a2c2b1-..." }
    ```

#### **Endpoint Detalhado: `PATCH /devices/{id}/state`**
-   **Descrição:** Altera o estado desejado de um dispositivo. O servidor recebe o estado, encontra o comando IR correspondente e o envia para o módulo.
-   **Corpo da Requisição (JSON):**
    ```json
    { "power": "on", "temperature": 22 }
    ```
-   **Resposta de Sucesso (`202 Accepted`):**
    ```json
    { "message": "State change command accepted...", "log_operation_uuid": "log-..." }
    ```
---

### 🎬 Cenários (Scenes)

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Listar/Criar Cenários** | `GET`, `POST` | `/scenes` | Gerencia cenários (conjuntos de ações). |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/scenes/{id}` | Gerencia um cenário específico. |
| **Ativar um Cenário** | `POST` | `/scenes/{id}/activate` | Rota de ação para executar todas as tarefas de um cenário. |

---

## 3. Lógica de Negócio e Interação MQTT

### 3.1. Subscrições MQTT
-   **`ir/data/+/telemetry`**: Para receber resultados de operações e leituras de sensores.
-   **`ir/status/+/state`**: Para receber atualizações de status (heartbeats) dos módulos.

### 3.2. Processamento de Mensagens Recebidas
-   **`operation: command:learn:result`**: Ao receber, usa o `command_uuid` para encontrar a requisição original e salva o `code` IR na tabela `ComandoIR` junto com o `label` e as `properties`.
-   **`operation: sensor:reading:result`**: Ao receber, insere um novo registro na tabela `LogTemperatura`.
-   **`operation: module:status:update`**: Ao receber, atualiza a tabela `ModuloSmart` com o status (`Online`) e o timestamp `last_seen`.

### 3.3. Lógica de Publicação MQTT
-   **Ao receber `POST /api/v1/devices/{id}/commands/learn`**:
    1.  Valida a requisição, busca o `ModuloSmart` associado e gera um `command_uuid`.
    2.  Publica a mensagem `operation: command:learn:request` no tópico `ir/control/{module_id}/command`.

-   **Ao receber `PATCH /api/v1/devices/{id}/state`**:
    1.  Busca o "último estado conhecido" do dispositivo.
    2.  Mescla o estado antigo com as alterações recebidas para criar o "novo estado completo desejado".
    3.  Busca na tabela `ComandoIR` o comando cujas `properties` correspondem a este novo estado completo.
    4.  Cria um registro em `LogOperacaoIR`.
    5.  Publica a mensagem `operation: command:execute:request` com o código IR no tópico `ir/control/{module_id}/command`.

### 3.4. Lógica de Agendamento
1.  Um processo interno (`node-cron`) executa a cada minuto.
2.  Para cada agendamento ativo, ele dispara a mesma lógica do endpoint **`PATCH /api/v1/devices/{id}/state`**, usando as propriedades do comando agendado para definir o estado desejado.