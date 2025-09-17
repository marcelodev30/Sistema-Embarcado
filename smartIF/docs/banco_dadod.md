# Documentação do Banco de Dados: Sistema de Controle IR

Este documento detalha a estrutura do banco de dados relacional. O esquema foi otimizado para garantir a integridade dos dados, evitar redundância e fornecer uma base robusta para o registro de operações, leituras de sensores e agendamentos.

## Estrutura das Tabelas

A seguir, a descrição detalhada de cada tabela, seus campos e relacionamentos.

---

### Tabela: `ModeloDispositivo`

Armazena os "tipos" ou modelos de aparelhos que podem ser controlados (ex: Ar Condicionado LG Dual Inverter).

| Coluna | Tipo de Dado Sugerido | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `INT` / `UUID` | **PK** (Primary Key) | Identificador único para o modelo do dispositivo. |
| `nome` | `VARCHAR(100)` | NOT NULL | Nome descritivo do modelo (ex: "Dual Inverter Voice"). |
| `marca` | `VARCHAR(50)` | | Marca do aparelho (ex: "LG"). |
| `modelo` | `VARCHAR(50)` | | Código do modelo (ex: "S4-Q12JA3WF"). |
| `temp_min` | `DECIMAL(4,2)` | | Temperatura mínima de operação suportada. |
| `temp_max` | `DECIMAL(4,2)` | | Temperatura máxima de operação suportada. |

---

### Tabela: `ComandoIR`

Armazena os códigos infravermelhos específicos para cada modelo de dispositivo.

| Coluna | Tipo de Dado Sugerido | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `INT` / `UUID` | **PK** | Identificador único para o comando. |
| `id_modelo_dispositivo` | `INT` / `UUID` | **FK** -> `ModeloDispositivo(id)` | Associa o comando ao modelo de dispositivo correto. |
| `nome_comando` | `VARCHAR(100)` | NOT NULL | Nome funcional do comando (ex: "Ligar 22°C", "Ventilação Média"). |
| `codigo_ir` | `JSON` / `TEXT` | NOT NULL | O código IR bruto, armazenado como um array de inteiros (ex: `[9000, 4500, ...]`). |
| `formato_codigo` | `VARCHAR(20)` | | Formato do código (ex: "raw_us", "pronto_hex"). |

---

### Tabela: `ModuloSmart`

Representa os microcontroladores (Pico W, ESP32) que atuam como leitores ou transmissores na rede.

| Coluna | Tipo de Dado Sugerido | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `INT` / `UUID` | **PK** | Identificador único para o módulo físico. |
| `nome_modulo` | `VARCHAR(100)` | UNIQUE | Nome único para identificar o módulo (ex: "pico-sala-101"). |
| `status_modulo` | `VARCHAR(20)` | | Status atual do módulo (ex: "Online", "Offline", "Reiniciando"). |

---

### Tabela: `Dispositivo`

Representa cada instância física de um aparelho instalado em um local. **Importante:** Esta tabela armazena apenas dados de configuração, não dados de estado transiente (como status ou temperatura atual).

| Coluna | Tipo de Dado Sugerido | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `INT` / `UUID` | **PK** | Identificador único para a instância do dispositivo. |
| `id_modelo_dispositivo` | `INT` / `UUID` | **FK** -> `ModeloDispositivo(id)` | Define qual o modelo deste dispositivo. |
| `id_modulo_smart` | `INT` / `UUID` | **FK** -> `ModuloSmart(id)` | Define qual módulo é responsável por controlar este dispositivo. |
| `nome_dispositivo` | `VARCHAR(100)` | | Nome amigável dado à instância (ex: "Ar da Sala de Reuniões"). |
| `localizacao` | `VARCHAR(255)` | | Descrição do local físico do aparelho. |

---

### Tabela: `LogOperacaoIR`

Tabela de log (auditoria) que registra todas as operações de envio ou aprendizagem de comandos IR. É fundamental para depuração e histórico.

| Coluna | Tipo de Dado Sugerido | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `BIGINT` | **PK** | Identificador único do registro de log. |
| `id_modulo_smart` | `INT` / `UUID` | **FK** -> `ModuloSmart(id)` | Qual módulo realizou a operação. |
| `id_dispositivo` | `INT` / `UUID` | **FK** -> `Dispositivo(id)` | Qual dispositivo foi o alvo da operação. |
| `id_comando_ir` | `INT` / `UUID` | **FK** -> `ComandoIR(id)` | Qual comando foi utilizado. |
| `tipo_operacao` | `VARCHAR(30)` | NOT NULL | Tipo do evento (ex: `EXECUTE_SUCCESS`, `LEARN_FAIL`). |
| `data_hora` | `DATETIME` / `TIMESTAMP`| NOT NULL | Data e hora exatas em que o evento ocorreu. |

---

### Tabela: `LogTemperatura`

Tabela de série temporal para armazenar as leituras de temperatura e umidade enviadas pelos módulos.

| Coluna | Tipo de Dado Sugerido | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `BIGINT` | **PK** | Identificador único do registro de log. |
| `id_modulo_smart` | `INT` / `UUID` | **FK** -> `ModuloSmart(id)` | Qual módulo fez a leitura. |
| `id_dispositivo` | `INT` / `UUID` | **FK** -> `Dispositivo(id)` | A qual dispositivo esta leitura se refere. |
| `temperatura` | `DECIMAL(5,2)` | | A temperatura registrada em Graus Celsius. |
| `umidade` | `DECIMAL(5,2)` | | A umidade relativa do ar (%). |
| `data_hora` | `DATETIME` / `TIMESTAMP`| NOT NULL | Data e hora exatas da medição. |

---

### Tabela: `Agendamento`

Armazena as tarefas agendadas para serem executadas nos dispositivos (lógica de automação).

| Coluna | Tipo de Dado Sugerido | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `INT` / `UUID` | **PK** | Identificador único para a tarefa agendada. |
| `id_dispositivo` | `INT` / `UUID` | **FK** -> `Dispositivo(id)` | Qual dispositivo executará a tarefa. |
| `id_comando_ir` | `INT` / `UUID` | **FK** -> `ComandoIR(id)` | Qual comando será executado. |
| `horario_execucao` | `VARCHAR(255)` | NOT NULL | Padrão cron para execução (ex: "0 8 * * *"). |
| `tipo_repeticao` | `VARCHAR(20)` | | Descrição amigável (ex: "Diariamente", "Apenas uma vez"). |
| `status_agendamento`| `VARCHAR(20)` | | Status da tarefa (ex: "Ativo", "Inativo", "Concluído"). |