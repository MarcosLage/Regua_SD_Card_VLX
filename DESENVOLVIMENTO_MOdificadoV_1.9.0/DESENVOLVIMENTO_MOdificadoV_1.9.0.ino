/*
  *****MUDADO FORMATO DO ARQUIVO DE TXT PARA CSV E MUDADO DO UNO PARA O NANO.***

  PROJETO: MEDIDAS SALVAS EM SD CARD GERENCIANDO ARQUIVOS
  AUTOR:   MARCOS ANTONIO LAGE
  DATA :   27/09/2021 
  
  ===========================VERSÃO:  1.7.0======================================
  
  NOTAS DA VERSÃO: SALVA MEDIDAS CONFORME PRESSIONAR DO BOTAO E QUANDO PRECISAR
  DE UM NOVO ARQUIVO CRIA O MESMO ATRAVES DE OUTRO BOTAO E COMEÇA SALVAR MEDIDAS
  NO ARQUIVO ATUAL. QUANDO DESLIGA AUTOMATICAMENTE AO LIGAR ELE JA CRIA UM ARQUIVO
  NOVO PRONTO A SALVAR MEDIDAS NELE, NÃO PRECISA CRIAR NOVO MANUALMENTE, ESTE RECURSO
  DE NOVO ARQUIVO USA SOMENTE DURANTE A COLETA QUANDO PRECISAR SEPARAR AS MEDIDAS. 

    ===========================VERSÃO:  1.8.0======================================

  IMPLEMENTAÇÃO DO SENSOR INFRAVERMELHO VL53LOX NO LUGAR DO POTENCIOMETRO QUE SIMULAVA 
  O SENSOR.

      ===========================VERSÃO:  1.9.0======================================

  ATRIBUIDO FUNÇÃO PARA MONITORAR ESTADO DE TENSAO DA BATERIA ONDE 1 LED LIGARA SINALIZANDO A TROCA
  

 
  PINOS CONEXÃO MICRO SDCARD
  PINO CS ======> 10
  PINO SCK======> 13
  PINO MOSI=====> 11
  PINO MISO=====> 12
  PINO VCC======> +5V
  PINO GND======> GND
*/

//==========INCLUSÃO DE BIBLIOTECAS==========
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include <Wire.h>
#include <VL53L0X.h>                      //V1.3.0

//==========DEFINIÇÕES DE VARIAVEIS==========

#define btCriarArquivo 2                 //cria pasta no cartao SD quando pressionado
#define btSalvaMedida  3                 //salva a medida que esta sendo lida pelo sensor ao pressionar
#define ledStatus      5                 //led de status se o cartao foi inicializado corretamente 
#define ledBateriaBaixa 6                //led de status da bateria se esta ok

String dataString = "";                  //variavel que recebera o valor do sensor pronto para armazenar
bool flag =            0x00;             //flags auxiliares para botoes 
bool flag2 =           0x00;
int dist = 0, dist_old = 0;
unsigned long timeout = 0;
byte numA =   1;                        //numero da pasta que e incrementado
const int chipSelect = 10;
int endereco = 50;                      //armazena o endereço atual da EEPROM
int entradaTensao = A0;                 //entrada que monitora a bateria
int valorAD = 0;                        //valor analogico na entrada
//==========INSTANCIAÇÃO DE OBJETOS==========
File dataFile;
VL53L0X sensorVL53;

//==========PROTÓTIPO DE FUNÇÕES=============
void novoArquivo();
void medidas();
void leBotoes();
void filtrar_sinal();
void monitoraBateria();

