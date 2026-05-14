EcoWatts

Sistema inteligente de monitoramento de consumo elétrico utilizando ESP32 e sensores ACS712, com exibição em LCD, envio de dados em JSON via Wi-Fi e integração futura com dashboard web e Power BI.

Objetivo do Projeto

O EcoWatts foi desenvolvido para:

Monitorar consumo de energia em tempo real
Calcular o custo aproximado da energia consumida
Exibir informações localmente em um display LCD
Enviar dados via Wi-Fi em formato JSON
Permitir integração com dashboards e banco de dados
Gerar insights de economia energética

O projeto foi pensado para aplicações educacionais, demonstrações em feiras tecnológicas e evolução futura para soluções profissionais de monitoramento elétrico.

Componentes Utilizados
ESP32
Sensor de corrente ACS712
Display LCD 16x2 com módulo I2C
Botões físicos
Fonte de alimentação
Rede Wi-Fi
Funcionamento

O sensor ACS712 mede a corrente elétrica do circuito.

O Sensor 1 fica em série com o disjuntor geral, sendo responsável pela medição da potência total do sistema.

A ESP32:

Lê os sensores
Calcula:
Corrente
Potência
Consumo estimado
Custo da energia
Mostra os dados no LCD
Envia os dados em JSON via Wi-Fi
Estrutura dos Botões
Botão Verde
Clique rápido:
Alterna os modos do display LCD
Clique longo:
Zera o custo local exibido no LCD
Segundo Botão
Clique longo:
Reseta as configurações Wi-Fi
Reabre o portal do WiFiManager
Envio de Dados JSON

Exemplo de JSON enviado pela ESP32:

{
  "corrente": 1.42,
  "potencia_total": 312.8,
  "consumo_kwh": 0.1542,
  "custo_local": 1.3456,
  "custo_historico": 25.8834,
  "wifi_rssi": -58
}


Explicação dos Campos

| Campo           | Descrição                               |
| --------------- | --------------------------------------- |
| corrente        | Corrente medida pelo ACS712             |
| potencia_total  | Potência total instantânea              |
| consumo_kwh     | Energia acumulada                       |
| custo_local     | Valor exibido no LCD                    |
| custo_historico | Valor acumulado enviado para plataforma |
| wifi_rssi       | Intensidade do sinal Wi-Fi              |

Conectividade Wi-Fi

O projeto utiliza WiFiManager.

Quando não existe Wi-Fi salvo:

a ESP32 cria uma rede própria
o usuário conecta pelo celular/notebook
escolhe a rede Wi-Fi disponível
a ESP salva automaticamente

Isso facilita apresentações em feiras, onde diferentes equipes podem compartilhar internet usando hotspot do celular.

Integração Futura

O EcoWatts foi planejado para integração com:

Dashboard Web
Banco de Dados
APIs REST
Power BI
Sistemas IoT
Possíveis Evoluções
Histórico diário/mensal
Alertas de alto consumo
Gráficos em tempo real
Controle remoto
Integração com nuvem
Monitoramento por setores
Aplicativo mobile
Tecnologias
C++
Arduino IDE
ESP32
JSON
WiFiManager
I2C LCD
Git/GitHub
Autores

Projeto desenvolvido por:

Luis Felipe Tavares Galvão
Equipe EcoWatts
Licença

Projeto educacional desenvolvido para fins acadêmicos e tecnológicos.


