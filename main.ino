#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <DHTesp.h>
#include <ESP_Mail_Client.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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
char apiKey[20] = "A7ZK7OU2HLTFCS3N";
char emailDestino[50] = "juniorfredson5209@gmail.com";
char limiteTemp[5] = "22";
char limiteUmid[5] = "49";
char idDispositivo[30] = "ESP8266_TCC_Empresa1";

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
const unsigned long intervaloAtualizacaoLCD = 10000; // 10 segundos (ciclo completo)
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

// --- Função para Carregar Configurações do SPIFFS ---
void carregarConfiguracoes() {
    if (!SPIFFS.begin()) {
        Serial.println("Falha ao iniciar SPIFFS");
        return;
    }
    File configFile = SPIFFS.open("/config.txt", "r");
    if (configFile) {
        String line;
        line = configFile.readStringUntil('\n'); strncpy(apiKey, line.c_str(), sizeof(apiKey) - 1); apiKey[sizeof(apiKey) - 1] = '\0';
        line = configFile.readStringUntil('\n'); strncpy(emailDestino, line.c_str(), sizeof(emailDestino) - 1); emailDestino[sizeof(emailDestino) - 1] = '\0';
        line = configFile.readStringUntil('\n'); strncpy(limiteTemp, line.c_str(), sizeof(limiteTemp) - 1); limiteTemp[sizeof(limiteTemp) - 1] = '\0';
        line = configFile.readStringUntil('\n'); strncpy(limiteUmid, line.c_str(), sizeof(limiteUmid) - 1); limiteUmid[sizeof(limiteUmid) - 1] = '\0';
        line = configFile.readStringUntil('\n'); strncpy(idDispositivo, line.c_str(), sizeof(idDispositivo) - 1); idDispositivo[sizeof(idDispositivo) - 1] = '\0';
        configFile.close();
        Serial.println("Configurações carregadas do SPIFFS");
    } else {
        Serial.println("Nenhum arquivo de configuração encontrado, usando valores padrão");
    }
    SPIFFS.end();
}

// --- Função para Salvar Configurações no SPIFFS ---
void salvarConfiguracoes() {
    if (!SPIFFS.begin()) {
        Serial.println("Falha ao iniciar SPIFFS");
        return;
    }
    File configFile = SPIFFS.open("/config.txt", "w");
    if (configFile) {
        configFile.println(apiKey);
        configFile.println(emailDestino);
        configFile.println(limiteTemp);
        configFile.println(limiteUmid);
        configFile.println(idDispositivo);
        configFile.close();
        Serial.println("Configurações salvas no SPIFFS");
    } else {
        Serial.println("Falha ao salvar configurações no SPIFFS");
    }
    SPIFFS.end();
}

// --- Página Web para Configuração ---
void handleRoot() {
    char html[512];
    snprintf(html, sizeof(html),
             "<html><body><h1>Configuracao ESP8266</h1>"
             "<form method='POST' action='/save'>"
             "API Key: <input type='text' name='apiKey' value='%s'><br>"
             "Email: <input type='text' name='email' value='%s'><br>"
             "Limite Temp: <input type='text' name='limiteTemp' value='%s'><br>"
             "Limite Umid: <input type='text' name='limiteUmid' value='%s'><br>"
             "ID Dispositivo: <input type='text' name='idDispositivo' value='%s'><br>"
             "<input type='submit' value='Salvar'></form></body></html>",
             apiKey, emailDestino, limiteTemp, limiteUmid, idDispositivo);
    server.send(200, "text/html", html);
}

