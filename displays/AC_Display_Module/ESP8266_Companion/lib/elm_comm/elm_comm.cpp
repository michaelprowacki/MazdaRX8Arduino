#include "elm_comm.hpp"

void elm_comm::init() {
    elm = new ELMo();
    hasInitializedElm = elm->initialize();

    // Serial.println(
    //     "Voltage,load,coolantTemp,rpm,kph,timingAdvance,intakeAirTemp,throttle,fuelLevel,absLoad,commandedAirFuelRatio,"
    //     "ambientAirTemp,fuelType,ethanolPercent,oilTemp,demandedTorque,torque,referenceTorque,deltaTime" );
}

void elm_comm::tick() {
    if ( hasInitializedElm && millis() - lastLoop > 50 ) {
        // unsigned long a = millis();

        logln("%s", elm->send("0105").c_str());

        // float volt = elm->batteryVoltage();
        // float load = elm->engineLoad();
        // float coolantTemp = elm->engineCoolantTemp();
        // float rpm = elm->rpm();
        // int32_t kph = elm->kph();
        // float timingAdvance = elm->timingAdvance();
        // float intakeAirTemp = elm->intakeAirTemp();
        // float throttle = elm->throttle();
        // float fuelLevel = elm->fuelLevel();
        // float absLoad = elm->absLoad();
        // float commandedAirFuelRatio = elm->commandedAirFuelRatio();
        // float ambientAirTemp = elm->ambientAirTemp();
        // uint8_t fuelType = elm->fuelType();
        // float ethanolPercent = elm->ethanolPercent();
        // float oilTemp = elm->oilTemp();
        // float demandedTorque = elm->demandedTorque();
        // float torque = elm->torque();
        // uint16_t referenceTorque = elm->referenceTorque();
        // Serial.printf( "%s,%s,%s,%s,%d,%s,%s,%s,%s,%s,%s,%s,%d,%s,%s,%s,%s,%d,%lu\r\n", String( volt ).c_str(),
        //                String( load ).c_str(), String( coolantTemp ).c_str(), String( rpm ).c_str(), (int)kph,
        //                String( timingAdvance ).c_str(), String( intakeAirTemp ).c_str(), String( throttle ).c_str(),
        //                String( fuelLevel ).c_str(), String( absLoad ).c_str(), String( commandedAirFuelRatio ).c_str(),
        //                String( ambientAirTemp ).c_str(), (int)fuelType, String( ethanolPercent ).c_str(),
        //                String( oilTemp ).c_str(), String( demandedTorque ).c_str(), String( torque ).c_str(),
        //                (int)referenceTorque, millis() - a );

        lastLoop = millis();
    }
}

void elm_comm::initializeElm() { hasInitializedElm = elm->initialize(); }
