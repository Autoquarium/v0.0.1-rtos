// About this class:
// FishMqtt combines the funcinality of the WiFi client and MQTT client, 
// this allows for more things to be done with less code and clutter 
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

class FishMqtt : public PubSubClient {
private:
	// MQTT broker info
	char *mqttServer =  "e948e5ec3f1b48708ce7748bdabab96e.s1.eu.hivemq.cloud";
	int mqttPort = 8883;

	// MQTT client credentials
	char *clientName = "FishClient-666";
	char *usrname = "fishusr";
	char *password = "Fish123!";

	// WiFi credentials
	char wifi_SSID[40];
	char wifi_PWD[40];
	
	// Alert credentials
  	String user_alrt;
  	String API_key = "akiafy9jms26ojnx53bw5vvivj1s4v";
	
	// WiFi client for esp32 chip
	WiFiClientSecure espClient;


public:

	/**
	 * @brief Construct a new Fish Mqtt object
	 * 
	 */
	FishMqtt() : PubSubClient(espClient) {}


	/**
	 * @brief sets s the wifi SSID and password, Dose not begin wifi connection 
	 * 
	 * @param SSID_in The name of the WiFi network to connect to
	 * @param PWD_in The password of the WiFi network to connect to
	 */
	void setWifiCreds(String SSID_in, String PWD_in) {

		if (SSID_in.length() <= 40 && PWD_in.length() <= 40) {
			SSID_in.toCharArray(wifi_SSID, SSID_in.length() + 1);
			PWD_in.toCharArray(wifi_PWD, PWD_in.length() + 1);
		} else {
			Serial.println("[ERROR] Could not set wiFi SSID or password");
		}
	}



	/**
	 * @brief Connects to the WiFi using the credentials provided in the class variables
	 *	Loops until connection is established
	 * 
	 */
	void connectToWifi() {
	  int status = WL_IDLE_STATUS;
	  Serial.print("Connecting to ");
	  
	  WiFi.begin(wifi_SSID, wifi_PWD);
	  Serial.println(wifi_SSID);
	  while (status != WL_CONNECTED) {
	    Serial.print(".");
	    status = WiFi.begin(wifi_SSID, wifi_PWD);
	    delay(10000);
	  }
	  Serial.println(WiFi.RSSI());
	  Serial.println("Connected to WiFi");

	  //needed to bypass verification
	  // TODO: change to something more secure (see here: https://github.com/hometinker12/ESP8266MQTTSSL)
	  espClient.setInsecure(); 
	}


	/**
	 * @brief Checks wifi connection and reconnects if needed
	 * 
	 */
	void checkWificonnection() {
		if (WiFi.status() != WL_CONNECTED) {
			Serial.print("Connection was lost");
			connectToWifi();
		}
	}


	/**
	 * @brief Connects the MQTT broker specified in the setServer() call
	 * If needed, reconnects to the configured WiFi using connectToWifi()
	 *
	 */
	void MQTTreconnect() {
	  Serial.println("Checking MQTT Broker connection");
	  while (!connected()) {
	      Serial.println("Reconnecting to MQTT Broker..");
	      if (connect(clientName, usrname, password)) {
	        Serial.println("Connected to broker.");
	        // subscribe to topic
	        subscribe("autoq/cmds/#"); //subscribes to all the commands messages triggered by the user
	        Serial.println("Subscribed to topic: commands");
	        return;
	      }
	      if(WiFi.status() != WL_CONNECTED) {
	        Serial.println("WiFI disconnected");
	        connectToWifi();
	      }
	  }
	}



	/**
	 * @brief sets the server and calls MQTTrecconnect() to connect to WiFi and the broker
	 *
	 */
	void setupMQTT() {
	  setServer(mqttServer, mqttPort);
	  delay(1500);
	  MQTTreconnect();
	}


	/**
	 * @brief This function serializes the inputted values and then 
	 * publishes the serialized string to the MQTT broker
	 * 
	 * @param tempVal the temperature value to be published
	 * @param pHVal the pH value to be published
	 * @param time the time that the data was published
	 */
   void publishSensorVals(float tempVal, float pHVal, int time) {
  
      // Serialize the sensor data
      DynamicJsonDocument doc(1024);
      doc["pH_val"] = pHVal;
      doc["temp_val"] = tempVal;
      doc["time"] = time;
      String output;
      serializeJson(doc, output);
  
  
      // publish the data to the broker
      if (!connected()) MQTTreconnect();
      publish("autoq/sensor/output", output.c_str()); //need to convert to c_string
   }

	
	
  void setAlertCreds(String User) {
    user_alrt = User;
    
  }


  // "https://api.pushover.net/1/messages.json?token=akiafy9jms26ojnx53bw5vvivj1s4v&user=uaeiijpxfayt5grxg85w97wkeu7gxq&message=testing";
  void sendPushAlert(String msg) {
      Serial.println("sending notifiction");
      HTTPClient http;
	  String url = "https://api.pushover.net/1/messages.json";
      String data_to_send = "token=" + API_key + "&user=" + user_alrt + "&message=" + msg;
      Serial.println(data_to_send);
	  http.begin(espClient, url);  //Specify destination for HTTP request
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      int httpResponseCode = http.POST(data_to_send);
      Serial.println(httpResponseCode);
      http.end();  //Free resources
  }

};
