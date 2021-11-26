// simple PID DAC
// https://microcontrollerslab.com/pid-controller-implementation-using-arduino/

#define DAC A14
#define SENSOR A0

double sensed_output, control_signal;
double setpoint;
double Kp; //proportional gain
double Ki; //integral gain
double Kd; //derivative gain
int T; //sample time in milliseconds (ms)
unsigned long last_time;
double delta_error, total_error, last_error, error;
int max_control =  1000;
int min_control = 5;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  analogWriteResolution(10);
  analogWrite(DAC, 12);

  Kp = .1;
  Ki = .8;
  Kd = .0001;
  T = 800;  // ms
  setpoint = 512;
}

void loop() {
  static int bias = 0;

  while (Serial.available()) { // perturb
    char c = Serial.read();
    if (c == '+') bias++;
    else if (c == '-') bias--;
  }
  sensed_output = analogRead(SENSOR) + bias;
  int r = PID_Control(); //calls the PID function every T interval and outputs a control signal
  if (r) {
    Serial.print(error);  Serial.print(" ");
    Serial.print(delta_error);  Serial.print(" ");
    Serial.print(total_error);  Serial.print(" ");
    Serial.print(sensed_output);  Serial.print(" ");
    Serial.println(control_signal);
    analogWrite(DAC, control_signal);
  }
}

int PID_Control() {

  unsigned long current_time = millis(); //returns the number of milliseconds passed since the Arduino started running the program

  int delta_time = current_time - last_time; //delta time interval

  if (delta_time < T) return 0;

  error = setpoint - sensed_output;
  total_error += error; //accumalates the error - integral term
  if (total_error >= max_control) total_error = max_control;
  else if (total_error <= min_control) total_error = min_control;

  delta_error = error - last_error; //difference of error for derivative term

  control_signal = Kp * error + Ki * total_error + Kd  * delta_error; //PID control compute
  if (control_signal >= max_control) control_signal = max_control;
  else if (control_signal <= min_control) control_signal = min_control;

  last_error = error;
  last_time = current_time;
  return 1;
}
