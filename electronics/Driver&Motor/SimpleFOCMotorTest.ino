#include <SimpleFOC.h>

BLDCMotor motor = BLDCMotor(7); // 7 pole pairs for 2204 drone motor
BLDCDriver3PWM driver = BLDCDriver3PWM(9, 10, 11, 12); // PWM pins: 9, 10, 11 | Enable pin: 12

// instantiate the commander
Commander command = Commander(Serial);
void doTarget(char* cmd) { command.scalar(&motor.target, cmd); }
void doLimit(char* cmd) { command.scalar(&motor.voltage_limit, cmd); }

void setup() {

  // use monitoring with serial 
  Serial.begin(115200);
  // enable more verbose output for debugging
  // comment out if not needed
  SimpleFOCDebug::enable(&Serial);

  // driver config
  // power supply voltage [V]
  driver.voltage_power_supply = 11.1; //set your power supply voltage;
  // limit the maximal dc voltage the driver can set
  driver.voltage_limit = 6; //set your voltage limit; Usually half of power supply voltage is a good starting point
  if(!driver.init()){
    Serial.println("Driver init failed!");
    return;
  }
  // link the motor and the driver
  motor.linkDriver(&driver);

  // limiting motor movements
  // start very low for high resistance motors
  // current = voltage / resistance, so try to be well under 1Amp
  motor.voltage_limit = 4; //set your voltage limit;   // [V]
 
  // open loop control config
  motor.controller = MotionControlType::velocity_openloop;

  // init motor hardware
  if(!motor.init()){
    Serial.println("Motor init failed!");
    return;
  }

  // set the target velocity [rad/s]
  motor.target = 20; // one rotation per second

  // add target command T
  command.add('T', doTarget, "target velocity");
  command.add('L', doLimit, "voltage limit");

  Serial.println("Motor ready!");
  Serial.println("Set target velocity [rad/s]");
  _delay(1000);
}

void loop() {

  // loop FOC algorithm, should be called as 
  // frequently as possible for best 
  // performance (e.g. 1kHz+)
  motor.loopFOC();

  // open loop velocity movement
  motor.move();

  // user communication
  command.run();
}
