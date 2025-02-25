#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <DHTesp.h>
#include <ESP_Mail_Client.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <UrlEncode.h>

// --- Definições de Pinos ---
#define SDA_PIN 5      // D1 - SDA para LCD
#define SCL_PIN 4      // D2 - SCL para LCD
#define BUZZER_PIN 16  // D0 - Pino do buzzer (GPIO16)
#define DHT_PIN 14     // D5 - Pino do sensor DHT11
#define LED_VERDE 12   // D6 - LED verde
#define LED_AMARELO 13 // D7 - LED amarelo
#define LED_VERMELHO 15// D8 - LED vermelho
#define LED_PIN LED_BUILTIN // Pino para manter power bank ligado

// --- Configurações de Hardware ---
DHTesp dht;
LiquidCrystal_I2C lcd(0x27, 16, 2);
ESP8266WebServer server(80);

// --- Configurações de Rede e Email ---
char apiKey[20] = "WOZ7Z41VYW66EAEU"; // API Key do ThingSpeak
char emailDestino[50] = "juniorfredson5209@gmail.com";
char tempMin[5] = "2";   // Temperatura mínima (2°C)
char tempMax[5] = "8";   // Temperatura máxima (8°C)
char umidMin[5] = "30";  // Umidade mínima (30%)
char umidMax[5] = "60";  // Umidade máxima (60%)
char idDispositivo[30] = "ESP8266_TCC_Empresa1";

// --- Configurações do WhatsApp (CallMeBot) ---
String phoneNumber = "556899993206";
String whatsAppApiKey = "6279478";

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT esp_mail_smtp_port_465
#define AUTHOR_EMAIL "suportetcc5209@gmail.com"
#define AUTHOR_PASSWORD "dnmv euye sumb bzid"

// --- Variáveis Globais ---
SMTPSession smtp;
WiFiClient client;
float somaTemperatura = 0;
float somaUmidade = 0;
int contadorLeituras = 0;
unsigned long ultimoEnvioThingSpeak = 0;
unsigned long ultimoEnvioEmail = 0;
unsigned long ultimoEnvioEmergencia = 0;
unsigned long ultimaAtualizacaoLCD = 0;
bool mostrandoIP = false;
const unsigned long intervaloEnvioEmail = 3600000; // 1 hora
const unsigned long intervaloMinimoEmergencia = 300000; // 5 minutos
const unsigned long intervaloAtualizacaoLCD = 10000; // 10 segundos
const unsigned long duracaoIP = 5000; // 5 segundos para exibir o IP
bool emailConfirmacaoEnviado = false;
bool emergenciaAtiva = false;

// --- Controle do Buzzer ---
unsigned long buzzerInicio = 0;
const unsigned long buzzerDuracao = 500;
bool buzzerAtivo = false;

// --- Controle de Reconexão Wi-Fi ---
unsigned long ultimoTentativaReconexao = 0;
const unsigned long intervaloReconexao = 5000;

// --- Temporização manual ---
unsigned long ultimaLeituraSensor = 0;
const unsigned long intervaloLeituraSensor = 2000; // 2 segundos
unsigned long ultimaManterPowerBank = 0;
const unsigned long intervaloManterPowerBank = 1000; // 1 segundo
const unsigned long intervaloThingSpeak = 30000; // 30 segundos

// --- Função para Remover Caracteres Indesejados ---
void limparString(char* str, size_t tamanho) {
    for (size_t i = 0; i < tamanho; i++) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0';
            break;
        }
    }
}

// --- Função para Enviar Mensagem pelo WhatsApp ---
void sendWhatsAppMessage(String message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi não conectado, abortando envio ao WhatsApp");
        return;
    }

    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + whatsAppApiKey + "&text=" + urlEncode(message);
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.setTimeout(15000);
    if (http.begin(client, url)) {
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0) {
            Serial.println("Código HTTP WhatsApp: " + String(httpResponseCode));
        } else {
            Serial.println("Erro HTTP: " + String(httpResponseCode));
        }
        http.end();
    } else {
        Serial.println("Falha ao iniciar conexão com o CallMeBot");
    }
}

