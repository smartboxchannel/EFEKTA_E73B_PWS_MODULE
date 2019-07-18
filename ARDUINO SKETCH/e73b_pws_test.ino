uint16_t m_s_m;
uint16_t m_s_m2;
uint16_t m_s_m_calc;
boolean flagSendmsm = 0;
float celsius = 0.0;
uint32_t rawTemperature = 0;
uint32_t rawTemperature2 = 0;
uint16_t currentBatteryPercent;
uint16_t batteryVoltage = 0;
uint16_t battery_vcc_min = 2300;
uint16_t battery_vcc_max = 3000;
int16_t linkQuality;

//#define MY_DEBUG
#define MY_DISABLED_SERIAL
#define MY_RADIO_NRF5_ESB
#define MY_RF24_PA_LEVEL (NRF5_PA_MAX)
//#define MY_PASSIVE_NODE
#define MY_NODE_ID 83
#define MY_PARENT_NODE_ID 0
#define MY_PARENT_NODE_IS_STATIC
#define MY_TRANSPORT_UPLINK_CHECK_DISABLED
#define MSM_SENS_ID 1
#define MSM_SENS_C_ID 2
#define TEMP_INT_ID 3
#define SIGNAL_Q_ID 10
#include <MySensors.h>
MyMessage msg_msm(MSM_SENS_ID, V_LEVEL);
MyMessage msg_msm2(MSM_SENS_C_ID, V_LEVEL);
MyMessage msg_temp(TEMP_INT_ID, V_TEMP);

void preHwInit() {
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  pinMode(15, OUTPUT);
  pinMode(5, INPUT);
}

void before()
{
  delay(3000);
  NRF_POWER->DCDCEN = 1;
  NRF_UART0->ENABLE = 0;
  analogReadResolution(12);
  analogReference(AR_VDD4);
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  NRF_TEMP->TASKS_STOP;
  NRF_TEMP->EVENTS_DATARDY = 0;
  NRF_TEMP->INTENSET = 1;
}


void presentation()
{
  sendSketchInfo("PWS GREEN nRF52", "1.01");
  wait(300);
  present(MSM_SENS_ID, S_CUSTOM, "DATA - SOIL MOISTURE");
  wait(300);
  present(MSM_SENS_C_ID, S_CUSTOM, "% - SOIL MOISTURE");
  wait(300);
  present(TEMP_INT_ID, S_TEMP, "TEMPERATURE");
  wait(300);
  present(SIGNAL_Q_ID, S_CUSTOM, "SIGNAL QUALITY");
  wait(300);
}

void setup() {
}

void loop() {
  int_temp();
  digitalWrite(15, HIGH);
  sleep(100);
  digitalWrite(15, LOW);
  msm ();
  digitalWrite(15, HIGH);
  sleep(100);
  digitalWrite(15, LOW);
  wait(50);
  if (flagSendmsm == 1) {
    send(msg_msm2.set(m_s_m_calc), 1);
    wait(3000, 1, 37);
    wait(200);
    send(msg_msm.set(m_s_m), 1);
    wait(3000, 1, 37);
    flagSendmsm = 0;
  }
  wait(200);
  send(msg_temp.set(celsius, 1), 1);
  wait(3000, 1, 0);
  sleep(15000);
  //sleep(2000);
  sendBatteryStatus();
  sleep(21600000); //6h
  //sleep(43200000); //12h
  //sleep(86400000); //24h
  //sleep(20000); //20s
}

void int_temp() {
  for (byte i = 0; i < 10; i++) {
    NRF_TEMP->TASKS_START = 1;
    while (!(NRF_TEMP->EVENTS_DATARDY)) {}
    rawTemperature = NRF_TEMP->TEMP;
    rawTemperature2 = rawTemperature2 + rawTemperature;
    wait(10);
  }
  celsius = ((((float)rawTemperature2) / 10) / 4.0);
  rawTemperature2 = 0;

}

void msm () {
  digitalWrite(6, LOW);
  wait(500);
  for (byte i = 0; i < 10; i++) {
    m_s_m = analogRead(5);
    m_s_m2 = m_s_m2 + m_s_m;
    wait(50);
  }
  m_s_m = m_s_m2 / 10;
  m_s_m2 = 0;
  digitalWrite(6, HIGH);
  wait(50);
  if(m_s_m >3000){
    m_s_m = 3000;
  }
  if(m_s_m <1100){
    m_s_m = 1100;
  }
  m_s_m_calc = map(m_s_m, 3000, 1100, 0, 100);
  flagSendmsm = 1;
}

void sendBatteryStatus() {
  wait(100);
  batteryVoltage = hwCPUVoltage();
  wait(20);

  if (batteryVoltage > battery_vcc_max) {
    currentBatteryPercent = 100;
  }
  else if (batteryVoltage < battery_vcc_min) {
    currentBatteryPercent = 0;
  } else {
    currentBatteryPercent = (100 * (batteryVoltage - battery_vcc_min)) / (battery_vcc_max - battery_vcc_min);
  }
  sendBatteryLevel(currentBatteryPercent, 1);
  wait(3000, C_INTERNAL, I_BATTERY_LEVEL);

  linkQuality = calculationRxQuality();
  wait(50);
  sendSignalStrength(linkQuality, 1);
  wait(2000, 1, V_VAR1);
}



//****************************** very experimental *******************************


bool sendSignalStrength(const int16_t level, const bool ack)
{
  return _sendRoute(build(_msgTmp, GATEWAY_ADDRESS, SIGNAL_Q_ID, C_SET, V_VAR1,
                          ack).set(level));
}
int16_t calculationRxQuality() {
  int16_t nRFRSSI_temp = transportGetReceivingRSSI();
  int16_t nRFRSSI = map(nRFRSSI_temp, -85, -40, 0, 100);
  if (nRFRSSI < 0) {
    nRFRSSI = 0;
  }
  if (nRFRSSI > 100) {
    nRFRSSI = 100;
  }
  return nRFRSSI;
}

//****************************** very experimental *******************************
