// WiFi bağlantısı için kütüphaneler
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// Wemos D1 Mini OLED 0.66" shield için kütüphaneler
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 oled(OLED_RESET); // oled isminde yeni bir display oluşturuyoruz

const char* ssid = "Wifi Modem ağ ismi";
const char* password = "wifi şifreniz";

const char* ssid_AP = "wemosd1mini"; // direkt bağlanmak için esp'nin oluşturacağı hotspot ismi
const char* password_AP = "2235";   // direkt bağlanmak için hotspot şifresi

uint8_t i;
bool ConnectionEstablished;

String TimeLine; // Telnet üzeri gönderdiğimiz komutlar bu değişkende saklanacak

#define MAX_TELNET_CLIENTS 2 // maksimum telnet bağlantısı

WiFiServer TelnetServer(23);
WiFiClient TelnetClient[MAX_TELNET_CLIENTS];

//*********************************************************************************************************
// bu fonksiyon ile ESP'den telnet bağlantısı yapan cep telefonu / bilgisayarlara bildiri gönderebiliyoruz
// örneğin TelnetMsg("Pin2 = HIGH , vana açıldı");

void TelnetMsg(String text)
{
  for (i = 0; i < MAX_TELNET_CLIENTS; i++)
  {
    if (TelnetClient[i] || TelnetClient[i].connected())
    {
      TelnetClient[i].println(text);
    }
  }
  delay(10);
}
//**********************************************************************************************************


//*******************************************************************************************
//bu fonksiyon telnet bağlantısı var mı, varsa mesaj gelmişmi bakıyor
//herhangi bir mesaj geldiyse yukarıda deklare ettiğimiz TimeLine değişkenine kayıt ediyor
String Telnet()
{
  String readTelnet;
  for (i = 0; i < MAX_TELNET_CLIENTS; i++)
  {
    if (TelnetClient[i] && !TelnetClient[i].connected())
    {
      Serial.print("Bağlantı koptu oturum kapatıldı "); Serial.println(i + 1);
      TelnetClient[i].stop();
    }
  }
  if (TelnetServer.hasClient())
  {
    ConnectionEstablished = false;

    for (i = 0; i < MAX_TELNET_CLIENTS; i++)
    {
      if (!TelnetClient[i])
      {
        TelnetClient[i] = TelnetServer.available();

        Serial.print("Yeni bir Telnet oturumu açıldı "); Serial.println(i + 1);

        TelnetClient[i].flush();
        TelnetClient[i].println("Hoşgeldiniz!");

        TelnetClient[i].print("ESP çalışalı geçen zaman: ");
        TelnetClient[i].println(millis());

        TelnetClient[i].print("ESP boş RAM: ");
        TelnetClient[i].println(ESP.getFreeHeap());

        TelnetClient[i].println("----------------------------------------------------------------");

        ConnectionEstablished = true;

        break;
      }
      else
      {
        Serial.println("Oturum kullanımda");
      }
    }

    if (ConnectionEstablished == false)
    {
      Serial.println("Boş oturum imkanı yok");
      TelnetServer.available().stop();
      TelnetMsg("Başka bir oturum açılamaz, maksimum oturum sayısına ulaşıldı!");
    }
  }

  for (i = 0; i < MAX_TELNET_CLIENTS; i++)
  {
    if (TelnetClient[i] && TelnetClient[i].connected())
    {
      if (TelnetClient[i].available())
      {
        while (TelnetClient[i].available())
        {
          readTelnet = TelnetClient[i].readString();
        }
      }
    }
  }
  return readTelnet;
}
//*************************************************************************************


//************************************************************************************
// OLED display için kullanımı kolay bir fonksiyon yazdık
// bu fonksiyona aktarılacak parametreler:
// display silinin mi ? 1 = evet
// karakter bütüklüğü 1 veya 2 yazılacak metne göre
// metin rengi = WHITE yada BLACK
// x ve y koordinatları
// yazılacak metin
// örneğin oledWrite(1,2,WHITE,3,3,"Vana acildi");
// Türkçe karakterler düzgün gösterilmiyor
//************************************************************
void oledWrite(bool cleardisplay, byte TextSize, int color, byte x, byte y, String txt)
{
  if (cleardisplay) oled.clearDisplay();
  oled.setTextSize(TextSize);
  oled.setTextColor(color);
  oled.setCursor(x, y);
  oled.println(txt);
  oled.display();
}
//*************************************************************************************

