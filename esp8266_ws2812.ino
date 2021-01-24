//WiFi library
#include <ESP8266WiFi.h>
//MQTT library
#include <PubSubClient.h>
//LED strip library
#include <NeoPixelBus.h>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>
#include <NeoAnimationFX.h>
//EEPROM library
#include <EEPROM.h>
//Over the air update libraries
#include <ESP8266WebServer.h>
#include <ElegantOTA.h>
//Library for create a simple web page
#include "PageBuilder.h"
//Sources of the web page
#include "WebPage.h"


//====================== CONSTANTS FOR NETWORK CONNECTION ==========================================
//WiFi connection parameters
const char*					_wifi_ssid						= "XXXXXX";
const char*					_wifi_pwd							= "XXXXXX";
IPAddress						_wifi_gateway(192, 168, 1, 1);
IPAddress						_wifi_subnet(255, 255, 255, 0);

//Method for checking if a connection exists and trying to establish a connection
bool								_wifi_check();
bool								_wifi_needConfigure		= true;
bool								_wifi_firstConnect		= true;
bool								_wifi_prevState				= false;

//Web server for over-the-air update functionality
ESP8266WebServer		_ota_webServer(81);
//Web server for the settings page
ESP8266WebServer		_conf_webServer(80);
//==================================================================================================

//============================ MQTT CLIENT =========================================================
WiFiClient					_mqtt_wifi_client;
PubSubClient				_mqtt_client(_mqtt_wifi_client);
void								_mqtt_configure_topics();
void								_mqtt_callback(char* topic, byte* payload, unsigned int length);
//MQTT Connection Timer - creates a delay between attempts to connect to the server
uint32_t						_mqtt_timer						= 0;
//MQTT Server Disconnect Counter - for network resiliency analysis
uint32_t						_mqtt_disconCntr			= 0;
//==================================================================================================

//====================== DEVICE PARAMETERS =========================================================
struct	__attribute__((__packed__))		config_t
{
	uint8_t						crc;
	
	struct
	{
		IPAddress					ip;
		char							alias[32];
	}network;
	
	struct
	{
		uint16_t					count;
		uint16_t					speed;
		uint8_t						brightness;
		uint8_t						effect;
		uint8_t						pin;
	}leds;
	
	struct
	{
		uint8_t						pin;
	}pir;

	struct
	{
		IPAddress					ip;
		uint16_t					port;
		char							user[32];
		char							pwd[32];
	}mqtt;
	
};
//Current device configuration
config_t						_conf;
bool								_conf_restore();
void								_conf_save();
//==================================================================================================

//============================ LED STRIP CONTROL ===================================================
typedef							NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod>		NEOMETHOD; //uses GPIO3/RX
NEOMETHOD						_ws8212(512, 2); // PIN is ignored for ESP8266
NeoAnimationFX<NEOMETHOD>		_ws8212_drv(_ws8212);

//LED strip driver initialization
void								_ws8212_drv_init();
//Applying values ​​to LEDs - called from the WS2812FX driver
uint16_t						_ws8212_drv_setVal();
//==================================================================================================

//============================ PIR SENSOR CONTROL ==================================================
//Current state of the PIR sensor
uint8_t							_pir_state						= 0;
//Previous state of the PIR sensor
uint8_t							_pir_prev							= 0;
//Timer events from the PIR sensor, so that events do not occur too often when pointing at the sensor
uint64_t            _pir_timer            = 0;
//==================================================================================================


//============================ ADDITIONAL METHODS ==================================================
String	ipAddr2String(const IPAddress& ipAddress)
{
	return	String(ipAddress[0]) + String(".") + String(ipAddress[1]) + String(".") +\
					String(ipAddress[2]) + String(".") + String(ipAddress[3]); 
}
//==================================================================================================


//============================ WEB PAGE CONTROL ====================================================
String	get_network_ip(PageArgument& args)					{return ipAddr2String(_conf.network.ip);}
String	get_network_alias(PageArgument& args)				{return String(_conf.network.alias);}
String	get_leds_count(PageArgument& args)					{return String(_conf.leds.count);}
String	get_leds_speed(PageArgument& args)					{return String(_conf.leds.speed);}
String	get_leds_pin(PageArgument& args)						{return String(_conf.leds.pin);}
String	get_pir_pin(PageArgument& args)							{return String(_conf.pir.pin);}
String	get_mqtt_ip(PageArgument& args)							{return ipAddr2String(_conf.mqtt.ip);}
String	get_mqtt_port(PageArgument& args)						{return String(_conf.mqtt.port);}
String	get_mqtt_user(PageArgument& args)						{return String(_conf.mqtt.user);}
String	get_mqtt_pwd(PageArgument& args)						{return String(_conf.mqtt.pwd);}

