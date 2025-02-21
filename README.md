
Documentação do Código: Monitoramento de Temperatura e Umidade com ESP8266

Data: 21 de fevereiro de 2025


Versão: 1.0


Autor: Fredson Junior
Plataforma: ESP8266


Objetivo: Monitorar temperatura e umidade usando um sensor DHT11, exibir informações em um LCD, enviar dados para o ThingSpeak, emitir alertas via LEDs e e-mails, e permitir configuração remota via interface web.


Visão Geral

Este código implementa um sistema de monitoramento ambiental baseado no microcontrolador ESP8266. Ele utiliza um sensor DHT11 para medir temperatura e umidade, exibe os dados em um LCD I2C, controla LEDs para indicar estados (normal, alerta, emergência), aciona um buzzer em situações críticas e envia relatórios e alertas por e-mail. Os dados também são enviados periodicamente ao ThingSpeak para armazenamento na nuvem. Configurações como limites de temperatura/umidade e texto do LCD podem ser ajustadas remotamente via interface web ou WiFiManager.


Bibliotecas Utilizadas

ESP8266WiFi.h: Gerencia a conexão Wi-Fi do ESP8266.

 WiFiClient.h: Permite comunicação com servidores remotos (ex.: ThingSpeak).

 ESP8266WebServer.h: Cria um servidor web para configuração remota.

 WiFiManager.h: Facilita a configuração inicial da rede Wi-Fi.

 DHTesp.h: Interface com o sensor DHT11.

 ESP_Mail_Client.h: Envio de e-mails via SMTP.

 time.h: Sincronização de horário via NTP.

 Wire.h: Comunicação I2C para o LCD.

 LiquidCrystal_I2C.h: Controle do display LCD I2C.


Estrutura do Código

1. Definições de Pinos

Definem os pinos do ESP8266 utilizados para os componentes de hardware:

SDA_PIN (D1) e SCL_PIN (D2): Comunicação I2C com o LCD.

 BUZZER_PIN (D0): Controle do buzzer.

 DHT_PIN (D5): Leitura do sensor DHT11.

 LED_VERDE (D6), LED_AMARELO (D7), LED_VERMELHO (D8): Indicadores de estado.

 LED_PIN (LED_BUILTIN): Mantém o power bank ligado.



2. Configurações de Hardware
DHTesp dht: Instância do sensor DHT11.

 LiquidCrystal_I2C lcd: Instância do LCD I2C (endereço 0x27, 16x2).

 ESP8266WebServer server: Servidor web na porta 80.



3. Configurações de Rede e Email
Variáveis de configuração (armazenadas no SPIFFS):
apiKey[20]: Chave da API do ThingSpeak.

 emailDestino[50]: E-mail do destinatário dos relatórios/alertas.

 limiteTemp[5]: Temperatura limite (em °C).

 limiteUmid[5]: Umidade limite (em %).

 idDispositivo[30]: Identificador do dispositivo.

 textoLCD[17]: Texto exibido no LCD em estado normal (máx. 16 caracteres).

 Constantes de e-mail:
SMTP_HOST: "smtp.gmail.com".

 SMTP_PORT: Porta 465 (SSL).

 AUTHOR_EMAIL e AUTHOR_PASSWORD: Credenciais do remetente.





4. Variáveis Globais
SMTPSession smtp e WiFiClient client: Para envio de e-mails e comunicação HTTP.
 Controle de dados:
somaTemperatura, somaUmidade, contadorLeituras: Calculam médias para relatórios.
 Temporizadores (unsigned long):
ultimoEnvioThingSpeak, ultimoEnvioEmail, ultimoEnvioEmergencia, ultimaAtualizacaoLCD, etc.: Controlam intervalos de ações.
 Constantes de tempo:
intervaloEnvioEmail: 1 hora (3.600.000 ms).

 intervaloMinimoEmergencia: 5 minutos (300.000 ms).

 intervaloAtualizacaoLCD: 10 segundos (ciclo completo).

 duracaoIP: 5 segundos (exibição do IP).

 intervaloThingSpeak: 30 segundos.
 Flags:
mostrandoIP: Alterna entre dados e IP no LCD.

 emailConfirmacaoEnviado: Evita envio repetido de e-mail de confirmação.

 emergenciaAtiva: Indica estado de emergência ativo.
5. Funções Principais
carregarConfiguracoes()

Propósito: Carrega parâmetros do arquivo /config.txt no SPIFFS.
 Comportamento: Se o arquivo não existir, define valores padrão e os salva.
 Saída: Configurações em variáveis globais.



salvarConfiguracoes()

Propósito: Salva as configurações atuais no SPIFFS.
 Comportamento: Sobrescreve o arquivo /config.txt com os valores das variáveis globais.


handleRoot()
Propósito: Gera a página HTML de configuração.
 Saída: Formulário web com campos para apiKey, emailDestino, limiteTemp, limiteUmid, idDispositivo e textoLCD.



