# Documentação de Payloads MQTT - Sistema de Controle IR

Este documento especifica a estrutura dos tópicos e dos payloads JSON utilizados para a comunicação via MQTT entre o servidor central (Broker/Backend) e os Módulos Smart (Pico W / ESP32).

## Estrutura Padrão dos Tópicos

Os tópicos MQTT seguem um padrão hierárquico para facilitar a organização e a depuração. A variável `{module_id}` deve ser substituída pelo ID único do Módulo Smart (ex: `pico-sala-101`).

- **`ir/control/{module_id}/command`**: Tópico para o **Servidor enviar comandos** para um módulo específico.
- **`ir/data/{module_id}/telemetry`**: Tópico para um **Módulo enviar dados** (resultados de operações, leituras de sensores) para o servidor.
- **`ir/status/{module_id}/state`**: Tópico para um **Módulo enviar informações de estado** (heartbeat, status online/offline) para o servidor.

## Estrutura Padrão do Payload

Todos os payloads JSON seguirão uma estrutura básica para manter a consistência e facilitar o processamento das mensagens.

```json
{
  "operation": "namespace:action:subject",
  "metadata": {
    "timestamp": "2025-09-17T03:30:00Z",
    "source_module_id": "{module_id}"
  },
  "payload": {
    //... dados específicos da operação
  }
}
```
- **`operation`**: String obrigatória que define a intenção da mensagem. Usa um formato `namespace:action:subject` para evitar conflitos.
- **`metadata`**: Objeto que contém informações contextuais sobre a mensagem.
  - `timestamp`: Padrão ISO 8601 (UTC) de quando a mensagem foi gerada.
  - `source_module_id`: ID do módulo que originou a mensagem (útil para o servidor).
- **`payload`**: Objeto que contém os dados específicos da operação.

---

## Payloads Detalhados por Fluxo

### Fluxo 1: Aprendizagem de Comando IR

Utilizado para ensinar um novo código de controle remoto ao sistema.

#### 1.1 Requisição de Aprendizagem (Servidor → Módulo Leitor)

- **Tópico**: `ir/control/{module_id}/command`
- **`operation`**: `command:learn:request`
- **Descrição**: O servidor comanda um módulo leitor para iniciar o processo de captura de um sinal IR.

```json
{
  "operation": "command:learn:request",
  "metadata": {
    "timestamp": "2025-09-17T10:00:00Z",
    "source_module_id": "server-backend"
  },
  "payload": {
    "command_uuid": "c9a2c2b1-5a3a-4f9d-8f0a-1b9b3b7c5e2d",
    "command_name": "Ligar Ar 22°C"
  }
}
```

#### 1.2 Resultado da Aprendizagem (Módulo Leitor → Servidor)

- **Tópico**: `ir/data/{module_id}/telemetry`
- **`operation`**: `command:learn:result`
- **Descrição**: O módulo leitor envia de volta o resultado da tentativa de captura.

**Exemplo de Sucesso:**
```json
{
  "operation": "command:learn:result",
  "metadata": {
    "timestamp": "2025-09-17T10:01:15Z",
    "source_module_id": "pico-leitor-01"
  },
  "payload": {
    "command_uuid": "c9a2c2b1-5a3a-4f9d-8f0a-1b9b3b7c5e2d",
    "status": "success",
    "data": {
      "format": "raw_us",
      "code": [9000, 4500, 560, 1690, 560, 560]
    }
  }
}
```

**Exemplo de Falha (Timeout):**
```json
{
  "operation": "command:learn:result",
  "metadata": {
    "timestamp": "2025-09-17T10:02:00Z",
    "source_module_id": "pico-leitor-01"
  },
  "payload": {
    "command_uuid": "c9a2c2b1-5a3a-4f9d-8f0a-1b9b3b7c5e2d",
    "status": "fail",
    "error_message": "Timeout: No IR signal received."
  }
}
```

---

### Fluxo 2: Execução de Comando IR

Utilizado para enviar um comando IR para um aparelho.

#### 2.1 Requisição de Execução (Servidor → Módulo Atuador)

- **Tópico**: `ir/control/{module_id}/command`
- **`operation`**: `command:execute:request`
- **Descrição**: O servidor ordena que um módulo atuador transmita um código IR.

```json
{
  "operation": "command:execute:request",
  "metadata": {
    "timestamp": "2025-09-17T08:00:00Z",
    "source_module_id": "server-backend"
  },
  "payload": {
    "log_operation_uuid": "log-xyz-789",
    "device_uuid": "a4b1e8d2-6f3c-4a1b-9d8c-2b9c7b5a1e3d",
    "data": {
      "format": "raw_us",
      "code": [9000, 4500, 560, 1690, 560, 560]
    }
  }
}
```

#### 2.2 Resultado da Execução (Módulo Atuador → Servidor) - *Opcional, mas recomendado*

- **Tópico**: `ir/data/{module_id}/telemetry`
- **`operation`**: `command:execute:result`
- **Descrição**: O módulo informa ao servidor se conseguiu transmitir o comando.

```json
{
  "operation": "command:execute:result",
  "metadata": {
    "timestamp": "2025-09-17T08:00:01Z",
    "source_module_id": "pico-sala-101"
  },
  "payload": {
    "log_operation_uuid": "log-xyz-789",
    "status": "success"
  }
}
```

---

### Fluxo 3: Envio de Telemetria de Sensores

Utilizado para o envio periódico de dados de sensores.

- **Tópico**: `ir/data/{module_id}/telemetry`
- **`operation`**: `sensor:reading:result`
- **Descrição**: Módulo envia dados de temperatura e umidade para o servidor.

```json
{
  "operation": "sensor:reading:result",
  "metadata": {
    "timestamp": "2025-09-17T11:05:00Z",
    "source_module_id": "pico-sala-101"
  },
  "payload": {
    "device_uuid": "a4b1e8d2-6f3c-4a1b-9d8c-2b9c7b5a1e3d",
    "temperature": 23.5,
    "humidity": 55.2
  }
}
```

---

### Fluxo 4: Gerenciamento de Módulos

Utilizado para monitorar a saúde e o estado dos Módulos Smart.

#### 4.1 Heartbeat / Status (Módulo → Servidor)

- **Tópico**: `ir/status/{module_id}/state`
- **`operation`**: `module:status:update`
- **Descrição**: Mensagem periódica (heartbeat) ou enviada em eventos específicos (boot) para informar ao servidor que o módulo está vivo e qual seu estado. Esta mensagem também é ideal para ser usada como **Last Will and Testament (LWT)** com `{"payload": {"status": "offline"}}`.

```json
{
  "operation": "module:status:update",
  "metadata": {
    "timestamp": "2025-09-17T11:10:00Z",
    "source_module_id": "pico-sala-101"
  },
  "payload": {
    "status": "online",
    "ip_address": "192.168.1.105",
    "firmware_version": "v1.2.0",
    "uptime_seconds": 3600
  }
}
```