//*************************************************************************************
// buna benzer ismini kendinizin belirleyebileceği fonksiyonlar oluşturup
// telnet üzeri gelen metinlerde istenilen komutlar varsa çağrılabilir
//************************************************************************************
void pin2high()
{
  digitalWrite(2, HIGH);
  // ekrani sil 1 = evet, yazi büyüklügü = 1, pozisyon x=2 , y=2 , renk = beyaz, metin
  oledWrite(1, 1, 2, 2, WHITE, "pin 2 high");
  Serial.println("işlenen komut: pin 2 high");
}
//*************************************************************************************


void pin2low()
{
  digitalWrite(2, LOW);
  oledWrite(1, 1, 2, 2, WHITE, "pin 2 low");
  Serial.println("işlenen komut: pin 2 low");
}

void setup() {
  Serial.begin(9600);

  pinMode(2, OUTPUT); // kullanacağınız giriş çıkışları deklare etmeyi unutmayın

  //*******************************************************************************
  // mevcut wifi ağınıza bağlanmak için modem ismi wifi şifreniz
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(5000); // Wifi bağlantısı için yeterli zaman tanıma
  //*******************************************************************************

  //*******************************************************************************
  // mevcut wifi ağınıza paralel, yada wifi ağı yoksa
  // bu komut ile kendinizin yukarıda belirleyeceği isim ve şifre ile hotspot açma
  // daha sonra cep telefonunuzda mevcut ağlar aratarak direkt ESP'ye bağlanabilirsiniz
  WiFi.softAP(ssid_AP, password_AP);
  //**************************************************************************************

  Serial.print("IP addresi: ");
  Serial.println(WiFi.localIP());
  Serial.print("Soft IP addresi: ");
  Serial.println(WiFi.softAPIP());

  //***************************************************************************************
  // ESP'yi PC'ye bağlamadan COM Port yerine IP numarasını kullanarak kod atabilmeniz için komutlar
  // putty gibi telnet programlarını kullanırken IP numarası yerine WemosD1Mini yada WemosD1mini.local
  // ismini kullanarak bağlanabilirsiniz
  ArduinoOTA.setHostname("WemosD1Mini");
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.begin();
  //******************************************************************************************************

  //*****************************************
  // telnet server başlatılması
  TelnetServer.begin();
  TelnetServer.setNoDelay(true);
  //*********************************************

  // OLED display başlatılıp yukarıda deklare ettiğimiz fonksiyonu kullanarak açılış metnini gösterme
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oledWrite(HIGH, 1, WHITE, 3, 3, "Sistem hazir");
  //*****************************************************************************************************
}

void loop()
{
  ArduinoOTA.handle(); // Over The Air kod yükleme fonksiyonu
  TimeLine = ""; // Telnet üzeri bildiri gelmeden değişkeni boşaltma

  TimeLine = Telnet(); // Telnet() fonksiyonunu çalıştırıp bildiri geldiyse TimeLine değişkeninde saklama

  //*******************************************************************
  // Telnet üzeri metin geldiyse ve ENTER ile >> char(13)<< bitirildiyse
  // gelen metni ...
  if (TimeLine.indexOf(char(13)) > -1)
  {
    TelnetMsg("Alınan komut : " + TimeLine);// Telnet üzeri onay amaçlı geri bildirim 
    Serial.println("Alınan komut: " + TimeLine);// eğer ESP PC'ye bağlı ise serial monitörde tekrarlama
    oledWrite(1, 1, 2, 2, WHITE, "Alinan komut: " + TimeLine);// Eğer Oled display bağlı ise tekrarlama
    if (TimeLine.indexOf("pin2high") > -1) pin2high(); // gelen metin içerisinde pin2high varsa yukarıda deklare edilen pin2high() fonksiyonunu çağırma
    if (TimeLine.indexOf("pin2low") > -1) pin2low(); // gelen metin içerisinde pin2low varsa yukarıda deklare edilen pin2low() fonksiyonunu çağırma
  }
  // bu if sorguları isteğe göre şekillendirilip çoğaltılabilinir
  // if sonucu hangi fonksiyon çağrılacaksa void setup() öncesi örnekte olduğu gibi oluşturulmalı
  //*************************************************************************
}