handleSave()

Propósito: Processa os dados enviados pelo formulário web.
 Comportamento: Atualiza as variáveis globais e salva no SPIFFS.



setupNTP()

Propósito: Sincroniza o horário via NTP.
 Comportamento: Usa servidores "pool.ntp.org" e "time.nist.gov".



enviarEmail(const char* assunto, const char* mensagem)

Propósito: Envia e-mails via SMTP (Gmail).
 Entradas: Assunto e corpo da mensagem.
 Saída: Log no monitor serial sobre sucesso ou falha.



enviarParaThingSpeak(float temperatura, float umidade)

Propósito: Envia dados ao ThingSpeak via HTTP GET.
 Entradas: Temperatura e umidade atuais.



enviarRelatorio()

Propósito: Calcula médias e envia relatório por e-mail a cada hora.
 Comportamento: Reseta acumuladores após envio.



enviarEmailConfirmacao()

Propósito: Envia e-mail único ao conectar à rede.



verificarAlertas(float temperatura, float umidade)

Propósito: Verifica limites e atualiza LEDs, buzzer e LCD.
 Lógica:
Calcula diferença percentual entre valores medidos e limites.

 Estados:


Normal (≤15%): LED verde, exibe textoLCD.
 Alerta (16-20%): LED amarelo, buzzer 1500 Hz, mensagem "Alerta: Verifique".
 Emergência (>20%): LED vermelho, buzzer 3000 Hz, mensagem "Emergencia!!!", e-mail de alerta.



 Saída: Atualiza hardware e logs no serial.



leituraSensor()

Propósito: Lê DHT11 e atualiza LCD com valores.
 Comportamento: Valida leituras; em caso de erro, exibe "Erro Sensor".



manterPowerBank()

Propósito: Pulsa o pino LED_PIN para evitar desligamento do power bank.



atualizarLCD()
Propósito: Alterna entre exibir dados do sensor e o IP a cada 10 segundos.
setup()
Propósito: Inicializa hardware, Wi-Fi, servidor web e e-mail.
 Comportamento: Configura pinos, carrega parâmetros, conecta à rede via WiFiManager e inicia serviços.



loop()

Propósito: Executa a lógica principal em loop contínuo.
 Tarefas:
Lê sensor a cada 2 segundos.

 Mantém power bank ativo.

 Atualiza LCD.

 Reconecta Wi-Fi se necessário.

 Envia dados ao ThingSpeak a cada 30 segundos.

 Envia relatório por e-mail a cada hora.


Fluxo de Operação

Inicialização (setup):
Configura pinos e periféricos.
 Carrega configurações do SPIFFS ou define padrões.
 Conecta à rede Wi-Fi via WiFiManager.
 Inicia servidor web e sincroniza NTP.
 Loop Principal (loop):
Lê temperatura e umidade periodicamente.
 Verifica alertas e atualiza LEDs, buzzer e LCD.
 Envia dados ao ThingSpeak e relatórios por e-mail.
 Processa requisições web para configuração.
 Configuração Remota:
Acessível via IP do ESP8266 (ex.: 192.168.x.x).
 Permite alterar todos os parâmetros, incluindo o texto do LCD.

Requisitos de Hardware

ESP8266 (ex.: NodeMCU).
Sensor DHT11 conectado ao pino D5.
LCD I2C 16x2 (endereço 0x27) nos pinos D1 (SDA) e D2 (SCL).
LEDs (verde, amarelo, vermelho) nos pinos D6, D7, D8.
Buzzer no pino D0.
Power bank (opcional, mantido via LED_BUILTIN).


Configuração Inicial

Instale as bibliotecas no Arduino IDE.
Carregue o código no ESP8266.
Conecte-se à rede "ThingSpeak" (senha: "esp82663") para configurar Wi-Fi e parâmetros iniciais.
Acesse a interface web pelo IP exibido no LCD ou serial.


Limitações e Observações

LCD: Máximo de 16 caracteres para textoLCD.
ThingSpeak: Requer conexão estável e API key válida.
E-mail: Limite de envio do Gmail pode ser atingido em cenários de emergência frequente.
SPIFFS: Capacidade limitada; evite configurações muito longas.


Exemplo de Uso

Alterar texto do LCD:
Acesse http://<IP_do_ESP8266>/.
 No campo "Texto LCD", insira "HEMOACRE".
 Clique em "Salvar".
 O LCD exibirá "HEMOACRE" em estado normal.
 Monitoramento:
LEDs indicam estado em tempo real.
Relatórios são enviados por e-mail a cada hora.


Possíveis Melhorias

Adicionar validação mais robusta para entradas web.
Implementar OTA (Over-The-Air) para atualizações remotas.
Suportar múltiplos sensores ou limites configuráveis por sensor.


