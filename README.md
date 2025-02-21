Monitoramento de Temperatura e Umidade com ESP8266
Este projeto utiliza um ESP8266 para monitorar temperatura e umidade com um sensor DHT11, exibindo os dados em um LCD I2C, enviando-os ao ThingSpeak e acionando alertas via e-mail, LEDs e buzzer com base em limites configuráveis. O sistema também oferece uma interface web para ajustar os parâmetros.

Funcionalidades
Leitura do sensor: Mede temperatura e umidade a cada 2 segundos com um DHT11.

 Exibição no LCD: Mostra os dados do sensor e alterna com o IP a cada 10 segundos (IP visível por 5 segundos).

 Envio ao ThingSpeak: Envia dados de temperatura e umidade a cada 30 segundos.

 Alertas:


LED Verde: Diferença de 0% a 15% em relação aos limites de temperatura ou umidade (buzzer desligado).

 LED Amarelo: Diferença de 16% a 20%, com buzzer a 1500 Hz por 500 ms.

 LED Vermelho: Diferença acima de 21%, com buzzer a 3000 Hz por 500 ms e envio de e-mail a cada 5 minutos durante a emergência.




 Relatórios: Envia um relatório por e-mail com médias a cada hora.

 Configuração via Web: Interface acessível pelo IP para ajustar API Key, e-mail, limites e ID do dispositivo, salvos no SPIFFS.

 Manutenção de Power Bank: Pulsa um pino a cada 1 segundo para evitar desligamento.

Hardware Necessário
ESP8266 (ex.: NodeMCU ou ESP-01)

 Sensor DHT11

 LCD I2C 16x2 (endereço 0x27)

 LEDs (verde, amarelo, vermelho)

 Buzzer

 Power bank (opcional, para alimentação)

 Resistores e jumpers para conexões



Conexões
Pino ESP8266
Componente
D1 (GPIO5)
SDA (LCD I2C)
D2 (GPIO4)
SCL (LCD I2C)
D0 (GPIO16)
Buzzer
D5 (GPIO14)
DHT11
D6 (GPIO12)
LED Verde
D7 (GPIO13)
LED Amarelo
D8 (GPIO15)
LED Vermelho
LED_BUILTIN
Power Bank (opcional)

Dependências
Bibliotecas Arduino:

ESP8266WiFi

 WiFiClient

 ESP8266WebServer

 WiFiManager (by tzapu)

 DHTesp (by beegee_tokyo)

 ESP_Mail_Client (by mobizt)

 Wire

 LiquidCrystal_I2C (by frankdebrabander)



Instale essas bibliotecas via Gerenciador de Bibliotecas no Arduino IDE.
Instalação

Clone o repositório: Abra o terminal e execute: git clone https://github.com/FJR5209/STU-ThingSpeak.git 
 Configure o hardware: Conecte os componentes conforme a tabela de conexões.

 Instale as dependências: No Arduino IDE, vá em Sketch > Include Library > Manage Libraries e instale as bibliotecas listadas.

 Carregue o código: Abra main.ino no Arduino IDE. Selecione a placa ESP8266 (ex.: NodeMCU 1.0) em Tools > Board. Conecte o ESP8266 via USB e clique em Upload.

 Configuração inicial: O ESP8266 criará um AP chamado "ThingSpeak" com senha "esp82663". Conecte-se a esse AP e configure o Wi-Fi pelo portal. Após conectado, o IP será exibido no LCD e no Serial.



Uso

Monitoramento: O LCD alterna entre dados do sensor e o IP (ex.: "172.16.4.0"). LEDs e buzzer indicam o estado com base nos limites.

 Ajuste de parâmetros: Acesse o IP no navegador. Altere API Key, E-mail, Limite Temp, Limite Umid e ID Dispositivo. Clique em "Salvar" para persistir no SPIFFS.

 ThingSpeak: Configure um canal no ThingSpeak e use a API Key padrão (ILYB0ARLD1MSHWL7) ou a sua própria.

 E-mails: Alertas de emergência são enviados a cada 5 minutos enquanto a condição persistir (LED vermelho). Relatórios são enviados hourly para o e-mail configurado.



Exemplo de Configuração

Limites padrão: limiteTemp = 25, limiteUmid = 61.

 Verde: Temp 21.25°C a 28.75°C, Umid 51.85% a 70.15% (buzzer desligado).

 Amarelo: Temp 20°C a 21.24°C ou 28.76°C a 30°C, Umid 48.8% a 51.84% ou 70.16% a 73.2% (buzzer 1500 Hz).

 Vermelho: Temp < 20°C ou > 30°C, Umid < 48.8% ou > 73.2% (buzzer 3000 Hz).



Contribuições

Sinta-se à vontade para abrir issues ou pull requests no GitHub para melhorias ou correções.

Licença



Créditos

Desenvolvido por Fredson Junior.

