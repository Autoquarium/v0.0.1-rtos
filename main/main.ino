#include <Arduino.h>

// semaphores for subcribed MQTT cmds - USE BINARY SEMAPHORES BECAUSE WE ARE TRIGGERING ANOTHER TASK TO RUN
SemaphoreHandle_t feed_semaphore;
SemaphoreHandle_t led_semaphore;
SemaphoreHandle_t setting_semaphore;

// mutex for CMD_PAYLOAD - USE MUTEX BECAUSE ITS A SHARED RESOURCE - https://stackoverflow.com/questions/62814/difference-between-binary-semaphore-and-mutex
SemaphoreHandle_t payload_mutex;


// MQTT parameters
int publish_interval = 1; // in minutes
char CMD_PAYLOAD[30];


// MAIN TASKS
void keepWifiConnected( void * parameter ){
  // keep track of last wake
  portTickType xLastWakeTime;

  // set delay period (5 seconds)
  portTickType xPeriod = ( 5000 / portTICK_RATE_MS );
  xLastWakeTime = xTaskGetTickCount ();
  
  for(;;){
    vTaskDelayUntil( &xLastWakeTime, xPeriod ); // run every 5 seconds
    Serial.println("checking wifi connection");
  }
}

void checkIncomingCmds( void * parameter ){
  // keep track of last wake
  portTickType xLastWakeTime;

  // set delay period (7 seconds)
  portTickType xPeriod = ( 7000 / portTICK_RATE_MS );
  xLastWakeTime = xTaskGetTickCount ();
  
  for(;;){
    vTaskDelayUntil( &xLastWakeTime, xPeriod ); // run every 5 seconds
    Serial.println("calling wiqtt.loop()");

    // if callback, give semaphore to needed operations:
    //  - move servo
    //  - change LED color/brightness
    //  - setting chnages (autoled, notifications, autofeed) 
  } 
}

void publishSensorVals( void * parameter ) {
  portTickType xLastWakeTime;

  // convert publish internal into ms
  int delay_in_ms = publish_interval * 60 * 1000;
  portTickType xPeriod = ( delay_in_ms / portTICK_RATE_MS );
  xLastWakeTime = xTaskGetTickCount ();
  
  for( ;; ) {
    // Wait for the next cycle.
    vTaskDelayUntil( &xLastWakeTime, xPeriod );
    Serial.println("Publishing new sensor values to broker");
    }
}



// CMD TASKS - these tasks are triggered inside the callback function
void feedCmdTask( void *pvParameters ) {
  for ( ;; ) {
    xSemaphoreTake(feed_semaphore, portMAX_DELAY);
    Serial.println("feed the fish");
    // TODO: add code to move servo
  }
}

void ledCmdTask( void *pvParameters ) {
  for ( ;; ) {
    xSemaphoreTake(led_semaphore, portMAX_DELAY);
    Serial.println("change led color/brightness");
    // TODO: add code to change leds
  }
}

void settingCmdTask( void *pvParameters ) {
  for ( ;; ) {
    xSemaphoreTake(setting_semaphore, portMAX_DELAY);
    Serial.println("change application settings");
    // TODO: add code to change settings
  }
}



/**
 * @brief Called everytime a topic that we are subscribed to publishes data, 
 * calls the appropriate functions to perform the needed actions
 * 
 * @param topic the topic that we are subscribed to 
 * @param payload the actual data receaved
 * @param length the legnth of the data receaved
 */
void callback(char* topic, byte* payload, unsigned int length) {
  
    // save payload to CMD_PAYLOAD
    // need to use a mutex for the payload so that the cmd tasks are not reading while this is writting
    int i = 0;
    xSemaphoreTake(payload_mutex, portMAX_DELAY);
    for (; i < length; i++) {
      CMD_PAYLOAD[i] = (char) payload[i];
    }
    CMD_PAYLOAD[i] = '\0';
    xSemaphoreGive(payload_mutex);
    
    // FEEDING CMDS
    if (!strcmp(topic, "autoq/cmds/feed")) {
      xSemaphoreGive(feed_semaphore);
    }

    // LIGHTING CMDS
    else if (!strcmp(topic, "autoq/cmds/leds")) {
      xSemaphoreGive(led_semaphore);
    }

    // SETTING CHANGES
    else if (!strcmp(topic, "autoq/cmds/settings")) {
      xSemaphoreGive(setting_semaphore); 
    }
    
    else {
        Serial.println("Not a valid topic");
    }
   
    return;
}


void taskCreation() {
  xTaskCreatePinnedToCore(
    publishSensorVals,
    "publish sensor vals",
    10000,
    NULL,
    1, // this task is NOT vital for correct system operation
    NULL,
    1
    );                             

  xTaskCreatePinnedToCore(
    keepWifiConnected,
    "keep wifi connected",
    10000,
    NULL,
    3, // this task is VERY vital for correct system operation
    NULL,
    1
    );                             

  xTaskCreatePinnedToCore(
    checkIncomingCmds,
    "check Incoming MQTT msgs",
    10000,
    NULL,
    2, // this task is vital for correct system operation
    NULL,
    1
    );                             

  xTaskCreatePinnedToCore(
    feedCmdTask,
    "triggers the servo motors",
    10000,
    NULL,
    0, // not time sensitive
    NULL,
    1
    );                             

  xTaskCreatePinnedToCore(
    ledCmdTask,
    "changes LED color",
    10000,
    NULL,
    0, // not time sensitive
    NULL,
    1
    );                             

  xTaskCreatePinnedToCore(
    settingCmdTask,
    "changes settings",
    10000,
    NULL,
    0, // not time sensitive
    NULL,
    1
    );                             
}


void setup() {
  Serial.begin(115200); 
  
  // create semaphores and mutex
  feed_semaphore = xSemaphoreCreateBinary();
  led_semaphore = xSemaphoreCreateBinary();
  setting_semaphore = xSemaphoreCreateBinary();
  payload_mutex = xSemaphoreCreateMutex();

  // create tasks
  Serial.println("Creating Tasks");
  taskCreation();
}

void loop() {
  
}