//==========CONFIGURAÇÕES INICIAIS===========
void setup() {
  //====define o modo dos pinos e inicializa====
  pinMode(btCriarArquivo,  INPUT_PULLUP);
  pinMode(btSalvaMedida,   INPUT_PULLUP);
  pinMode(ledStatus,       OUTPUT);
  pinMode(entradaTensao, INPUT);
  pinMode(ledBateriaBaixa, OUTPUT);

  digitalWrite(ledStatus,  HIGH);
  digitalWrite(ledBateriaBaixa, LOW);
  
  //======inicia a serial para debug==========
  Serial.begin(9600);
  while (!Serial) {
    ; 
  }
  Serial.print(F("Inicializando SD card..."));

  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card falhou, ou nao esta presente"));
    while (1);
  }
  Serial.println(F("card inicializado."));

  Wire.begin();

  sensorVL53.init();
  sensorVL53.setTimeout(500);

  // Perfil de longo alcance
  sensorVL53.setSignalRateLimit(0.1);
  sensorVL53.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensorVL53.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  
  digitalWrite(ledStatus, LOW);


  numA = EEPROM.read(endereco)+1;
  
}//END SETUP

//==========LOOP INFINITO===============
void loop() {

  medidas();
  leBotoes();
       
    if(digitalRead(btCriarArquivo) && flag){
      
        numA++;
        
        novoArquivo();
                
          if(numA == 250){
             digitalWrite(ledStatus, HIGH);                    //led sinaliza que criou N maximos de pastas
                for (int i = 0 ; i < EEPROM.length() ; i++) {
                     EEPROM.write(i, 0);
                     delay(5);
                }//end for que limpa todos endereçoes eeprom
                
             digitalWrite(ledStatus, LOW);                 //led apaga após limpar enderços da eeprom
             numA = 1;                                     //reinicializa numA
             EEPROM.write(endereco, numA);                 //grava numA na eeprom
             delay(5);                                     //aguarda tempo minimo de 3 ms para gravar
             EEPROM.read(endereco);                        //le a eeprom
          }//end do tamanho de numA
          

        flag = 0x00;                                       //limpa flag para um novo teste
      }//end criar arquivo

           
      if(digitalRead(btSalvaMedida) && flag2){

                   dataFile = SD.open("med"+String(numA)+".csv", FILE_WRITE);     //abre arquivo atual se nao tem cria
                   dataFile.println(dataString);                                  //grava o valor no documento
                   dataFile.close();                                              //fecha documento
                   Serial.println(F("gravado valor!"));                           //mostra na serial o valor
                   Serial.println(dataString);

                   flag2 = 0x00;                                                  //limpa flag para um novo teste
                   
        }//end salva valor do sensor

        monitoraBateria();
      
  delay(100);    
}//end void loop

//******************DESENVOLVIMENTO DE FUNÇÕES**********************
void medidas(){

  dist = (sensorVL53.readRangeSingleMillimeters()-60);              //faz a leitura da medida em mm do sensor
  filtrar_sinal();                                                  //Filtra o valor de distancia

  dataString = String(dist);                                        //coloca valor numa string
 
  Serial.println(dist);                                             //imprime na serial

}//end medidas

void leBotoes(){
    if(!digitalRead(btCriarArquivo)){
        flag = 0x01;
      }
     
    if(!digitalRead(btSalvaMedida)){
        flag2 = 0x01;
      }
    delay(50);  
}//end le botoes


void novoArquivo(){
    
    EEPROM.write(endereco,numA);
    
    dataFile = SD.open("med"+String(numA)+".csv", FILE_WRITE);
    dataFile.close();
   
    Serial.println(F("Arq criado!"));
    delay(50);

}//end novoArquivo

// Função para filtrar o valor medido
void filtrar_sinal()
{
  // Se a distância medida for maior que 8000 e ainda não tiver passado 1 segundo de timeout
  if (dist > 8000 && ((millis() - timeout) < 1000))
  {
    // Descarta a medição feita e iguala ela à anterior
    dist = dist_old;
  }
  else // Caso contrário (medição < 8000 ou passou do timeout)
  {
    // Não descarta a medição atual e atualiza a medição antiga para a atual
    dist_old = dist;
    timeout = millis(); // Reseta o valor da variável do timeout
  }
}

void monitoraBateria(){
  
  valorAD = analogRead(entradaTensao);

  Serial.print("Valor de tensao: ");
  Serial.println(valorAD);

  if (valorAD < 750){
      digitalWrite(ledBateriaBaixa,HIGH);
      
    }else{
       digitalWrite(ledBateriaBaixa,LOW);
      }

}