PageElement MAIN_PAGE_ELEMENT(_WEB_PAGE,
															{
																{"NETWORK.IP", get_network_ip},
																{"NETWORK.ALIAS", get_network_alias},
																{"LEDS.COUNT", get_leds_count},
																{"LEDS.SPEED", get_leds_speed},
																{"LEDS.PIN", get_leds_pin},
																{"PIR.PIN", get_pir_pin},
																{"MQTT.IP", get_mqtt_ip},
																{"MQTT.PORT", get_mqtt_port},
																{"MQTT.USER", get_mqtt_user},
																{"MQTT.PWD", get_mqtt_pwd},
															});
PageBuilder MAIN_PAGE("/", {MAIN_PAGE_ELEMENT});


String	http_save_config(PageArgument& args)
{
	Serial.println("Save configuration");
	
	//Parsing the passed parameters
	String	parameter			= args.arg("network.ip");
	Serial.println(parameter);
	_conf.network.ip.fromString(parameter.c_str());
	
	parameter							= args.arg("network.alias");
	Serial.println(parameter);
	strncpy(_conf.network.alias, parameter.c_str(), sizeof(_conf.network.alias));
	
	parameter							= args.arg("leds.count");
	Serial.println(parameter);
	_conf.leds.count			= parameter.toInt();
	
	parameter							= args.arg("leds.speed");
	Serial.println(parameter);
	_conf.leds.speed			= parameter.toInt();
	
	parameter							= args.arg("leds.pin");
	Serial.println(parameter);
	_conf.leds.pin				= parameter.toInt();
	
	parameter							= args.arg("pir.pin");
	Serial.println(parameter);
	_conf.pir.pin					= parameter.toInt();

	parameter							= args.arg("mqtt.ip");
	Serial.println(parameter);
	_conf.mqtt.ip.fromString(parameter.c_str());

	parameter							= args.arg("mqtt.port");
	Serial.println(parameter);
	_conf.mqtt.port				= parameter.toInt();

	parameter							= args.arg("mqtt.user");
	Serial.println(parameter);
	strncpy(_conf.mqtt.user, parameter.c_str(), sizeof(_conf.mqtt.user));

	parameter							= args.arg("mqtt.pwd");
	Serial.println(parameter);
	strncpy(_conf.mqtt.pwd, parameter.c_str(), sizeof(_conf.mqtt.pwd));
	
	//Saving the configuration
	_conf_save();
	_wifi_needConfigure		= true;
	return String("");
}
PageElement	SAVE_CONFIG_ELEMENT("<html></html>", {{"", http_save_config}});
PageBuilder SAVE_CONFIG_PAGE("/config", {SAVE_CONFIG_ELEMENT});

String	http_leds_on(PageArgument& args)
{
	Serial.println("LEDs ON");
	_ws8212_drv.setBrightness(150);
	return String("");
}
PageElement	LEDS_ON_ELEMENT("<html></html>", {{"", http_leds_on}});
PageBuilder LEDS_ON_PAGE("/leds_on", {LEDS_ON_ELEMENT});

String	http_leds_off(PageArgument& args)
{
	Serial.println("LEDs OFF");
	_ws8212_drv.setBrightness(0);
	return String("");
}
PageElement	LEDS_OFF_ELEMENT("<html></html>", {{"", http_leds_off}});
PageBuilder LEDS_OFF_PAGE("/leds_off", {LEDS_OFF_ELEMENT});

String  http_restart(PageArgument& args)
{
  Serial.println("RESTART");
  ESP.restart();
  return String("");
}
PageElement RESTART_ELEMENT("<html></html>", {{"", http_restart}});
PageBuilder RESTART_PAGE("/restart", {RESTART_ELEMENT});
//==================================================================================================







