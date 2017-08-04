#include <Pozyx.h>
#include <Pozyx_definitions.h>
#include <Wire.h>
#include <ros.h>
#include <sensor_msgs/NavSatFix.h>
#include <common.h>

/*void getgps(const sensor_msgs::NavSatFix& gps_msg)
  if(gps_msg.data > 1.0)
    digitalWrite(13, HIGH-digitalRead(13));   // blink the led
}*/

uint16_t source_id;                 // the network id of this device
uint16_t chat_id = 0;             //Broadcast the message
int status;

uint8_t ranging_protocol = POZYX_RANGE_PROTOCOL_PRECISION; // ranging protocol of the Pozyx.
/* const int num_cars = 2; //Amount of other cars */
const int num_cars = 1; //Amount of other cars
/* uint16_t car_ids[num_cars] = {0x6806,0x6827}; //Default is car0 sender so only range car1,car2 receivers */
uint16_t car_ids[num_cars] = {0x6806}; //Default is car0 sender so only range car1,car2 receivers

void setup_uwb()
{
    UWB_settings_t uwb_settings;
    Pozyx.getUWBSettings(&uwb_settings);
    uwb_settings.bitrate = 2;
    uwb_settings.plen = 0x24;
    Pozyx.setUWBSettings(&uwb_settings);
}

void setup(){
  Serial.begin(57600);
  // initialize Pozyx
  if(Pozyx.begin() == POZYX_FAILURE){
    Serial.println("ERROR: Unable to connect to POZYX shield");
    Serial.println("Reset required");
    delay(100);
    abort();
  }
    /* delay(1000); */
    //setup_uwb();
  // read the network id of this device
  /* Pozyx.setOperationMode(POZYX_ANCHOR_MODE); */
  /* delay(1000); */
  Pozyx.regRead(POZYX_NETWORK_ID, (uint8_t*)&source_id, 2);
  if (String(source_id,HEX) == "6867"){ //This is car1 sender
    car_ids[0] = 0x6802; //so only range car0 receiver
    /* car_ids[1] = 0x6827; //and car2 reciever */
  }
  else if (String(source_id,HEX) == "685b"){ //This is car2 sender
    car_ids[0] = 0x6802; //so only range car0
    car_ids[1] = 0x6806; //and car1
  }
}

void discover()
{
    Pozyx.clearDevices();
    int status = Pozyx.doDiscovery(POZYX_DISCOVERY_ANCHORS_ONLY);
    if (status == POZYX_SUCCESS)
    {
        uint8_t n_devs = 0;
        status = 0;
        status = Pozyx.getDeviceListSize(&n_devs);
        if (n_devs > 0)
        {
            uint16_t devices[n_devs];
            Pozyx.getDeviceIds(devices, n_devs);
            for (int i = 0; i < n_devs; i++)
            {
                Serial.println(String(devices[i], HEX));
            }
        }
    }
    else
    {
    }
}

void loop()
{
    /* discover(); */
    send_message();
}

void send_message() {
    int car_data_size = sizeof(dq_range);
    size_t total_car_data = sizeof(dq_header) + num_cars * car_data_size;
    uint8_t buffer[total_car_data];
    dq_header header = {0, num_cars};

    // writes message header
    memcpy(buffer, &header, sizeof(dq_header));

    for (int i = 0; i < num_cars; i++) {
        if (String(source_id,HEX) != String(car_ids[i],HEX)) {
            device_range_t range;
            status = 0;
            while (status != POZYX_SUCCESS) {
                status = Pozyx.doRanging(car_ids[i], &range);
                if (status == POZYX_SUCCESS) {
                    if (range.distance > 0)
                    {
                        dq_range rng = {car_ids[i], range.distance};
                        /* dq_range rng = {31, range.distance}; */
                        memcpy(buffer + i * car_data_size + sizeof(dq_header),
                            &rng, sizeof(dq_range));
                        Serial.println(rng.dist);
                        break;
                    }
                }
            }
        }
    }

    status = Pozyx.writeTXBufferData(buffer, total_car_data);
    // broadcast the contents of the TX buffer
    status = Pozyx.sendTXBufferData(chat_id);
    delay(1);
}
