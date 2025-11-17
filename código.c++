// importa as bibliotecas
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
// Configurações WiFi
const char* ssid = "Inteli.Iot";
const char* password = "%(Yk(sxGMtvFEs.3";
// Configurações MQTT (HiveMQ)
const char* mqtt_server = "8e80b959383043a492c08186f3bed4d1.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "teste";
const char* mqtt_password = "Teste123";
const char* mqtt_client_id = "ESP32_Traffic_Light";
const char* mqtt_topic_status = "traffic/status";
const char* mqtt_topic_command = "traffic/command";
// Pinos do Semáforo Principal (existem dois semáforos com esses pinos)
const int LED_PRINCIPAL_VERMELHO = 21;
const int LED_PRINCIPAL_AMARELO = 22;
const int LED_PRINCIPAL_VERDE = 23;
// Pinos do Semáforo Secundário
const int LED_SECUNDARIO_VERMELHO = 16;
const int LED_SECUNDARIO_AMARELO = 17;
const int LED_SECUNDARIO_VERDE = 18;
// Pinos do Sensor de Proximidade
const int TRIG_PIN = 5;
const int ECHO_PIN = 34;
// Pino do Sensor de Luminosidade (LDR)
const int LDR_PIN = 35;  // Pino analógico para LDR
// Parâmetros ajustáveis
int distanciaDeteccao = 50; // cm
int tempoVermelho = 5000; // ms
int tempoAmarelo = 2000; // ms
int tempoAmareloSecundario = 2000; // ms
int limiarLuminosidadeBaixo = 500;  // Abaixo deste valor ativa modo noturno
int limiarLuminosidadeAlto = 700;   // Acima deste valor desativa modo noturno
bool sistemaAtivo = true;
bool modoNoturno = false;
bool modoNoturnoForcado = false;  // Para controle manual
// Variáveis de controle
bool pedestreDetectado = false;
unsigned long ultimaMudanca = 0;
unsigned long ultimoPisca = 0;
bool estadoPisca = false;
enum Estado {
  VERDE_PRINCIPAL,
  AMARELO_PRINCIPAL,
  VERMELHO_PRINCIPAL,
  AMARELO_SECUNDARIO_TRANSICAO,
  MODO_NOTURNO
};
Estado estadoAtual = VERDE_PRINCIPAL;
WiFiClientSecure espClient;
PubSubClient client(espClient);
void setup() {
  Serial.begin(115200);
  // Configurar pinos dos LEDs
  pinMode(LED_PRINCIPAL_VERMELHO, OUTPUT);
  pinMode(LED_PRINCIPAL_AMARELO, OUTPUT);
  pinMode(LED_PRINCIPAL_VERDE, OUTPUT);
  pinMode(LED_SECUNDARIO_VERMELHO, OUTPUT);
  pinMode(LED_SECUNDARIO_AMARELO, OUTPUT);
  pinMode(LED_SECUNDARIO_VERDE, OUTPUT);
  // Configurar pinos dos sensores
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  // Estado inicial
  setLuzVerde();
  // Conectar WiFi
  setup_wifi();
  // Configurar SSL
  espClient.setInsecure();
  // Configurar MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("Sistema iniciado!");
}
//função para conectar ao wifi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Mensagem recebida: ");
  Serial.println(message);
  // Processar comandos
  if (message.startsWith("DISTANCE:")) {
    distanciaDeteccao = message.substring(9).toInt();
    Serial.print("Nova distância: ");
    Serial.println(distanciaDeteccao);
  }
  else if (message.startsWith("TIME_RED:")) {
    tempoVermelho = message.substring(9).toInt();
    Serial.print("Novo tempo vermelho: ");
    Serial.println(tempoVermelho);
  }
  else if (message.startsWith("TIME_YELLOW:")) {
    tempoAmarelo = message.substring(12).toInt();
    Serial.print("Novo tempo amarelo: ");
    Serial.println(tempoAmarelo);
  }
  else if (message.startsWith("BRIGHTNESS_LOW:")) {
    limiarLuminosidadeBaixo = message.substring(15).toInt();
    Serial.print("Novo limiar baixo: ");
    Serial.println(limiarLuminosidadeBaixo);
  }
  else if (message.startsWith("BRIGHTNESS_HIGH:")) {
    limiarLuminosidadeAlto = message.substring(16).toInt();
    Serial.print("Novo limiar alto: ");
    Serial.println(limiarLuminosidadeAlto);
  }
  else if (message == "NIGHT_MODE_ON") {
    modoNoturnoForcado = true;
    modoNoturno = true;
    Serial.println("Modo noturno forçado ON");
  }
  else if (message == "NIGHT_MODE_OFF") {
    modoNoturnoForcado = false;
    modoNoturno = false;
    setLuzVerde();
    Serial.println("Modo noturno forçado OFF");
  }
  else if (message == "NIGHT_MODE_AUTO") {
    modoNoturnoForcado = false;
    Serial.println("Modo noturno automático");
  }
  else if (message == "ENABLE") {
    sistemaAtivo = true;
    Serial.println("Sistema ativado");
  }
  else if (message == "DISABLE") {
    sistemaAtivo = false;
    Serial.println("Sistema desativado");
  }
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      client.subscribe(mqtt_topic_command);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}
long medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);
  long distancia = duracao * 0.034 / 2;
  if (distancia == 0 || distancia > 400) {
    return -1;
  }
  return distancia;
}
int medirLuminosidade() {
  return analogRead(LDR_PIN);  // Retorna valor entre 0-4095
}
void setLuzVerde() {
  digitalWrite(LED_PRINCIPAL_VERMELHO, LOW);
  digitalWrite(LED_PRINCIPAL_AMARELO, LOW);
  digitalWrite(LED_PRINCIPAL_VERDE, HIGH);
  digitalWrite(LED_SECUNDARIO_VERMELHO, HIGH);
  digitalWrite(LED_SECUNDARIO_AMARELO, LOW);
  digitalWrite(LED_SECUNDARIO_VERDE, LOW);
  estadoAtual = VERDE_PRINCIPAL;
}
void setLuzAmarela() {
  digitalWrite(LED_PRINCIPAL_VERMELHO, LOW);
  digitalWrite(LED_PRINCIPAL_AMARELO, HIGH);
  digitalWrite(LED_PRINCIPAL_VERDE, LOW);
  digitalWrite(LED_SECUNDARIO_VERMELHO, HIGH);
  digitalWrite(LED_SECUNDARIO_AMARELO, LOW);
  digitalWrite(LED_SECUNDARIO_VERDE, LOW);
  estadoAtual = AMARELO_PRINCIPAL;
}
void setLuzVermelha() {
  digitalWrite(LED_PRINCIPAL_VERMELHO, HIGH);
  digitalWrite(LED_PRINCIPAL_AMARELO, LOW);
  digitalWrite(LED_PRINCIPAL_VERDE, LOW);
  digitalWrite(LED_SECUNDARIO_VERMELHO, LOW);
  digitalWrite(LED_SECUNDARIO_AMARELO, LOW);
  digitalWrite(LED_SECUNDARIO_VERDE, HIGH);
  estadoAtual = VERMELHO_PRINCIPAL;
}
void setAmareloSecundario() {
  digitalWrite(LED_PRINCIPAL_VERMELHO, HIGH);
  digitalWrite(LED_PRINCIPAL_AMARELO, LOW);
  digitalWrite(LED_PRINCIPAL_VERDE, LOW);
  digitalWrite(LED_SECUNDARIO_VERMELHO, LOW);
  digitalWrite(LED_SECUNDARIO_AMARELO, HIGH);
  digitalWrite(LED_SECUNDARIO_VERDE, LOW);
  estadoAtual = AMARELO_SECUNDARIO_TRANSICAO;
}
void modoNoturnoPiscar() {
  if (millis() - ultimoPisca >= 500) {  // Pisca a cada 500ms
    estadoPisca = !estadoPisca;
    digitalWrite(LED_PRINCIPAL_VERMELHO, LOW);
    digitalWrite(LED_PRINCIPAL_AMARELO, estadoPisca ? HIGH : LOW);
    digitalWrite(LED_PRINCIPAL_VERDE, LOW);
    digitalWrite(LED_SECUNDARIO_VERMELHO, LOW);
    digitalWrite(LED_SECUNDARIO_AMARELO, estadoPisca ? HIGH : LOW);
    digitalWrite(LED_SECUNDARIO_VERDE, LOW);
    ultimoPisca = millis();
  }
  estadoAtual = MODO_NOTURNO;
}
void publicarStatus() {
  int luminosidade = medirLuminosidade();
  String status = "{";
  status += "\"estado\":\"";
  if (modoNoturno) status += "MODO_NOTURNO";
  else if (estadoAtual == VERDE_PRINCIPAL) status += "VERDE";
  else if (estadoAtual == AMARELO_PRINCIPAL) status += "AMARELO";
  else if (estadoAtual == VERMELHO_PRINCIPAL) status += "VERMELHO";
  else if (estadoAtual == AMARELO_SECUNDARIO_TRANSICAO) status += "AMARELO_SECUNDARIO";
  status += "\",";
  status += "\"pedestre_detectado\":" + String(pedestreDetectado ? "true" : "false") + ",";
  status += "\"distancia_configurada\":" + String(distanciaDeteccao) + ",";
  status += "\"tempo_vermelho\":" + String(tempoVermelho) + ",";
  status += "\"tempo_amarelo\":" + String(tempoAmarelo) + ",";
  status += "\"sistema_ativo\":" + String(sistemaAtivo ? "true" : "false") + ",";
  status += "\"modo_noturno\":" + String(modoNoturno ? "true" : "false") + ",";
  status += "\"modo_noturno_forcado\":" + String(modoNoturnoForcado ? "true" : "false") + ",";
  status += "\"luminosidade\":" + String(luminosidade);
  status += "}";
  client.publish(mqtt_topic_status, status.c_str());
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (!sistemaAtivo) {
    delay(100);
    return;
  }
  // Verificar luminosidade para modo noturno (apenas se não estiver forçado)
  int luminosidade = medirLuminosidade();
  if (!modoNoturnoForcado) {
    // Ativar modo noturno quando luminosidade baixa
    if (luminosidade < limiarLuminosidadeBaixo && !modoNoturno) {
      modoNoturno = true;
      Serial.println("MODO NOTURNO ATIVADO - Luminosidade baixa");
      publicarStatus();
    }
    // Desativar modo noturno quando luminosidade alta (histerese)
    else if (luminosidade > limiarLuminosidadeAlto && modoNoturno) {
      modoNoturno = false;
      Serial.println("MODO NOTURNO DESATIVADO - Luminosidade alta");
      setLuzVerde();
      pedestreDetectado = false;
      publicarStatus();
    }
  }
  // Se estiver em modo noturno, apenas piscar amarelo
  if (modoNoturno) {
    modoNoturnoPiscar();
    delay(100);
    return;
  }
  // Operação normal do semáforo
  long distancia = medirDistancia();
  // Debug
  static unsigned long ultimoDebug = 0;
  if (millis() - ultimoDebug >= 500) {
    Serial.print("Distância: ");
    Serial.print(distancia);
    Serial.print(" cm | Luminosidade: ");
    Serial.print(luminosidade);
    Serial.print(" | Estado: ");
    if (estadoAtual == VERDE_PRINCIPAL) Serial.println("VERDE");
    else if (estadoAtual == AMARELO_PRINCIPAL) Serial.println("AMARELO");
    else if (estadoAtual == VERMELHO_PRINCIPAL) Serial.println("VERMELHO");
    else if (estadoAtual == AMARELO_SECUNDARIO_TRANSICAO) Serial.println("AMARELO_SECUNDARIO");
    ultimoDebug = millis();
  }
  // Detecção de objeto
  if (distancia > 0 && distancia < distanciaDeteccao) {
    if (!pedestreDetectado && estadoAtual == VERDE_PRINCIPAL) {
      pedestreDetectado = true;
      setLuzAmarela();
      ultimaMudanca = millis();
      publicarStatus();
      Serial.println("OBJETO DETECTADO! Mudando para amarelo...");
    }
  }
  unsigned long agora = millis();
  if (pedestreDetectado) {
    if (estadoAtual == AMARELO_PRINCIPAL && (agora - ultimaMudanca >= tempoAmarelo)) {
      setLuzVermelha();
      ultimaMudanca = agora;
      publicarStatus();
      Serial.println("Semáforo vermelho - Via secundária liberada");
    }
    else if (estadoAtual == VERMELHO_PRINCIPAL && (agora - ultimaMudanca >= tempoVermelho)) {
      setAmareloSecundario();
      ultimaMudanca = agora;
      publicarStatus();
      Serial.println("Semáforo secundário amarelo - Preparando para fechar");
    }
    else if (estadoAtual == AMARELO_SECUNDARIO_TRANSICAO && (agora - ultimaMudanca >= tempoAmareloSecundario)) {
      setLuzVerde();
      pedestreDetectado = false;
      ultimaMudanca = agora;
      publicarStatus();
      Serial.println("Voltando ao verde - Via principal liberada");
    }
  }
  // Publicar status a cada 2 segundos
  static unsigned long ultimaPublicacao = 0;
  if (agora - ultimaPublicacao >= 2000) {
    publicarStatus();
    ultimaPublicacao = agora;
  }
  delay(100);
}