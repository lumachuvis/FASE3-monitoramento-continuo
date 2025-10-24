# FIAP - Faculdade de Informática e Administração Paulista

<p align="center">
<a href= "https://www.fiap.com.br/"><img src="assets/logo-fiap.png" alt="FIAP - Faculdade de Informática e Admnistração Paulista" border="0" width=40% height=40%></a>
</p>

<br>

# FASE 3: Monitoramento Contínuo – IoT na Saúde

## Beginner Coders

## 👨‍🎓 Integrantes: 
- <a href="https://www.linkedin.com/in/luana-porto-pereira-gomes/">Luana Porto Pereira Gomes</a>
- <a href="https://www.linkedin.com/in/luma-x">Luma Oliveira</a>
- <a href="https://www.linkedin.com/in/priscilla-oliveira-023007333/">Priscilla Oliveira </a>
- <a href="https://www.linkedin.com/in/paulobernardesqs?utm_source=share&utm_campaign=share_via&utm_content=profile&utm_medium=ios_app">Paulo Bernardes</a>  

## 👩‍🏫 Professores:
### Tutor(a) 
- <a href="https://www.linkedin.com/in/leonardoorabona/">Leonardo Ruiz</a>
### Coordenador(a)
- <a href="https://www.linkedin.com/in/profandregodoi/">André Godoi</a>


## 📜 Descrição

Implementação das Partes 1 e 2 do projeto CardioIA, focado na integração de conceitos de IoT, computação em nuvem (Cloud) e Edge/Fog Computing. Este projeto foi desenvolvido para a disciplina "Capítulo 1 – CardioIA Conectada: IoT e Visualização de Dados para Saúde Digital" do curso de Inteligência Artificial (3º Semestre) da FIAP.

Nesta fase, desenvolvemos um protótipo funcional que simula um sistema vestível de monitoramento cardíaco. A solução é capaz de capturar sinais vitais simulados, armazenar e processar informações localmente (Edge Computing), transmitir os dados para a nuvem via MQTT e exibir resultados em dashboards interativos (Node-RED).

## 📝 Sobre o Projeto

Foram utilizados dados simulados de sensores no ambiente Wokwi:

    DHT22: Sensor para medição de temperatura e umidade, representando uma medição ambiental próxima ao paciente.
    
    MPU6050: Acelerômetro e Giroscópio para detectar movimento e atividade (ex: esforço físico).
    
    Botão (Push Button): Usado para simular manualmente os batimentos cardíacos (BPM). O sistema conta os cliques por janela de 10s e converte para BPM.
    
    Chave Deslizante: Usada para simular manualmente o status da conectividade (ONLINE/OFFLINE).

Observação Importante (SPIFFS vs. Cartão SD): A orientação inicial previa o uso do SPIFFS/LittleFS para o armazenamento em flash interna. Entretanto, no simulador Wokwi, essas partições não são emuladas de forma completa, gerando falhas de montagem e impedindo a escrita real.
Por isso, optamos pelo cartão SD (microSD), que o simulador suporta de forma estável. Essa troca manteve o objetivo pedagógico: provar a resiliência offline (edge-first) e a sincronização automática quando o dispositivo retorna ao modo online.

## 📁 Estrutura de pastas

Dentre os arquivos e pastas presentes na raiz do projeto, definem-se:

<b>assets</b>: aqui estão os arquivos relacionados a elementos não-estruturados deste repositório, como diagramas de arquitetura ou prints do dashboard.
    
<b>codigo_esp32</b>: aqui está o código-fonte em C++ (formato .ino) para ser utilizado no Wokwi ou Arduino IDE.
    
<b>dashboard_nodered</b>: aqui está o arquivo .json de exportação do fluxo do Node-RED.
    
<b>relatorios</b>: aqui estão os relatórios descritivos da Parte 1 (fluxo de funcionamento e resiliência) e Parte 2 (comunicação MQTT e dashboard).
    
<b>README.md</b>: arquivo que serve como guia e explicação geral sobre o projeto (o mesmo que você está lendo agora).

## 🔧 Como executar o código
### ✅ Parte 1: Simulação no Wokwi (ESP32)

Acesse o projeto com todas as etapas de hardware simuladas: 👉 https://wokwi.com/projects/445625615909410817
    Inicie a simulação.
    Observe as conexões do DHT22, MPU6050, Botão, LED, Chave Deslizante e Módulo SD.
    Use a Chave Deslizante para alternar entre os modos ONLINE e OFFLINE.
    Observe o "Serial Monitor":
        No modo OFFLINE, os dados são salvos no buffer do SD.
        Ao mudar para ONLINE, o sistema "sincroniza" os dados (imprime no monitor) e limpa o buffer.
    Pressione o Botão para simular BPM e teste os alertas (ex: BPM > 120 ou Temp > 38°C) para ver o LED de Alerta local acender.
    
### ☁️ Parte 2: Transmissão para Nuvem e Visualização (Node-RED)

Acesse sua instância do Node-RED (ex: via FlowFuse).
    Importe o fluxo disponível em dashboard_nodered/flow.json.
    Configure os nós "MQTT In" com as credenciais do seu broker (HiveMQ Cloud) e os tópicos corretos.
    Execute o código do ESP32 (com a lógica de publicação MQTT real, não apenas a simulação do monitor serial).
    Acesse a UI (User Interface) do Node-RED para visualizar os dados em tempo real.

## 🧪 Etapas Realizadas 
### 🔹 PARTE 1 – Armazenamento e processamento local (Edge Computing):

Desenvolvimento da aplicação no Wokwi com ESP32.
Leitura de 3 sensores/entradas distintas (DHT22, MPU6050 e Botão para BPM).
Adaptação (SPIFFS/SD): Uso de Cartão SD (microSD) para armazenamento local, motivada pelas limitações de emulação do SPIFFS no Wokwi.
Simulação de conectividade Wi-Fi via Chave Deslizante (ONLINE/OFFLINE).
Implementação de lógica de resiliência offline (salva no SD) e sincronização (envia dados ao ficar online e limpa o buffer).
Inclusão de Alerta Local (LED) para condições críticas (Temp > 38 °C ou BPM > 120).

### 🔹 PARTE 2 – Transmissão para nuvem e visualização (Fog/Cloud Computing):

Envio de dados do ESP32 para o broker HiveMQ Cloud via protocolo MQTT (publicações a cada 3 segundos).
Criação de um dashboard no Node-RED (FlowFuse) para exibição em tempo real.
Componentes do Dashboard:
    chart: Exibe a variação dos batimentos cardíacos (BPM).
    gauge: Mostra a temperatura corporal.
    text: Mostra o status atual (“Normal” ou “⚠ ALERTA”).
Implementação de lógica de alerta no dashboard (ex: bpm = 130 ou temp = 39.2).

## 🗃 Histórico de lançamentos

* 0.1.0 - 24/10/2025
    *

## 📋 Licença

<img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/cc.svg?ref=chooser-v1"><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/by.svg?ref=chooser-v1"><p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/"><a property="dct:title" rel="cc:attributionURL" href="https://github.com/agodoi/template">MODELO GIT FIAP</a> por <a rel="cc:attributionURL dct:creator" property="cc:attributionName" href="https://fiap.com.br">Fiap</a> está licenciado sobre <a href="http://creativecommons.org/licenses/by/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">Attribution 4.0 International</a>.</p>