// --- Salvar Configurações do Formulário ---
void handleSave() {
    if (server.hasArg("apiKey")) strncpy(apiKey, server.arg("apiKey").c_str(), sizeof(apiKey) - 1); apiKey[sizeof(apiKey) - 1] = '\0';
    if (server.hasArg("email")) strncpy(emailDestino, server.arg("email").c_str(), sizeof(emailDestino) - 1); emailDestino[sizeof(emailDestino) - 1] = '\0';
    if (server.hasArg("limiteTemp")) strncpy(limiteTemp, server.arg("limiteTemp").c_str(), sizeof(limiteTemp) - 1); limiteTemp[sizeof(limiteTemp) - 1] = '\0';
    if (server.hasArg("limiteUmid")) strncpy(limiteUmid, server.arg("limiteUmid").c_str(), sizeof(limiteUmid) - 1); limiteUmid[sizeof(limiteUmid) - 1] = '\0';
    if (server.hasArg("idDispositivo")) strncpy(idDispositivo, server.arg("idDispositivo").c_str(), sizeof(idDispositivo) - 1); idDispositivo[sizeof(idDispositivo) - 1] = '\0';

    salvarConfiguracoes();
    Serial.println("Configurações salvas via web");
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
    if (client.connect("api.thingspeak.com", 80)) {
        client.setTimeout(5000);
        char url[128];
        snprintf(url, sizeof(url), "/update?api_key=%s&field1=%.2f&field2=%.2f", apiKey, temperatura, umidade);
        client.print("GET ");
        client.print(url);
        client.print(" HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: close\r\n\r\n");
        client.stop();
        Serial.println("Dados enviados ao ThingSpeak: T=" + String(temperatura) + "C, U=" + String(umidade) + "%");
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

// --- Email de Confirmação ---
void enviarEmailConfirmacao() {
    if (!emailConfirmacaoEnviado) {
        char mensagem[128];
        snprintf(mensagem, sizeof(mensagem), "Parabéns, você está conectado ao %s, e ele está operando corretamente.", idDispositivo);
        enviarEmail("Confirmação de conexão", mensagem);
        emailConfirmacaoEnviado = true;
    }
}

// --- Verificação de Alertas com LEDs, Buzzer e Mensagens no LCD ---
void verificarAlertas(float temperatura, float umidade) {
    float tempLimite = atof(limiteTemp);
    float umidLimite = atof(limiteUmid);

    // Calcula a diferença percentual em relação aos limites
    float diffTemp = abs(temperatura - tempLimite) / tempLimite * 100.0;
    float diffUmid = abs(umidade - umidLimite) / umidLimite * 100.0;
    float maxDiff = max(diffTemp, diffUmid); // Usa a maior diferença para determinar o estado

    // Define os estados com base nos percentuais
    bool dentroNormal = (maxDiff <= 15.0); // 0% a 15%
    bool alertaAmarelo = (maxDiff > 15.0 && maxDiff <= 20.0); // 16% a 20%
    bool emergencia = (maxDiff > 20.0); // 21% ou mais

    if (dentroNormal) {
        digitalWrite(LED_VERDE, HIGH);
        digitalWrite(LED_AMARELO, LOW);
        digitalWrite(LED_VERMELHO, LOW);
        if (!mostrandoIP) {
            lcd.setCursor(0, 0);
            lcd.print("LAB VACINAS         ");
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
            Serial.println("Estado Amarelo - Buzzer ligado");
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
            Serial.println("Estado Vermelho - Buzzer ligado");
            tone(BUZZER_PIN, 3000);
            buzzerInicio = millis();
            buzzerAtivo = true;
        }
        if (!emergenciaAtiva || (millis() - ultimoEnvioEmergencia >= intervaloMinimoEmergencia)) {
            char mensagem[128];
            snprintf(mensagem, sizeof(mensagem), "Emergência detectada!\nTemp: %.2f°C (Limite: %.2f)\nUmid: %.2f%% (Limite: %.2f)", 
                     temperatura, tempLimite, umidade, umidLimite);
            enviarEmail("Alerta de Emergência", mensagem);
            emergenciaAtiva = true;
            ultimoEnvioEmergencia = millis();
        }
    }

    if (buzzerAtivo && (millis() - buzzerInicio >= buzzerDuracao)) {
        noTone(BUZZER_PIN);
        buzzerAtivo = false;
        Serial.println("Buzzer desligado");
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
        lcd.setCursor(0, 0);
        lcd.print("                ");
        if (buzzerAtivo) {
            noTone(BUZZER_PIN);
            buzzerAtivo = false;
            Serial.println("Buzzer desligado devido a erro");
        }
        return;
    }

    Serial.print("Leitura: Temp="); Serial.print(temperatura); Serial.print("°C, Umid="); Serial.print(umidade); Serial.println("%");
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

// --- Função para atualizar o LCD (alternar entre dados e IP) ---
void atualizarLCD() {
    if (millis() - ultimaAtualizacaoLCD >= intervaloAtualizacaoLCD) {
        if (!mostrandoIP) {
            // Inicia a exibição do IP
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
        // Volta aos dados do sensor após 5 segundos
        mostrandoIP = false;
        lcd.clear();
        ultimaAtualizacaoLCD = millis();
        leituraSensor(); // Força uma atualização imediata dos dados
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
    WiFiManagerParameter tempParam("tempLimite", "Limite Temp", limiteTemp, 5);
    WiFiManagerParameter umidParam("umidLimite", "Limite Umid", limiteUmid, 5);
    WiFiManagerParameter idParam("idDevice", "ID Dispositivo", idDispositivo, 30);

    wm.addParameter(&apiKeyParam);
    wm.addParameter(&emailParam);
    wm.addParameter(&tempParam);
    wm.addParameter(&umidParam);
    wm.addParameter(&idParam);

    if (!wm.autoConnect("ThingSpeak", "esp82663")) {
        Serial.println("Falha na conexão Wi-Fi! Reiniciando...");
        ESP.restart();
    }

    Serial.println("Wi-Fi conectado!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());

    carregarConfiguracoes();

    File configFile = SPIFFS.open("/config.txt", "r");
    if (!configFile) {
        strncpy(apiKey, apiKeyParam.getValue(), sizeof(apiKey) - 1); apiKey[sizeof(apiKey) - 1] = '\0';
        strncpy(emailDestino, emailParam.getValue(), sizeof(emailDestino) - 1); emailDestino[sizeof(emailDestino) - 1] = '\0';
        strncpy(limiteTemp, tempParam.getValue(), sizeof(limiteTemp) - 1); limiteTemp[sizeof(limiteTemp) - 1] = '\0';
        strncpy(limiteUmid, umidParam.getValue(), sizeof(limiteUmid) - 1); limiteUmid[sizeof(limiteUmid) - 1] = '\0';
        strncpy(idDispositivo, idParam.getValue(), sizeof(idDispositivo) - 1); idDispositivo[sizeof(idDispositivo) - 1] = '\0';
        salvarConfiguracoes();
        Serial.println("Configurações iniciais salvas do WiFiManager");
    } else {
        configFile.close();
    }

    setupNTP();
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.begin();
    Serial.println("Servidor web iniciado");
    MailClient.networkReconnect(true);
    smtp.callback([](SMTP_Status status) {});

    enviarEmailConfirmacao();
}

// --- Loop Principal ---
void loop() {
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
            Serial.println("WiFi desconectado, reconectando...");
            WiFi.reconnect();
            ultimoTentativaReconexao = millis();
        }
        delay(50);
        return;
    }

    if (millis() - ultimoEnvioThingSpeak >= intervaloThingSpeak) {
        float temp = dht.getTemperature();
        float umid = dht.getHumidity();
        if (!isnan(temp) && !isnan(umid)) {
            enviarParaThingSpeak(temp, umid);
            ultimoEnvioThingSpeak = millis();
        } else {
            Serial.println("Erro na leitura para ThingSpeak");
        }
    }

    if (millis() - ultimoEnvioEmail >= intervaloEnvioEmail) {
        enviarRelatorio();
    }

    delay(50);
}