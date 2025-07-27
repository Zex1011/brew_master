# brew_master
Projeto de sistemas embarcados

# Descrição do projeto:

O projeto consiste em um sistema embarcado para controle de temperatura para a confecção de cerveja. O sistema embarcado deverá através do uso de sensores de temperatura, sistema de aquecimento e motor para mistura, manter determinadas temperaturas por intervalos definidos e manter a homogeneidade térmica da cerveja durante este processo.

# Tecnologias Utilizadas:

* Hardware Principal: ESP32
* Software: 
    * Criação de Statechart: itemis CREATE
    * Sistema operacional para controle de tasks: FreeRTOS
    * Compilador: Arduino IDE
* Linguagens:
    * Aplicação: C++
    * Interface gráfica: Python
* Periféricos:
    * Inputs: 2 sensores de temperatura (I2C)
    * Outputs: Aquecimento (PWM), Mixer (Gpio)


# Requisitos definidos para o projeto

## Requisitos Funcionais (RF)

| Código    | Descrição                                                                                                                                              |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **RF‑01** | Medir a temperatura da cerveha em tempo real.                                                                                                          |
| **RF‑02** | Controlar o misturador (ligar/desligar) durante o processo.                                                                                            |
| **RF‑03** | Controlar o aquecedor mantendo a temperatura‑alvo.                                                                                                     |
| **RF‑04** | Permitir a execução da curva padrão.                                                                                                                   |
| **RF‑05** | Permitir ao usuário criar e editar curvas de temperatura personalizadas.                                                                               |
| **RF‑06** | Comunicar‑se com um computador (PC ↔ ESP) para configuração e monitoramento.                                                                           |
| **RF‑07** | Detectar falhas (sensores fora de faixa, sobre‑temperatura, falha de agitador, etc.) e exibir avisos ao operador.                                      |
| **RF‑08** | Oferecer controle de temperatura On/Off com histerese.                                                                                                 |
| **RF‑09** | Acionar o misturador quando a diferença entre sensores de temperatura exceder 1 °C e mantê‑lo ligado por um período após a estabilização.              |
| **RF‑10** | Exibir ao usuário no fim do processo a quantidade de tempo em que aquecimento ou mistura foi necessário.                                               |
| **RF‑11** | Permitir um grande número de curvas configuradas, com adição permitira durante a configuração                                                          |


## Requisitos Não Funcionais (RNF)

| Código     | Categoria                    | Descrição                                                                                                            |
| ---------- | ---------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| **RNF‑01** | Restrições de Hardware       | O sensor de temperatura deve usar interface **I²C**.                                                                 |
| **RNF‑02** | Restrições de Hardware       | A saída do misturador deve ser comandada por **GPIO** dedicado.                                                      |
| **RNF‑03** | Restrições de Hardware       | O aquecimento será ser controlado por GPIO com histerese.                                                            |
| **RNF‑04** | Qualidade – Confiabilidade   | O sistema deve manter a temperatura dentro de ±1 °C da referência durante todo o processo.                           |
| **RNF‑05** | Qualidade – Usabilidade      | Alertas e estados do processo devem ser apresentados ao operador em linguagem clara (Português) via interface PC.    |
| **RNF‑06** | Qualidade – Manutenibilidade | O código‑fonte deve incluir documentação interna clara; uso de **Doxygen** é desejável.                              |
| **RNF‑07** | Documentação Externa         | Entregar documentação contendo descrição de hardware e *statechart* dos modos de operação.                           |
| **RNF‑08** | Portabilidade                | O firmware deve ser compatível com a família **ESP32**.                                                              |
| **RNF‑09** | Segurança                    | O sistema deve desligar o aquecimento automaticamente se a temperatura ultrapassar o limite “Max” definido na curva. |

---


## Critérios de Aceitação

### Requisitos Funcionais

| Código    | Critérios de Aceitação                                                                                                                                                                                                                      |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **RF‑01** | • O sistema atualiza a leitura de temperatura pelo menos a cada 1 s.<br>• A leitura difere no máximo ±0,5 °C de um termômetro de referência calibrado.                                                                                      |
| **RF‑02** | • O operador consegue ligar e desligar o misturador manualmente via interface PC.<br>• O GPIO correspondente muda de estado em ≤2 s após o comando.<br>• O misturador liga automaticamente nos pontos definidos pela lógica de processo. |
| **RF‑03** | • Com o set‑point definido, a temperatura permanece dentro de ±1 °C por pelo menos 10 min.<br>• O aquecedor desliga se a temperatura ultrapassar o set‑point +1 °C.                                                                         |
| **RF‑04** | • Selecionar “Curva Padrão” inicia automaticamente as etapas default configuradas.<br>• Cada etapa é mantida dentro de ±1 °C pelo seu tempo configurado (t₀, t₁, t₂, etc).                                                                        |
| **RF‑05** | • O usuário consegue criar uma curva nova.<br>• A curva permanece após o processo e poderá ser novamente editada.                                       |
| **RF‑06** | • O PC exibe temperatura, status do aquecedor e estado do misturador em tempo real com latência ≤2 s.                                                    |
| **RF‑07** | • Ao retirar o sensor de temperatura, um aviso é mostrado em ≤2 s.<br>• Se a temperatura ultrapassar “Max” +2 °C, o aquecedor desliga e  avisa o usuário.                               |
| **RF‑08** | • O aquecedor liga quando T≤set‑1 °C e desliga quando T≥set+1 °C.                                                             |
| **RF‑09** | • Se ΔT entre sensores >1 °C por >5 s, o misturador liga.<br>• Após ΔT ≤1 °C, o misturador permanece ligado por um tempo de pós‑mistura (exemplo 20 s).                                                                          |