void setup() {
	//Initialize the terminal
	Serial.begin(115200);
	delay(5);
	Serial.println();
	randomSeed(micros());
	//Display a message about the start of the system
	Serial.println("---------------------------------------------------");
	Serial.println("Starting the LED strip controller");

	//Loading the configuration
	Serial.printf("The size of the structure with the configuration:		%d.\n",	sizeof(config_t));
	if (_conf_restore() == false)
	{	Serial.println("Error loading parameters - I apply from the code");
		_conf.network.ip					= IPAddress(0, 0, 0, 0);
		String	alias							= "ESP8266_led_cntrl_" + String(random(0xffff), HEX);
		strcpy(_conf.network.alias, alias.c_str());
		_conf.leds.count					= 32;
		_conf.leds.speed					= 100;
		_conf.leds.brightness			= 250;
		_conf.leds.effect					= 0;
		_conf.leds.pin						= 2;
		_conf.pir.pin							= 14;
		_conf.mqtt.ip							= IPAddress(0, 0, 0, 0);
		_conf.mqtt.port						= 0;
		memset(_conf.mqtt.user, 0, sizeof(_conf.mqtt.user));
		memset(_conf.mqtt.pwd, 0, sizeof(_conf.mqtt.pwd));
		Serial.println("Save the parameters to the EEPROM");
		_conf_save();
	}

	//Output the configuration to the terminal
	Serial.println("The parameters are set:");
	Serial.printf("	Number of LEDs:		%d.\n", _conf.leds.count);
	Serial.printf("	Speed of effects:			%d.\n", _conf.leds.speed);
	Serial.printf("	Brightness:				%d.\n", _conf.leds.brightness);
	Serial.printf("	The effect of normal mode:		%d.\n", _conf.leds.effect);
	Serial.printf("	Alias:				%s.\n", _conf.network.alias);
	Serial.printf("	PIN of LEDs:			%d.\n", _conf.leds.pin);
	Serial.printf("	Pin of the PIR sensor:			%d.\n", _conf.pir.pin);	
	
	//Initializing the LED strip
	_ws8212_drv.init();
	_ws8212_drv.setColor(100, 100, 100);
	_ws8212_drv.setLength(_conf.leds.count);
	_ws8212_drv.setBrightness(_conf.leds.brightness);
	_ws8212_drv.setSpeed(_conf.leds.speed);
	_ws8212_drv.setMode(_conf.leds.effect);
	_ws8212_drv.start();
	//If the PIR sensor is enabled, then initialize the input for it
	pinMode(_conf.pir.pin, INPUT);

  MAIN_PAGE.insert(_conf_webServer);
  SAVE_CONFIG_PAGE.insert(_conf_webServer);
  LEDS_ON_PAGE.insert(_conf_webServer);
  LEDS_OFF_PAGE.insert(_conf_webServer);
  RESTART_PAGE.insert(_conf_webServer);
  _conf_webServer.begin();

  _ota_webServer.on("/", []()
	{	_ota_webServer.send(200, "text/plain", "To update firmware: ipaddr/update.");});
	ElegantOTA.begin(&_ota_webServer);
	_ota_webServer.begin();
}


//==================================================================================================
//====================== MAIN CYCLE ================================================================
void loop() {

	String	alias(_conf.network.alias);

	//======= WIFI CONNECTION CONTROL ==================================================
	//Get the WiFi state, if there is no connection, then we will try to connect
	bool	wifi_state			= _wifi_check();
	if ((wifi_state == true) && (_wifi_prevState == false))
	{
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
	}
	_wifi_prevState				= wifi_state;
	//==================================================================================


	//======= GENERATING A PIR SENSOR STATE CHANGE EVENT ===============================
	//Check the status of the PIR sensor, if it has changed - we update the information in the topic
	_pir_state						= digitalRead(_conf.pir.pin);
  uint64_t  passed      = millis() - _pir_timer;
	if (_pir_state != _pir_prev)
	  if ((_pir_state == true) || (passed > 10000))
  	{	//The state has changed - we record the new state
  		Serial.printf("PIR event: %d.\n", _pir_state);
  		if (_pir_state == false)		_mqtt_client.publish_P(String(alias + "/state/pir").c_str(), "0", true);
  		else												_mqtt_client.publish_P(String(alias + "/state/pir").c_str(), "1", true);
      _pir_prev           = _pir_state;
      _pir_timer          = millis();
  	}
  if (_pir_state == true)   _pir_timer  = millis();
	//==================================================================================


	//======= ADJUSTING THE BRIGHTNESS OF THE LED STRIP ================================
	//If there is no connection to the network or server, then turn off the lights
	if (wifi_state == false)		_conf.leds.brightness			= 0;
	//Adjust the brightness of the strip
	uint16_t	value				= _ws8212_drv.getBrightness();
	if (value != _conf.leds.brightness)
	{		
		if (value > _conf.leds.brightness)		value	-= 1;
		else																	value	+= 5;
		value								= (value > 255) ? 255 : value;
		_ws8212_drv.setBrightness(value);
		_mqtt_client.publish_P(String(alias + "/state/brightness").c_str(), String(value).c_str(), true);
		
		if (_ws8212_drv.getBrightness() == _conf.leds.brightness)
		{
			Serial.printf("Brightness is set: %d.\n", _conf.leds.brightness);
			_mqtt_client.publish_P(String(alias + "/state/brightness").c_str(), String(_conf.leds.brightness).c_str(), true);
		}
	}
	//==================================================================================

	_conf_webServer.handleClient();
	_ota_webServer.handleClient();
	if (_mqtt_client.connected())		_mqtt_client.loop();

	_ws8212_drv.service();
	yield();
}
//==================================================================================================
//==================================================================================================