// --- Função para Carregar Configurações do SPIFFS ---
void carregarConfiguracoes() {
    if (!SPIFFS.begin()) {
        Serial.println("Falha ao iniciar SPIFFS");
        lcd.setCursor(0, 0);
        lcd.print("Erro SPIFFS     ");
        delay(2000);
        return;
    }
    File configFile = SPIFFS.open("/config.txt", "r");
    if (configFile) {
        String line;
        line = configFile.readStringUntil('\n'); strncpy(apiKey, line.c_str(), sizeof(apiKey) - 1); apiKey[sizeof(apiKey) - 1] = '\0'; limparString(apiKey, sizeof(apiKey));
        line = configFile.readStringUntil('\n'); strncpy(emailDestino, line.c_str(), sizeof(emailDestino) - 1); emailDestino[sizeof(emailDestino) - 1] = '\0'; limparString(emailDestino, sizeof(emailDestino));
        line = configFile.readStringUntil('\n'); strncpy(tempMin, line.c_str(), sizeof(tempMin) - 1); tempMin[sizeof(tempMin) - 1] = '\0'; limparString(tempMin, sizeof(tempMin));
        line = configFile.readStringUntil('\n'); strncpy(tempMax, line.c_str(), sizeof(tempMax) - 1); tempMax[sizeof(tempMax) - 1] = '\0'; limparString(tempMax, sizeof(tempMax));
        line = configFile.readStringUntil('\n'); strncpy(umidMin, line.c_str(), sizeof(umidMin) - 1); umidMin[sizeof(umidMin) - 1] = '\0'; limparString(umidMin, sizeof(umidMin));
        line = configFile.readStringUntil('\n'); strncpy(umidMax, line.c_str(), sizeof(umidMax) - 1); umidMax[sizeof(umidMax) - 1] = '\0'; limparString(umidMax, sizeof(umidMax));
        line = configFile.readStringUntil('\n'); strncpy(idDispositivo, line.c_str(), sizeof(idDispositivo) - 1); idDispositivo[sizeof(idDispositivo) - 1] = '\0'; limparString(idDispositivo, sizeof(idDispositivo));
        configFile.close();
        Serial.println("Configurações carregadas do SPIFFS:");
        Serial.println("API Key: [" + String(apiKey) + "]");
        Serial.println("Email: [" + String(emailDestino) + "]");
        Serial.println("Temp Min: [" + String(tempMin) + "]");
        Serial.println("Temp Max: [" + String(tempMax) + "]");
        Serial.println("Umid Min: [" + String(umidMin) + "]");
        Serial.println("Umid Max: [" + String(umidMax) + "]");
        Serial.println("ID Dispositivo: [" + String(idDispositivo) + "]");
    } else {
        Serial.println("Nenhum arquivo de configuração encontrado, mantendo valores padrão ou do WiFiManager");
    }
    SPIFFS.end();
}

// --- Função para Salvar Configurações no SPIFFS ---
void salvarConfiguracoes() {
    if (!SPIFFS.begin()) {
        Serial.println("Falha ao iniciar SPIFFS");
        lcd.setCursor(0, 0);
        lcd.print("Erro SPIFFS     ");
        delay(2000);
        return;
    }
    File configFile = SPIFFS.open("/config.txt", "w");
    if (configFile) {
        configFile.print(apiKey); configFile.print("\n");
        configFile.print(emailDestino); configFile.print("\n");
        configFile.print(tempMin); configFile.print("\n");
        configFile.print(tempMax); configFile.print("\n");
        configFile.print(umidMin); configFile.print("\n");
        configFile.print(umidMax); configFile.print("\n");
        configFile.print(idDispositivo); configFile.print("\n");
        configFile.close();
        Serial.println("Configurações salvas no SPIFFS:");
        Serial.println("API Key: [" + String(apiKey) + "]");
        Serial.println("Email: [" + String(emailDestino) + "]");
        Serial.println("Temp Min: [" + String(tempMin) + "]");
        Serial.println("Temp Max: [" + String(tempMax) + "]");
        Serial.println("Umid Min: [" + String(umidMin) + "]");
        Serial.println("Umid Max: [" + String(umidMax) + "]");
        Serial.println("ID Dispositivo: [" + String(idDispositivo) + "]");
    } else {
        Serial.println("Falha ao salvar configurações no SPIFFS");
    }
    SPIFFS.end();
}

// --- Variável global para o buffer HTML ---
char html[1024]; // Aumentado para 1024 caracteres

