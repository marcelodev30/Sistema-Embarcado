# Documentação do Servidor Backend - Sistema de Controle IR (v2)

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

A API segue os princípios REST, utilizando métodos HTTP para ações e URLs baseadas em substantivos para recursos.

**Base URL:** `/api/v1`

---

### 🏛️ Autenticação e Usuários

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Obter Token de Acesso** | `POST` | `/auth/token` | Envia `username` e `password` para gerar um token de autenticação (login). |
| **Listar/Criar Usuários** | `GET`, `POST` | `/usuarios` | `GET` lista todos os usuários. `POST` cria um novo usuário. |
| **Ver/Atualizar/Deletar** | `GET`, `PUT`, `DELETE`| `/usuarios/{id}` | Gerencia um usuário específico. `PUT` atualiza tudo, `DELETE` apaga. |
| **Atualização Parcial** | `PATCH` | `/usuarios/{id}` | Atualiza informações parciais, como apenas o nível de acesso. |

---

### 📍 Localização (Setores e Salas)

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Listar/Criar Setores** | `GET`, `POST` | `/setores` | `GET` lista todos os setores. `POST` cria um novo setor. |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/setores/{id}` | Gerencia um setor específico. |
| **Listar/Criar Salas** | `GET`, `POST` | `/salas` | `GET` lista todas as salas. `POST` cria uma nova sala. |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/salas/{id}` | Gerencia uma sala específica. |

---

### 💡 Dispositivos, Modelos e Comandos

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Listar/Criar Modelos** | `GET`, `POST` | `/modelos-dispositivos` | Gerencia os "tipos" de dispositivos que o sistema suporta. |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/modelos-dispositivos/{id}` | Gerencia um modelo específico. |
| **Listar/Criar Dispositivos**| `GET`, `POST` | `/dispositivos` | Gerencia os aparelhos físicos instalados. |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/dispositivos/{id}` | Gerencia um aparelho físico específico. |
| **Controlar um Dispositivo** | `PATCH` | `/dispositivos/{id}/estado` | **Rota principal de controle.** Envia um JSON para mudar o estado, como `{"ligado": true, "temperatura": 22}`. |
| **Aprender Comando** | `POST`| `/dispositivos/{id}/comandos/aprender` | **Inicia o processo de aprendizagem de um novo comando IR.** |
| **Listar Comandos** | `GET` | `/dispositivos/{id}/comandos` | Lista todos os comandos IR aprendidos para um dispositivo. |

---

### 🎬 Cenários (Scenes)

| Ação | Método HTTP | Endpoint (URL) | Descrição |
| :--- | :--- | :--- |:---|
| **Listar/Criar Cenários** | `GET`, `POST` | `/cenarios` | `GET` lista todos os cenários. `POST` cria um novo cenário. |
| **Ver/Atualizar/Deletar**| `GET`, `PUT`, `DELETE`| `/cenarios/{id}` | Gerencia um cenário específico. |
| **Ativar um Cenário** | `POST` | `/cenarios/{id}/ativar` | Rota de ação para executar todas as tarefas de um cenário. |

---

## 3. Lógica de Negócio e Interação MQTT

O servidor deve se conectar ao broker MQTT na inicialização e manter a conexão ativa.

### 3.1. Subscrições MQTT

O servidor deve se inscrever nos seguintes tópicos para receber informações de todos os módulos:

- **`ir/data/+/telemetry`**: Para receber resultados de operações e leituras de sensores de todos os módulos.
- **`ir/status/+/state`**: Para receber atualizações de status (heartbeats) de todos os módulos.

### 3.2. Processamento de Mensagens Recebidas

- **`operation: command:learn:result`**:
  1.  Recebe o resultado do processo de aprendizagem.
  2.  Usa o `command_uuid` para encontrar a requisição original.
  3.  Se o `status` for `success`, salva o `code` IR na tabela `ComandoIR`.
  4.  (Opcional) Notifica o cliente original via WebSockets que o processo foi concluído.

- **`operation: sensor:reading:result`**:
  1.  Recebe uma leitura de temperatura/umidade.
  2.  Insere um novo registro na tabela `LogTemperatura`.

- **`operation: module:status:update`**:
  1.  Recebe um heartbeat de um módulo.
  2.  Atualiza a tabela `ModuloSmart` com o status (`Online`) e o timestamp `last_seen`.

### 3.3. Lógica de Publicação MQTT

- **Ao receber `POST /api/dispositivos/{id}/comandos/aprender`**:
  1.  Valida a requisição e o `id` do dispositivo.
  2.  Busca no DB qual `ModuloSmart` está associado a este dispositivo.
  3.  Gera um `command_uuid` único.
  4.  Publica uma mensagem com `operation: command:learn:request` no tópico `ir/control/{module_id}/command`.

- **Ao receber `PATCH /api/dispositivos/{id}/estado`**:
  1.  Valida a requisição.
  2.  O servidor "traduz" o corpo da requisição (ex: `{"temperatura": 22}`) para o nome de um comando salvo no banco (ex: `'mudar_temp_22'`).
  3.  Busca no DB o `ComandoIR` correspondente.
  4.  Busca qual `ModuloSmart` controla o dispositivo.
  5.  Cria um registro em `LogOperacaoIR`.
  6.  Publica uma mensagem com `operation: command:execute:request`, contendo o código IR, no tópico `ir/control/{module_id}/command`.

### 3.4. Lógica de Agendamento

1.  Um processo interno (usando `node-cron`) é executado a cada minuto.
2.  O processo verifica a tabela `Agendamento` por tarefas que devem ser executadas.
3.  Para cada tarefa encontrada, ele dispara a mesma lógica do endpoint `PATCH /api/dispositivos/{id}/estado`.
4.  Atualiza o status do agendamento na tabela.