### Requisitos Não Funcionais

| Código     | Critérios de Aceitação                                                                                                                                                   |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **RNF‑01** | • O sensor I²C conecta‑se aos pinos dedicados SDA/SCL e responde ao comando *WHO\_AM\_I*.<br>• A comunicação opera a 100 kHz ou 400 kHz sem erros por 10 min.      |
| **RNF‑02** | • O GPIO do misturador é dedicado e não conflita com outras funções; mede‑se nível lógico correto (3,3 V/0 V).                                                           |
| **RNF‑03** | • O aquecedor é acionado pelo GPIO configurado com uma histerese de exemplo.<br>• Frequência de comutação ≤1 Hz.                                                                                          |
| **RNF‑05** | • Todos os textos da interface estão em visiveís e em Português.  |
| **RNF‑06** | • Executar “doxygen” gera HTML sem erros.                                                             |
| **RNF‑07** | • Documentação final entregue em PDF inclui: explica do processo, statechart com estados e transições.                                                        |
| **RNF‑08** | • Firmware compila no ESP32‑WROOM.                                                    |
| **RNF‑09** | • Quando T > “Max” +2 °C, o aquecedor é desligado em ≤1 s.<br>• O sistema entra em estado de falha que requer intervenção do operador para reiniciar.                    |

---



## Estrutura de Tasks do Projeto

O projeto utiliza **FreeRTOS** com múltiplas *tasks* concorrentes, cada uma responsável por uma parte específica do controle, aquisição ou interação do sistema embarcado. A seguir, é feita uma explicação prévia de cada uma:

---

### `TimerTask`

Responsável por manter o controle de tempo interno usado pelo *statechart*. A cada 1 segundo, decrementa um contador (`cb.secLeft`) e, ao chegar a zero, sinaliza o evento `timer_trigger` para a máquina de estados.

---

### `PidTask`

Implementa o **controle PID** da temperatura baseado na leitura de um dos sensores (Sensor 1).
Funções principais:

* Lê o *setpoint* definido pelo usuário (`cb.setPoint`);
* Lê a temperatura atual (`g_sensor1`);
* Executa o cálculo PID com base nesses valores;
* Atualiza a saída PWM (`ledcWrite`) para ajustar o atuador conforme o erro.

Além disso, a task configura o controlador PID e o PWM na inicialização.

---

### `I2CTask`

Realiza a leitura periódica (1 Hz) dos sensores de temperatura via barramento **I²C**.
A cada execução:

* Tenta ler valores dos sensores em `I2C_ADDR_SENSOR1` e `I2C_ADDR_SENSOR2`;
* Se a leitura for bem-sucedida, atualiza as variáveis globais `g_sensor1` e `g_sensor2`.

---

### `TempTask`

Task de supervisão térmica, que:

* Verifica se a temperatura está fora da faixa em relação ao setpoint;
* Verifica discrepância entre os dois sensores (ex: se a diferença for maior que 1 grau);
* Aciona eventos na máquina de estados (`raiseTemp_wrong`, `raiseTemp_right`, `raiseMixer_on`, `raiseMixer_off`);
* Gera *logs* no formato `DATA-setP-s1-s2-diffFlag` para possível análise posterior.

---

### `UartTask`

Monitora a interface **UART** (serial) para comandos externos.
Permite interações como:

* Definir novo setpoint com valores inteiros;
* Comandos de controle como `start`, `default`, `reset`, `new`, etc.;
* Inserção direta de valores simulados de temperatura (`TEMPONExxx` e `TEMPTWOxxx`).

Interpreta a entrada caractere por caractere e processa ao detectar final de linha (`\n` ou `\r`).

---

### `app_tasks_init()`

Função que realiza a **inicialização das tarefas** e componentes do sistema:

* Inicializa mutex de proteção de máquina de estados;
* Configura pinos e UART via `CallbackModule`;
* Inicializa o barramento I²C;
* Cria todas as *tasks* do sistema com suas respectivas prioridades.




### Simulação

Uma demonstração do sistema funcionando está disponível no video "funcionamento_brew_master.mp4", e consiste em utilizar um arduino secudário para fazer o papel de aquecimento e sensor de temperatura 1, mostrando, junto com a interface gráfica, o funcionamento da statechart, comunicação I2C, controle PID e seu PWM, funcionamento do timer, adição de configuração de curvas, dentre outros.



[![Clique aqui para ver a demonstração em vídeo](https://img.youtube.com/vi/GieusbFribU/hqdefault.jpg)](https://www.youtube.com/watch?v=GieusbFribU)