//====================== УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ ПО WIFI ===========================================
//Метод проверки наличия подключения и попытки установки соединения
bool	_wifi_check()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		if (_wifi_needConfigure)
		{
			Serial.println("Конфигурируем WiFi");
			WiFi.hostname(_conf.network.alias);
			if (_conf.network.ip != IPAddress(0, 0, 0, 0))
			{
				Serial.println("Применяю статический IP");
				WiFi.config(_conf.network.ip, _wifi_subnet, _wifi_gateway);
			}else	Serial.println("Адрес устройства будет назначен роутером");

			WiFi.begin(_wifi_ssid, _wifi_pwd);
			
			Serial.println("Конфигурируем MQTT");
			_mqtt_client.setServer(_conf.mqtt.ip, _conf.mqtt.port);
			_mqtt_client.setCallback(_mqtt_callback);
			
			_wifi_needConfigure		= false;
		}
		delay(5);
		if (WiFi.status() != WL_CONNECTED)		return false;
	}
	if (WiFi.status() == WL_CONNECTED)
	{
		if ((!_mqtt_client.connected()) && (_mqtt_timer == 0))
		{
			Serial.println("Пытаюсь подключиться к серверу MQTT");
			_mqtt_client.connect(_conf.network.alias, _conf.mqtt.user, _conf.mqtt.pwd);
			if (_mqtt_client.connected())
			{
				Serial.println("Подключился к серверу MQTT");
				_mqtt_disconCntr++;
				_mqtt_configure_topics();
			}else	Serial.println("Не получилось подключиться серверу MQTT");
			_mqtt_timer					= 200;
		}
		if (!_mqtt_client.connected())		_mqtt_timer--;
		else															_mqtt_timer		= 200;
	}
	
	if ((WiFi.status() == WL_CONNECTED) && _mqtt_client.connected())		return true;
	else																																return false;
};
//==================================================================================================


//============================ КЛИЕНТ MQTT =========================================================
void	_mqtt_configure_topics()
{
	String	alias(_conf.network.alias);
	_mqtt_client.publish_P(String(alias + "/ctrl/brightness").c_str(), String(_conf.leds.brightness).c_str(), true);
	_mqtt_client.subscribe(String(alias + "/ctrl/brightness").c_str());
	_mqtt_client.publish_P(String(alias + "/ctrl/effect").c_str(), String(_conf.leds.effect).c_str(), true);
	_mqtt_client.subscribe(String(alias + "/ctrl/effect").c_str());

	_mqtt_client.publish_P(String(alias + "/state/pir").c_str(), "0", true);
	_mqtt_client.publish_P(String(alias + "/state/brightness").c_str(), String(_ws8212_drv.getBrightness()).c_str(), true);
	_mqtt_client.publish_P(String(alias + "/state/effect").c_str(), String(_conf.leds.effect).c_str(), true);
	long	rssi		= WiFi.RSSI();
	_mqtt_client.publish_P(String(alias + "/state/rssi").c_str(), String(rssi).c_str(), true);
	Serial.println("Опубликованы и подключены топики MQTT");
};

void	_mqtt_callback(char* topic, byte* payload, unsigned int length)
{
	Serial.print("Message arrived in topic: ");
	Serial.println(topic);
	Serial.print("Message:");
	for (int i = 0; i < length; i++)	Serial.print((char)payload[i]);
	Serial.println("");

	payload[length]							= '\0';
	String	strTopic						= String((char*)topic);
	String	alias(_conf.network.alias);
	
	if (strTopic == String(alias + "/ctrl/brightness"))
	{	_conf.leds.brightness			= String((char*)payload).toInt();
		return;
	}
	if (strTopic == String(alias + "/ctrl/effect"))
	{	_conf.leds.effect					= String((char*)payload).toInt();
		_ws8212_drv.setMode(_conf.leds.effect);
		_mqtt_client.publish_P(String(alias + "/state/effect").c_str(), String(_conf.leds.effect).c_str(), true);
		return;
	}
};
//==================================================================================================


//====================== ПАРАМЕТРЫ УСТРОЙСТВА ======================================================
uint8_t	_conf_crc()
{
	uint8_t		crc					= 0;
	for (uint16_t idx = 1; idx < sizeof(config_t); idx++)
	{	crc			+= ((uint8_t*)&_conf)[idx];
	}
	if (crc == 0)		crc			= 1;
	return crc;
}

bool	_conf_restore()
{
	EEPROM.begin(512);
	for (uint16_t idx = 0; idx < sizeof(config_t); idx++)
	{	((uint8_t*)&_conf)[idx]		= EEPROM.read(idx);
		yield();
	}
	if (_conf_crc() != _conf.crc)		return false;
	else														return true;
};

void	_conf_save()
{
	_conf.crc		= _conf_crc();
	for (uint16_t idx = 0; idx < sizeof(config_t); idx++)
	{	EEPROM.put(idx, ((uint8_t*)&_conf)[idx]);
		EEPROM.commit();
	}
};
//==================================================================================================