// --- Página Web para Configuração ---
void handleRoot() {
    snprintf(html, sizeof(html),
             "<html><body><h1>Configuracao ESP8266</h1>"
             "<form method='POST' action='/save'>"
             "API Key: <input type='text' name='apiKey' value='%s'><br>"
             "Email: <input type='text' name='email' value='%s'><br>"
             "Temp Min: <input type='text' name='tempMin' value='%s'><br>"
             "Temp Max: <input type='text' name='tempMax' value='%s'><br>"
             "Umid Min: <input type='text' name='umidMin' value='%s'><br>"
             "Umid Max: <input type='text' name='umidMax' value='%s'><br>"
             "ID Dispositivo: <input type='text' name='idDispositivo' value='%s'><br>"
             "<input type='submit' value='Salvar'></form></body></html>",
             apiKey, emailDestino, tempMin, tempMax, umidMin, umidMax, idDispositivo);
    server.send(200, "text/html", html);
}

// --- Salvar Configurações do Formulário ---
void handleSave() {
    if (server.hasArg("apiKey")) strncpy(apiKey, server.arg("apiKey").c_str(), sizeof(apiKey) - 1); apiKey[sizeof(apiKey) - 1] = '\0'; limparString(apiKey, sizeof(apiKey));
    if (server.hasArg("email")) strncpy(emailDestino, server.arg("email").c_str(), sizeof(emailDestino) - 1); emailDestino[sizeof(emailDestino) - 1] = '\0'; limparString(emailDestino, sizeof(emailDestino));
    if (server.hasArg("tempMin")) strncpy(tempMin, server.arg("tempMin").c_str(), sizeof(tempMin) - 1); tempMin[sizeof(tempMin) - 1] = '\0'; limparString(tempMin, sizeof(tempMin));
    if (server.hasArg("tempMax")) strncpy(tempMax, server.arg("tempMax").c_str(), sizeof(tempMax) - 1); tempMax[sizeof(tempMax) - 1] = '\0'; limparString(tempMax, sizeof(tempMax));
    if (server.hasArg("umidMin")) strncpy(umidMin, server.arg("umidMin").c_str(), sizeof(umidMin) - 1); umidMin[sizeof(umidMin) - 1] = '\0'; limparString(umidMin, sizeof(umidMin));
    if (server.hasArg("umidMax")) strncpy(umidMax, server.arg("umidMax").c_str(), sizeof(umidMax) - 1); umidMax[sizeof(umidMax) - 1] = '\0'; limparString(umidMax, sizeof(umidMax));
    if (server.hasArg("idDispositivo")) strncpy(idDispositivo, server.arg("idDispositivo").c_str(), sizeof(idDispositivo) - 1); idDispositivo[sizeof(idDispositivo) - 1] = '\0'; limparString(idDispositivo, sizeof(idDispositivo));

    salvarConfiguracoes();
    server.send(200, "text/html", "<html><body><h1>Configuracoes salvas!</h1><a href='/'>Voltar</a></body></html>");
}

// --- Função de Configuração NTP ---
void setupNTP() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    while (now < 1000) {
        delay(500);
        now = time(nullptr);
    }
    Serial.println("NTP sincronizado");
}

// --- Função de Envio de Email ---
void enviarEmail(const char* assunto, const char* mensagem) {
    Session_Config config;
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = F("meudominio.net");

    SMTP_Message message;
    message.sender.name = "Monitoramento DHT11";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = assunto;
    message.addRecipient("Administrador", emailDestino);
    message.text.content = mensagem;
    message.text.charSet = "utf-8";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&config)) {
        Serial.println("Falha na conexão SMTP");
        return;
    }
    if (MailClient.sendMail(&smtp, &message)) {
        Serial.println("E-mail enviado: " + String(assunto));
    } else {
        Serial.println("Falha ao enviar e-mail: " + String(smtp.errorReason()));
    }
}

