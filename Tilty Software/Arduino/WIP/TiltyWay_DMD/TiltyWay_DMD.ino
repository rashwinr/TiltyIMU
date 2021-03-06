// Settings file
#import "settings.h"

// Wire library
#include <i2c_t3.h>

// PID library
#include <PID.h>

// IMU sensor libraries
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

// Dual Motor Driver Library
#include <DualMotorDriver.h>

// Global variables file
#import "Global_variables.h"

#include <EEPROM.h>
#include "DebugUtils.h"
#include "CommunicationUtils.h"
#include "FreeIMU.h"

FreeIMU fIMU = FreeIMU();

void setup() {
	Wire.begin(I2C_MASTER, 0, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);

	//while (!Serial) {}
	
	delay(5);
	imu = MPU6050();
	imu.reset();
	delay(5);
	if (imu.init() == false) {	SOS(0, "\tIMU not detected");}
	setupDMP();
	
	setupPID();
	
	//attachInterrupt(RESET_PIN, reset, RISING);
	
	//while (true) {	testIMU();}
	
	checkToStart();
}

void loop() {
	//if (readDMP()) {
	if (readCombinedYPR(ypr)) {
		/*
		fIMU.getYawPitchRoll(ypr);
		yaw = ypr[YAW] - ypr_offset[YAW];
		pitch = ypr[PITCH] - ypr_offset[PITCH];
		roll = ypr[ROLL] - ypr_offset[ROLL];
		*/
		printYPR();
		tiltPID.update();
		printPID();
		//printPID();
		checkToRun();
		delay(5);
	}
}



void checkToRun() {
	// Check hard cutoff limits
	//if (abs(pitch) >= MAX_PITCH || abs(roll) >= MAX_ROLL) {	SOS(4, "\tExceeded maximum pitch/roll");} // Check pitch and roll
	//else if (tiltPID.limited() && tilt_power != 0) {	SOS(5, "\tExceeded max speed/power");} // Check power/speed
	if (abs(pitch) >= MAX_PITCH || abs(roll) >= MAX_ROLL) {	motors.setMotors(0,0);}
	else {	motors.setMotors(int16_t(tilt_power), int16_t(tilt_power));}
}


void checkToStart() {
	int how_many = 2; // The number of conditions that must be met at once to start main balancing code
	byte conditions = 0b00000000;
	unsigned long startup_timer = 0;
	
	while (conditions != pow(2, how_many) - 1) {
		//if (readDMP()) {
		if (readCombinedYPR(ypr)) {
			motors.setMotors(0, 0);
			/*
			fIMU.getYawPitchRoll(ypr);
			yaw = ypr[YAW] - ypr_offset[YAW];
			pitch = ypr[PITCH] - ypr_offset[PITCH];
			roll = ypr[ROLL] - ypr_offset[ROLL];
			*/
			Serial.println(pitch);
			if (pitch >= STARTING_PITCH - STARTING_PITCH_RANGE && pitch <= STARTING_PITCH + STARTING_PITCH_RANGE) {
				conditions |= 0b00000001;
				if (startup_timer == 0) {	startup_timer = millis();}
			}
			else {
				conditions &= 0b11111100;
				startup_timer = 0;
			}
		
			if (millis() - startup_timer >= REQUIRED_TIME && startup_timer != 0) {	conditions |= 0b00000010;}
		}
		//delay(3);
	}
	tilt_power = 0;
	tiltPID.reset();
}


void reset() {
	SOS(6, "\tUser reset");
}