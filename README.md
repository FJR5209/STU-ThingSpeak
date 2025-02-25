Documentação do Código: Monitoramento de Temperatura e Umidade com ESP8266

Data: 25 de fevereiro de 2025 
Versão: 1.2

Autor: Fredson Junior


Plataforma: ESP8266

Objetivo

Monitorar temperatura e umidade usando um sensor DHT11, exibir informações em um LCD I2C, enviar dados para o ThingSpeak, emitir alertas via LEDs, buzzer, e-mails e WhatsApp, e permitir configuração remota via interface web acessada pelo IP do ESP8266.

Visão Geral

Este código implementa um sistema de monitoramento ambiental baseado no microcontrolador ESP8266. Ele utiliza um sensor DHT11 para medir temperatura e umidade, exibe os dados em um LCD I2C 16x2, controla LEDs (verde, amarelo, vermelho) e um buzzer para indicar estados (normal, alerta, emergência), e envia relatórios e alertas por e-mail (via SMTP Gmail) e WhatsApp (via CallMeBot). Os dados são enviados periodicamente ao ThingSpeak para armazenamento na nuvem. As configurações, incluindo limites de temperatura e umidade (mínimos e máximos), e-mail de destino, chave da API do ThingSpeak, identificador do dispositivo, número de telefone e chave da API do CallMeBot, podem ser ajustadas remotamente via uma interface web ou via WiFiManager durante a configuração inicial.

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
 UrlEncode.h: Codificação de URLs para envio de mensagens via WhatsApp.



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



3. Configurações de Rede e E-mail

Variáveis de configuração (armazenadas no SPIFFS):

apiKey[20]: Chave da API do ThingSpeak.
 emailDestino[50]: E-mail do destinatário dos relatórios e alertas.
tempMin[5] e tempMax[5]: Limites mínimo (ex.: 2°C) e máximo (ex.: 8°C) de temperatura.
 umidMin[5] e umidMax[5]: Limites mínimo (ex.: 30%) e máximo (ex.: 60%) de umidade.
 idDispositivo[30]: Identificador do dispositivo.
 phoneNumber[15]: Número de telefone para envio de mensagens via WhatsApp (ex.: "+556812345678").
whatsAppApiKey[10]: Chave da API do CallMeBot para envio de mensagens via WhatsApp (ex.: "6279478").



Constantes de e-mail:

SMTP_HOST: "smtp.gmail.com".
 SMTP_PORT: Porta 465 (SSL).
 AUTHOR_EMAIL e AUTHOR_PASSWORD: Credenciais do remetente (e-mail Gmail e senha de app).



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

carregarConfiguracoes():


Propósito: Carrega parâmetros do arquivo /config.txt no SPIFFS.
Comportamento: Se o arquivo não existir, define valores padrão e os salva.
Saída: Configurações em variáveis globais.
 salvarConfiguracoes():


Propósito: Salva as configurações atuais no SPIFFS.
 Comportamento: Sobrescreve o arquivo /config.txt com os valores das variáveis globais.
 handleRoot():


Propósito: Gera a página HTML de configuração.
 Saída: Formulário web com campos para apiKey, emailDestino, tempMin, tempMax, umidMin, umidMax, idDispositivo, phoneNumber, e whatsAppApiKey.
 handleSave():


Propósito: Processa os dados enviados pelo formulário web.
 Comportamento: Atualiza as variáveis globais e salva no SPIFFS.

 setupNTP():


Propósito: Sincroniza o horário via NTP.
 Comportamento: Usa servidores "pool.ntp.org" e "time.nist.gov".
 enviarEmail(const char* assunto, const char* mensagem):


Propósito: Envia e-mails via SMTP (Gmail).
 Entradas: Assunto e corpo da mensagem.
 Saída: Log no monitor serial sobre sucesso ou falha.

 enviarParaThingSpeak(float temperatura, float umidade):


Propósito: Envia dados ao ThingSpeak via HTTP GET.
 Entradas: Temperatura e umidade atuais.
 enviarRelatorio():


Propósito: Calcula médias e envia relatório por e-mail a cada hora.
 Comportamento: Reseta acumuladores após envio.
enviarEmailConfirmacao():


Propósito: Envia um e-mail único ao conectar à rede, também enviado via WhatsApp.



 sendWhatsAppMessage(String message):


Propósito: Envia mensagens via WhatsApp usando a API do CallMeBot.

 Entradas: Mensagem a ser enviada.

 Comportamento: Utiliza phoneNumber e whatsAppApiKey configuráveis para autenticação.
 verificarAlertas(float temperatura, float umidade):


Propósito: Verifica limites e atualiza LEDs, buzzer, LCD, e-mails e WhatsApp.

 Lógica:


Calcula diferença percentual entre valores medidos e limites (mínimos e máximos).

 Estados:


Normal (≤15% fora da faixa): LED verde, exibe "LAB VACINAS" no LCD.
 Alerta (16-20% fora da faixa): LED amarelo, buzzer a 1500 Hz, mensagem "Alerta: Verifique" no LCD.
 Emergência (>20% fora da faixa): LED vermelho, buzzer a 3000 Hz, mensagem "Emergencia!!!" no LCD, e-mails e mensagens via WhatsApp.



 leituraSensor():