// --- Envio de Dados ao ThingSpeak ---
void enviarParaThingSpeak(float temperatura, float umidade) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi não conectado, abortando envio ao ThingSpeak");
        return;
    }

    WiFiClient client;
    if (client.connect("api.thingspeak.com", 80)) {
        String url = "/update?api_key=" + String(apiKey) + "&field1=" + String(temperatura, 2) + "&field2=" + String(umidade, 2);
        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: api.thingspeak.com\r\n" +
                     "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (client.connected() && millis() - timeout < 5000) {
            if (client.available()) {
                String response = client.readStringUntil('\r');
                if (response.startsWith("HTTP/1.1 200 OK")) {
                    Serial.println("Dados enviados ao ThingSpeak");
                }
                break;
            }
            delay(10);
        }
        client.stop();
    } else {
        Serial.println("Falha ao conectar ao ThingSpeak");
    }
}

// --- Envio de Relatório por Email ---
void enviarRelatorio() {
    if (contadorLeituras == 0) return;
    float mediaTemperatura = somaTemperatura / contadorLeituras;
    float mediaUmidade = somaUmidade / contadorLeituras;

    char texto[128];
    snprintf(texto, sizeof(texto), "Relatório:\nTemperatura Média: %.2f°C\nUmidade Média: %.2f%%", mediaTemperatura, mediaUmidade);
    enviarEmail("Relatório Temperatura e Umidade", texto);

    somaTemperatura = 0;
    somaUmidade = 0;
    contadorLeituras = 0;
    ultimoEnvioEmail = millis();
}

// --- Confirmação de Conexão ---
void enviarEmailConfirmacao() {
    if (!emailConfirmacaoEnviado) {
        char mensagem[128];
        snprintf(mensagem, sizeof(mensagem), "Parabéns, você está conectado ao %s, e ele está operando corretamente.", idDispositivo);
        enviarEmail("Confirmação de conexão", mensagem);
        sendWhatsAppMessage(mensagem);
        emailConfirmacaoEnviado = true;
    }
}

// --- Verificação de Alertas com LEDs, Buzzer e Mensagens ---
void verificarAlertas(float temperatura, float umidade) {
    float tempMinFloat = atof(tempMin);  // 2°C
    float tempMaxFloat = atof(tempMax);  // 8°C
    float umidMinFloat = atof(umidMin);  // 30%
    float umidMaxFloat = atof(umidMax);  // 60%

    // Calcula variações percentuais para temperatura
    float diffTempMin = 0, diffTempMax = 0;
    if (temperatura < tempMinFloat) {
        diffTempMin = (tempMinFloat - temperatura) / tempMinFloat * 100.0;
    } else if (temperatura > tempMaxFloat) {
        diffTempMax = (temperatura - tempMaxFloat) / tempMaxFloat * 100.0;
    }

    // Calcula variações percentuais para umidade
    float diffUmidMin = 0, diffUmidMax = 0;
    if (umidade < umidMinFloat) {
        diffUmidMin = (umidMinFloat - umidade) / umidMinFloat * 100.0;
    } else if (umidade > umidMaxFloat) {
        diffUmidMax = (umidade - umidMaxFloat) / umidMaxFloat * 100.0;
    }

    float maxDiffTemp = max(diffTempMin, diffTempMax);
    float maxDiffUmid = max(diffUmidMin, diffUmidMax);
    float maxDiff = max(maxDiffTemp, maxDiffUmid);

    bool dentroNormal = (temperatura >= tempMinFloat && temperatura <= tempMaxFloat) && 
                        (umidade >= umidMinFloat && umidade <= umidMaxFloat);
    bool alertaAmarelo = (maxDiff > 0 && maxDiff <= 15.0);
    bool emergencia = (maxDiff > 20.0);

    if (dentroNormal) {
        digitalWrite(LED_VERDE, HIGH);
        digitalWrite(LED_AMARELO, LOW);
        digitalWrite(LED_VERMELHO, LOW);
        if (!mostrandoIP) {
            lcd.setCursor(0, 0);
            lcd.print("LAB VACINAS     ");
        }
        noTone(BUZZER_PIN);
        buzzerAtivo = false;
        emergenciaAtiva = false;
    } else if (alertaAmarelo) {
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_AMARELO, HIGH);
        digitalWrite(LED_VERMELHO, LOW);
        if (!mostrandoIP) {
            lcd.setCursor(0, 0);
            lcd.print("Alerta: Verifique");
        }
        if (!buzzerAtivo) {
            tone(BUZZER_PIN, 1500);
            buzzerInicio = millis();
            buzzerAtivo = true;
        }
        emergenciaAtiva = false;
    } else if (emergencia) {
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_AMARELO, LOW);
        digitalWrite(LED_VERMELHO, HIGH);
        if (!mostrandoIP) {
            lcd.setCursor(0, 0);
            lcd.print("Emergencia!!!   ");
        }
        if (!buzzerAtivo) {
            tone(BUZZER_PIN, 3000);
            buzzerInicio = millis();
            buzzerAtivo = true;
        }
        if (!emergenciaAtiva || (millis() - ultimoEnvioEmergencia >= intervaloMinimoEmergencia)) {
            char mensagem[256];
            if (maxDiffTemp > maxDiffUmid) {
                if (temperatura < tempMinFloat) {
                    snprintf(mensagem, sizeof(mensagem), 
                             "Emergência detectada!\nTemp: %.2f°C (Mínimo: %.2f)\nUmid: %.2f%%", 
                             temperatura, tempMinFloat, umidade);
                } else {
                    snprintf(mensagem, sizeof(mensagem), 
                             "Emergência detectada!\nTemp: %.2f°C (Máximo: %.2f)\nUmid: %.2f%%", 
                             temperatura, tempMaxFloat, umidade);
                }
            } else {
                if (umidade < umidMinFloat) {
                    snprintf(mensagem, sizeof(mensagem), 
                             "Emergência detectada!\nTemp: %.2f°C\nUmid: %.2f%% (Mínimo: %.2f)", 
                             temperatura, umidade, umidMinFloat);
                } else {
                    snprintf(mensagem, sizeof(mensagem), 
                             "Emergência detectada!\nTemp: %.2f°C\nUmid: %.2f%% (Máximo: %.2f)", 
                             temperatura, umidade, umidMaxFloat);
                }
            }
            enviarEmail("Alerta de Emergência", mensagem);
            sendWhatsAppMessage(mensagem);
            emergenciaAtiva = true;
            ultimoEnvioEmergencia = millis();
        }
    }

    if (buzzerAtivo && (millis() - buzzerInicio >= buzzerDuracao)) {
        noTone(BUZZER_PIN);
        buzzerAtivo = false;
    }
}

