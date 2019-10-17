﻿#include "Arduino.h"
#include <ArduinoJson.h>
#include <SmartSensor-hwserial.h>

#define SENSOR_CMD_STREAMON  "{\"header\":\"STREAMON\"};"

#define MAX_OPENCAM_ERROR   7
#define MAX_JSON_ERROR   	7

SmartSensor::SmartSensor()
 : m_id(0), m_type(0),
   m_x(0), m_y(0), m_h(0), m_w(0),
   isCamOpened(false), hasDelayed(false), bSendStreamOn(false),
   nJsonErrCount(0), nOpenCamFailCount(0)
{

}

SmartSensor::~SmartSensor()
{

}

void SmartSensor::serialFlush()
{
	while (Serial.available() > 0)
	    char t = Serial.read();

}

void SmartSensor::begin()
{
    Serial.begin(38400);
    hasDelayed = false;
    isCamOpened = false;
    bSendStreamOn = false;
    nOpenCamFailCount = 0;
    nJsonErrCount = 0;
}

void SmartSensor::end()
{
    Serial.end();
}

bool SmartSensor::openCam()
{
	if (isCamOpened)
        return true;

	if (!hasDelayed)
	{
		delay(5000);
		hasDelayed = true;
	}
	else
	    delay(1000);

	if (!bSendStreamOn)
	{
	    serialFlush();
	    // If it has not received response for the previous streamon cmd yet,
	    // do not send streamon command again.
		Serial.print(SENSOR_CMD_STREAMON);
        //Serial.println("send: STREAMON");
		bSendStreamOn = true;
 	}

    if (Serial.available() > 0)
    {
        bSendStreamOn = false; // After receiving response, reset the flag

        DynamicJsonDocument doc(256);
		String s = Serial.readStringUntil(';');
		//Serial.println(s);
		DeserializationError error = deserializeJson(doc, s);
		if (error)
		{
		    //Serial.println("STREAMON CMD error!!");
		    return false;
		}

		String hdr = doc["header"];
		if (hdr == "CAM_SUCCESS")
		{
		    //Serial.println("STREAMON OK!!");
		    isCamOpened = true;
		    return true;
 		}
		//else
		    //Serial.println("STREAMON Failed!!");
	}
	else
	{
	    nOpenCamFailCount++;
	 	if (nOpenCamFailCount > MAX_OPENCAM_ERROR)
	 	{
	 	    bSendStreamOn = false;
	 	    nOpenCamFailCount = 0;
		}
	}
	return false;
}

bool SmartSensor::isDetected()
{
	bool ret = openCam();
    if (!ret)
    {
   	    //Serial.println("openCam() failed");
        return false;
	}
	//else
	    //Serial.println("openCam() OK");

  	DynamicJsonDocument doc(256);

    if (Serial.available() > 0)
    {
		String s = Serial.readStringUntil(';');
		//Serial.println(s);

		DeserializationError error = deserializeJson(doc, s);
		if (error)
		{
		    nJsonErrCount++;
		    //Serial.print("JSON Error:");
		    //Serial.println(nJsonErrCount);
		    if (nJsonErrCount > MAX_JSON_ERROR)
		    {
		        this->end();
		        delay(50);
		        Serial.begin(115200);
		    	Serial.print("reset\n");
		    	Serial.print("reset\n");
		    	Serial.print("reset\n");
		    	Serial.flush();
    			//Serial.println("send reset");
                delay(50);
                serialFlush();
		        this->end();
		        delay(50);

          		this->begin();
			}

			return false;
		}
		else
		    nJsonErrCount = 0;

		String hdr = doc["hdr"];
        //Serial.println("RECEIVE HDR");
		if (hdr != "DET")
			return false;

		m_id = doc["id"];
		m_type = doc["t"];
		if (m_id <= 0 || m_type <=0)
		    return false;

		m_x = doc["x"];
		m_y = doc["y"];
		m_h = doc["h"];
		m_w = doc["w"];
		return true;
	}
	return false;
}

int SmartSensor::getFuncID()
{
	return m_id;
}

int SmartSensor::getTypeID()
{
	return m_type;
}

int SmartSensor::getPosX()
{
	return m_x;
}

int SmartSensor::getPosY()
{
	return m_y;
}

int SmartSensor::getH()
{
	return m_h;
}

int SmartSensor::getW()
{
	return m_w;
}