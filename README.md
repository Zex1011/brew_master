# brew_master
Projeto de sistemas embarcados

# Requisitos básicos gerais do projeto combinados em sala
-> Sensor de temperatura através de interface I2C

-> GPIO para saída do misturador

-> PWM ou GPIO com histerese para controle de aquecimento

-> Conter curva padrão --> 67º - t0; 78º - t1; 85 - t2.

-> Sistema de adição de curva

-> Interface pelo computador (PC <-> ESP)

-> Detectar erros no processo e mostrar avisos

-> Controle de temperatura através de PID ou on/off com histerese.

-> Documentação com: descrição de hardware, stateshart

-> DOXYGEN -> opcional para documentação

-> Ligar o Mixer quando os sensores de temperatura tiver 1º de diferença (manter por um período após estabilizar)


# Requisitos reais definidos para o projeto

## Requisitos Funcionais (RF)

| Código    | Descrição                                                                                                                                              |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **RF‑01** | Medir a temperatura da cerveha em tempo real.                                                                                                          |
| **RF‑02** | Controlar o misturador (ligar/desligar) durante o processo.                                                                                            |
| **RF‑03** | Controlar o aquecedor mantendo a temperatura‑alvo.                                                                                                     |
| **RF‑04** | Permitir a execução da curva padrão.                                                                                                                   |
| **RF‑05** | Permitir ao usuário criar e editar curvas de temperatura personalizadas.                                                                                |
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