// --- Função de Leitura do Sensor ---
void leituraSensor() {
    float umidade = dht.getHumidity();
    float temperatura = dht.getTemperature();

    if (isnan(temperatura) || isnan(umidade) || temperatura < -20 || temperatura > 60 || umidade < 0 || umidade > 100) {
        Serial.println("Leitura inválida do sensor DHT11");
        lcd.setCursor(0, 1);
        lcd.print("Erro Sensor     ");
        return;
    }

    Serial.print("Temp="); Serial.print(temperatura); Serial.print("°C, Umid="); Serial.print(umidade); Serial.println("%");
    if (!mostrandoIP) {
        lcd.setCursor(0, 1);
        lcd.print("T:");
        lcd.print(temperatura);
        lcd.print("C U:");
        lcd.print(umidade);
        lcd.print("% ");
    }

    somaTemperatura += temperatura;
    somaUmidade += umidade;
    contadorLeituras++;

    verificarAlertas(temperatura, umidade);
}

// --- Função para manter Power Bank ligado ---
void manterPowerBank() {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
}

// --- Função para atualizar o LCD ---
void atualizarLCD() {
    if (millis() - ultimaAtualizacaoLCD >= intervaloAtualizacaoLCD) {
        if (!mostrandoIP) {
            String ip = WiFi.localIP().toString();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("IP:             ");
            lcd.setCursor(0, 1);
            lcd.print(ip + "    ");
            mostrandoIP = true;
            ultimaAtualizacaoLCD = millis();
        }
    } else if (mostrandoIP && (millis() - ultimaAtualizacaoLCD >= duracaoIP)) {
        mostrandoIP = false;
        lcd.clear();
        ultimaAtualizacaoLCD = millis();
        leituraSensor();
    }
}

