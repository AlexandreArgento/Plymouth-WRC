#include "XBus.h"
#include <Wire.h>

XBus::XBus(uint8_t address) : address(address),wokeUp(false){
	for(int i = 0; i < 4; ++i){
		if(i < 3){
			accel[i] =0;
			rot[i] =0;
			mag[i] =0;
		}
		quat[i] = 0;
	}
}

void XBus::dataswapendian(uint8_t* data, uint8_t length){
	uint8_t cpy[length];
	memcpy(cpy,data,length);
	for(int i = 0; i < length/4; i++){
		for(int j = 0; j < 4; j++){
			data[j+i*4] = cpy[3-j+i*4];
		}
	}
}

void XBus::parseData(uint8_t* data, uint8_t datalength){
	if(datalength < 2)
	return;
	
	uint8_t actualMID = data[0];
	if(actualMID == 0x10 || actualMID == 0x20 || actualMID == 0x40 || actualMID == 0x80 || actualMID == 0xC0 || actualMID == 0xE0){
		uint8_t length = data[2];
		switch(((uint16_t)data[1] |((uint16_t)data[0]<<8)) & (uint16_t)0xFFF0){
		case (uint16_t)DataID::QUAT:
			dataswapendian(data+3, sizeof(float)*4);
			memcpy(quat, data+3, sizeof(float)*4);
			break;
		case (uint16_t)DataID::ACCEL:
			dataswapendian(data+3, 3*sizeof(float));
			memcpy(accel, data+3, sizeof(float)*3);
			break;
		case (uint16_t)DataID::MAG:
			dataswapendian(data+3, 3*sizeof(float));
			memcpy(mag, data+3, sizeof(float)*3);
			break;
		case (uint16_t)DataID::ROT:
			dataswapendian(data+3, 3*sizeof(float));
			memcpy(rot, data+3, sizeof(float)*3);
			break;
		case (uint16_t)DataID::DV:
			dataswapendian(data+3, 3*sizeof(float));
			memcpy(dv, data+3, sizeof(float)*3);
			break;
		}
		parseData(data+3+length, datalength - length - 3);
	}else{
		uint8_t length = data[1];
		if(actualMID == (uint8_t)MesID::DATA || actualMID == (uint8_t)MesID::DATA2)
		parseData(data+2, length);
	}
}

void XBus::readPipeStatus(){
	Wire.beginTransmission(address);
	Wire.write(XSENS_PIPE_STATUS);
	Wire.endTransmission();

	Wire.requestFrom(address,(uint8_t)4);
	if(Wire.available()>0) {
		for(int i = 0; i < 4; i++){
			data[i] = Wire.read();
			}
	}
	notificationSize = (uint16_t)data[0] | ((uint16_t)data[1]<<8);
	measurementSize = (uint16_t)data[2] | ((uint16_t)data[3]<<8);
}
void XBus::readPipeNotif(){
	if(notificationSize > 0){
		wokeUp = true;
		Wire.beginTransmission(address);
		Wire.write(XSENS_NOTIF_PIPE);
		Wire.endTransmission();

		Wire.requestFrom(address,notificationSize);
		if(Wire.available()>0) {
			for(int i = 0; i < notificationSize; ++i){
				datanotif[i] = Wire.read();
			}
		}
	}
}
void XBus::readPipeMeas(){
	if(measurementSize > 0){
		wokeUp = true;
		Wire.beginTransmission(address);
		Wire.write(XSENS_MEAS_PIPE);
		Wire.endTransmission();
		
		Wire.requestFrom(address,measurementSize);
		if(Wire.available()>0) {
			for(int i = 0; i < measurementSize; ++i){
				datameas[i] = Wire.read();
			}
		}
	}
}
bool XBus::readUntilAck(uint8_t ACK){
	unsigned long t_now = millis();
	bool res = false;
	while(datanotif[0] != ACK && millis()-t_now < 5000){
		readPipeStatus();
		readPipeNotif();
		res = datanotif[0] == ACK;
		delay(1);
	}
	return res;
}

uint8_t* XBus::buildMessage(MesID MID, uint8_t* data, uint8_t length){
	uint8_t* res;
	res = new uint8_t[length+4];
	res[0] = XSENS_CONTROL_PIPE;
	res[1] = (uint8_t)MID;
	res[2] = length;
	
	if(length > 0 && data != NULL)
		memcpy(res+3, data, length);
	
	uint8_t checksum = 0x00;
	for(int i = 1; i< length+3; i++)
		checksum -= res[i];
	checksum -= 0xFF;
	res[length+3] = checksum;
	
	return res;
}

void XBus::quatToHeading(){
	headingYaw = atan2(2*quat[0]*quat[3]+quat[1]*quat[2], 1-2*(quat[2]*quat[2]+quat[3]*quat[3]));
}

void XBus::setLatLongAlt(float lat, float longitude, float alt){
	  
	uint8_t* dataMes = new uint8_t[24];
	goToConfig();
	
	  flt.f = lat;
	  da.p.s = flt.p.s;
	  da.p.e = flt.p.e-127 +1023;  // exponent differ
	  da.p.m = flt.p.m;
	  for(int i = 0; i<8; ++i)
		dataMes[i]=da.b[7-i];
	
	  flt.f = longitude;
	  da.p.s = flt.p.s;
	  da.p.e = flt.p.e-127 +1023;  // exponent differ
	  da.p.m = flt.p.m;
	  for(int i = 0; i<8; ++i)
		dataMes[i+8]=da.b[7-i];
	
	  flt.f = alt;
	  da.p.s = flt.p.s;
	  da.p.e = flt.p.e-127 +1023;  // exponent differ
	  da.p.m = flt.p.m;
	  for(int i = 0; i<8; ++i)
		dataMes[i+16]=da.b[7-i];
	
	uint8_t* dataS = buildMessage(SETLATLONG,dataMes,24);
	
	Serial.println("writing latlong");	
	Wire.beginTransmission(address);
	Wire.write((char*)dataS,28);
	Wire.endTransmission();
	
	readUntilAck(0x6F);
	
	delete dataS;
	delete dataMes;
	
	goToMeas();
	
}

void XBus::goToConfig(){
	
	uint8_t* dataS = buildMessage(GOTOCONFIG,NULL,0);
	
	Serial.println("sending config");	
	Wire.beginTransmission(address);
	Wire.write((char*)dataS,4);
	Wire.endTransmission();
	
	readUntilAck(0x31);
	
	delete dataS;
}

void XBus::goToMeas(){
	uint8_t* dataS = buildMessage(GOTOMEAS,NULL,0);
	
	Serial.println("sending Meas");	
	Wire.beginTransmission(address);
	Wire.write((char*)dataS,4);
	Wire.endTransmission();
	
	readUntilAck(0x11);
	
	delete dataS;
}

void XBus::read(){
	readPipeStatus();
	readPipeNotif();
	readPipeMeas();
	parseData(datameas, measurementSize);
	quatToHeading();
}