Propósito: Lê o sensor DHT11 e atualiza o LCD com valores.

 Comportamento: Valida leituras; em caso de erro, exibe "Erro Sensor".
 manterPowerBank():


Propósito: Pulsa o pino LED_PIN para evitar desligamento do power bank.
 atualizarLCD():


Propósito: Alterna entre exibir dados do sensor e o IP a cada 10 segundos.
 setup():


Propósito: Inicializa hardware, Wi-Fi, servidor web e serviços de e-mail.

 Comportamento: Configura pinos, carrega parâmetros, conecta à rede via WiFiManager, e inicia serviços.
 loop():


Propósito: Executa a lógica principal em loop contínuo.

 Tarefas:


Lê sensor a cada 2 segundos.
 Mantém power bank ativo.
 Atualiza LCD.
 Reconecta Wi-Fi se necessário.
 Envia dados ao ThingSpeak a cada 30 segundos.
 Envia relatório por e-mail e WhatsApp a cada hora.
 Processa requisições web para configuração.



Fluxo de Operação

Inicialização (setup):


Configura pinos e periféricos.
 Carrega configurações do SPIFFS ou define padrões.
 Conecta à rede Wi-Fi via WiFiManager.
 Inicia servidor web e sincroniza NTP.
 Loop Principal (loop):


Lê temperatura e umidade periodicamente.
 Verifica alertas e atualiza LEDs, buzzer e LCD.
 Envia dados ao ThingSpeak e relatórios por e-mail/WhatsApp.
 Processa requisições web para configuração.
 Configuração Remota:


Acessível via IP do ESP8266 (ex.: 192.168.x.x).
 Permite alterar todos os parâmetros via formulário web, incluindo número de telefone e chave da API do WhatsApp.
Requisitos de Hardware

ESP8266 (ex.: NodeMCU).
 Sensor DHT11 conectado ao pino D5.
 LCD I2C 16x2 (endereço 0x27) nos pinos D1 (SDA) e D2 (SCL).
 LEDs (verde, amarelo, vermelho) nos pinos D6, D7, D8.
 Buzzer no pino D0.
 Power bank (opcional, mantido via LED_BUILTIN).



Configuração Inicial

Instale as bibliotecas listadas no Arduino IDE.
 Carregue o código no ESP8266.
 Conecte-se à rede "ThingSpeak" (senha: "esp82663") para configurar Wi-Fi e parâmetros iniciais via WiFiManager.
 Acesse a interface web pelo IP exibido no LCD ou no monitor serial.



Limitações e Observações

LCD: Máximo de 16 caracteres para o texto exibido.
 ThingSpeak: Requer conexão estável e chave da API válida.
 E-mail e WhatsApp: Limite de envio do Gmail e CallMeBot pode ser atingido em cenários de emergência frequente.
 SPIFFS: Capacidade limitada; evite configurações muito longas (ex.: números de telefone ou chaves de API longos podem exceder o tamanho dos arrays).
 Sensor DHT11: Precisão limitada (±2°C e ±5% para umidade); para aplicações críticas (ex.: vacinas), considere um sensor mais preciso (DHT22, BME280).
 Número de Telefone e Chave API: Certifique-se de que o número de telefone não exceda 14 dígitos (incluindo código do país) e a chave da API não exceda 9 caracteres.



Exemplo de Uso

Alterar número de telefone e chave da API do WhatsApp:


Acesse http://<IP_do_ESP8266>/.
 No formulário, insira "Phone Number: +55689991234" e "WhatsApp API Key: 1234567".
 Clique em "Salvar".
 O sistema usará o novo número e chave para enviar mensagens via WhatsApp.
 Alterar limites de temperatura e umidade:


No mesmo formulário, insira Temp Min: 2, Temp Max: 8, Umid Min: 30, Umid Max: 60.
 Clique em "Salvar".
 O sistema monitorará temperaturas entre 2°C e 8°C, e umidade entre 30% e 60%, com alertas correspondentes.
 Monitoramento:


LEDs indicam estado em tempo real (verde: normal, amarelo: alerta, vermelho: emergência).
 Relatórios são enviados por e-mail e WhatsApp a cada hora ou em emergências.






Possíveis Melhorias

Adicionar validação mais robusta para entradas web (ex.: formatar número de telefone, validar chave da API).
 Implementar OTA (Over-The-Air) para atualizações remotas.
 Suportar múltiplos sensores ou limites configuráveis por sensor.
 Usar um sensor mais preciso (ex.: DHT22, BME280) para maior acurácia.
 Adicionar criptografia para credenciais sensíveis (ex.: chave da API, senha de e-mail).


Notas sobre as Atualizações:

Atualizei a versão para "1.2" para refletir as novas funcionalidades.
 Adicionei phoneNumber e whatsAppApiKey nas seções de "Configurações de Rede e E-mail", "Funções Principais" (com sendWhatsAppMessage), e "Exemplo de Uso".
 Incluí observações sobre os limites de tamanho para phoneNumber e whatsAppApiKey, bem como na seção de "Limitações e Observações".
 Mantive a data atual (25 de fevereiro de 2025) para refletir a última atualização.
 Ajustei a descrição para enfatizar a configuração remota via IP para todos os parâmetros, incluindo os novos campos do WhatsApp.