// --- Configuração Inicial ---
void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_AMARELO, OUTPUT);
    pinMode(LED_VERMELHO, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("ESP8266 Iniciando");

    dht.setup(DHT_PIN, DHTesp::DHT11);

    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFiManager wm;
    WiFiManagerParameter apiKeyParam("apiKey", "Chave da API", apiKey, 20);
    WiFiManagerParameter emailParam("email", "E-mail", emailDestino, 50);
    WiFiManagerParameter tempMinParam("tempMin", "Temp Min", tempMin, 5);
    WiFiManagerParameter tempMaxParam("tempMax", "Temp Max", tempMax, 5);
    WiFiManagerParameter umidMinParam("umidMin", "Umid Min", umidMin, 5);
    WiFiManagerParameter umidMaxParam("umidMax", "Umid Max", umidMax, 5);
    WiFiManagerParameter idParam("idDevice", "ID Dispositivo", idDispositivo, 30);

    wm.addParameter(&apiKeyParam);
    wm.addParameter(&emailParam);
    wm.addParameter(&tempMinParam);
    wm.addParameter(&tempMaxParam);
    wm.addParameter(&umidMinParam);
    wm.addParameter(&umidMaxParam);
    wm.addParameter(&idParam);

    if (!wm.autoConnect("ThingSpeak", "esp82663")) {
        Serial.println("Falha na conexão Wi-Fi! Reiniciando...");
        ESP.restart();
    }

    Serial.println("Wi-Fi conectado!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());

    carregarConfiguracoes();

    if (!SPIFFS.begin()) {
        Serial.println("Falha ao iniciar SPIFFS na verificação");
    }
    File configFile = SPIFFS.open("/config.txt", "r");
    if (!configFile) {
        strncpy(apiKey, apiKeyParam.getValue(), sizeof(apiKey) - 1); apiKey[sizeof(apiKey) - 1] = '\0'; limparString(apiKey, sizeof(apiKey));
        strncpy(emailDestino, emailParam.getValue(), sizeof(emailDestino) - 1); emailDestino[sizeof(emailDestino) - 1] = '\0'; limparString(emailDestino, sizeof(emailDestino));
        strncpy(tempMin, tempMinParam.getValue(), sizeof(tempMin) - 1); tempMin[sizeof(tempMin) - 1] = '\0'; limparString(tempMin, sizeof(tempMin));
        strncpy(tempMax, tempMaxParam.getValue(), sizeof(tempMax) - 1); tempMax[sizeof(tempMax) - 1] = '\0'; limparString(tempMax, sizeof(tempMax));
        strncpy(umidMin, umidMinParam.getValue(), sizeof(umidMin) - 1); umidMin[sizeof(umidMin) - 1] = '\0'; limparString(umidMin, sizeof(umidMin));
        strncpy(umidMax, umidMaxParam.getValue(), sizeof(umidMax) - 1); umidMax[sizeof(umidMax) - 1] = '\0'; limparString(umidMax, sizeof(umidMax));
        strncpy(idDispositivo, idParam.getValue(), sizeof(idDispositivo) - 1); idDispositivo[sizeof(idDispositivo) - 1] = '\0'; limparString(idDispositivo, sizeof(idDispositivo));
        salvarConfiguracoes();
    }
    SPIFFS.end();

    setupNTP();
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.begin();
    MailClient.networkReconnect(true);

    float temp = dht.getTemperature();
    float umid = dht.getHumidity();
    if (!isnan(temp) && !isnan(umid)) {
        enviarParaThingSpeak(temp, umid);
    }

    enviarEmailConfirmacao();
}

// --- Loop Principal ---
void loop() {
    if (millis() - ultimoEnvioThingSpeak >= intervaloThingSpeak) {
        float temp = dht.getTemperature();
        float umid = dht.getHumidity();
        if (!isnan(temp) && !isnan(umid)) {
            enviarParaThingSpeak(temp, umid);
            ultimoEnvioThingSpeak = millis();
        }
    }

    server.handleClient();
    ESP.wdtFeed();

    if (millis() - ultimaLeituraSensor >= intervaloLeituraSensor) {
        leituraSensor();
        ultimaLeituraSensor = millis();
    }

    if (millis() - ultimaManterPowerBank >= intervaloManterPowerBank) {
        manterPowerBank();
        ultimaManterPowerBank = millis();
    }

    atualizarLCD();

    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - ultimoTentativaReconexao >= intervaloReconexao) {
            WiFi.reconnect();
            ultimoTentativaReconexao = millis();
        }
        delay(50);
        return;
    }

    if (millis() - ultimoEnvioEmail >= intervaloEnvioEmail) {
        enviarRelatorio();
    }

    delay(50);
}
