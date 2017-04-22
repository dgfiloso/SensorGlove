#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>

/*
 * ************************************************************************************************************
 * -------------------------------------------   Estructuras   ------------------------------------------------
 * ************************************************************************************************************
 */
struct sensorFlex_t {
  float flexR;
  float flexAngle;
};

struct sensorMpu_t {
  float angX;
  float angY;
};

/*
 * ************************************************************************************************************
 * -------------------------------------------   Variables   --------------------------------------------------
 * ************************************************************************************************************
 */
//  Variables generales
struct sensorFlex_t flex;           //  Estructura para guardar los datos del Flex Sensor
struct sensorMpu_t mpu;             //  Estructura para guardar los datos del MPU6050
unsigned long exec_time;            //  Tiempo de ejecución del sensado y el envío de datos

//  Variables del Flex Sensor
const int FLEX_PIN      = A0;       //  Pin de lectura del Flex Sensor
const float VCC         = 4.98;     //  Tensión de alimentación del divisor de tensión
const float R_DIV       = 10000;    //  Resistencia en serie con el Flex Sensor
const float STRAIGHT_R  = 37300.0;  //  Resistencia del Flex Sensor estirado
const float BEND_R      = 90000.0;  //  Resistencia del Flex Sensor doblado 90 grados

//  Variables del MPU6050
MPU6050 mpuSensor;                  //  Objeto para usar las funciones del MPU6050
const int CALIB_PIN = 8;            //  Pin del led que indica que se está calibrando el dispositivo
const int READY_PIN = 10;           //  Pin del led que indica que el dispositivo está listo para usarse

int buffersize=1000;                //Amount of readings used to average, make it higher to get more precision but sketch will be slower  (default:1000)
int acel_deadzone=8;                //Acelerometer error allowed, make it lower to get more precision, but sketch may not converge  (default:8)
int giro_deadzone=1;                //Giro error allowed, make it lower to get more precision, but sketch may not converge  (default:1)
int mean_ax,mean_ay,mean_az;        //  Valores del acelerómetro para el calibrado
int mean_gx,mean_gy,mean_gz;        //  Valores del giroscopo para el calibrado
int state=0;                        //  Estado de la calibración
int ax_offset,ay_offset,az_offset;  //  Offset del acelerómetro
int gx_offset,gy_offset,gz_offset;  //  Offset del giroscopo

float ang_x_prev, ang_y_prev;       //  Ángulos calculados en el muestreo anterior
long tiempo_prev;                   //  Tiempo calculado en el muestreo anterior

/*
 * ************************************************************************************************************
 * -------------------------------------   Funciones para el Flex Sensor   ------------------------------------
 * ************************************************************************************************************
 */
struct sensorFlex_t sense_flex()
{
  struct sensorFlex_t flex_data;
  
  //  Leemos el ADC y calculamos el voltaje y la resistencia
  int flexADC = analogRead(FLEX_PIN);
  float flexV = flexADC * VCC / 1023.0;
  flex_data.flexR = R_DIV * (VCC / flexV - 1.0);

  //  Estimación del ángulo
  flex_data.flexAngle = map(flex_data.flexR, STRAIGHT_R, BEND_R, 0, 90.0);

  return flex_data;
}

/*
 * ************************************************************************************************************
 * -------------------------------------   Funciones para el MPU6050   ----------------------------------------
 * ************************************************************************************************************
 */
 void mean_mpu()
 {
  int ax, ay, az;
  int gx, gy, gz;
  long i=0,buff_ax=0,buff_ay=0,buff_az=0,buff_gx=0,buff_gy=0,buff_gz=0;

  while (i<(buffersize+101)){
    // read raw accel/gyro measurements from device
    mpuSensor.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    if (i>100 && i<=(buffersize+100)){ //First 100 measures are discarded
      buff_ax=buff_ax+ax;
      buff_ay=buff_ay+ay;
      buff_az=buff_az+az;
      buff_gx=buff_gx+gx;
      buff_gy=buff_gy+gy;
      buff_gz=buff_gz+gz;
    }
    if (i==(buffersize+100)){
      mean_ax=buff_ax/buffersize;
      mean_ay=buff_ay/buffersize;
      mean_az=buff_az/buffersize;
      mean_gx=buff_gx/buffersize;
      mean_gy=buff_gy/buffersize;
      mean_gz=buff_gz/buffersize;
    }
    i++;
    delay(2); //Needed so we don't get repeated measures
  }
}

void calibration_mpu()
{
  ax_offset=-mean_ax/8;
  ay_offset=-mean_ay/8;
  az_offset=(16384-mean_az)/8;

  gx_offset=-mean_gx/4;
  gy_offset=-mean_gy/4;
  gz_offset=-mean_gz/4;
  while (1){
    int ready=0;
    mpuSensor.setXAccelOffset(ax_offset);
    mpuSensor.setYAccelOffset(ay_offset);
    mpuSensor.setZAccelOffset(az_offset);

    mpuSensor.setXGyroOffset(gx_offset);
    mpuSensor.setYGyroOffset(gy_offset);
    mpuSensor.setZGyroOffset(gz_offset);

    mean_mpu();
    Serial.println("...");

    if (abs(mean_ax)<=acel_deadzone) ready++;
    else ax_offset=ax_offset-mean_ax/acel_deadzone;

    if (abs(mean_ay)<=acel_deadzone) ready++;
    else ay_offset=ay_offset-mean_ay/acel_deadzone;

    if (abs(16384-mean_az)<=acel_deadzone) ready++;
    else az_offset=az_offset+(16384-mean_az)/acel_deadzone;

    if (abs(mean_gx)<=giro_deadzone) ready++;
    else gx_offset=gx_offset-mean_gx/(giro_deadzone+1);

    if (abs(mean_gy)<=giro_deadzone) ready++;
    else gy_offset=gy_offset-mean_gy/(giro_deadzone+1);

    if (abs(mean_gz)<=giro_deadzone) ready++;
    else gz_offset=gz_offset-mean_gz/(giro_deadzone+1);

    if (ready==6) break;
  }
}

struct sensorMpu_t sense_mpu()
{
  struct sensorMpu_t mpu_data;
  int ax, ay, az;
  int gx, gy, gz;
  float dt;
  
  // Leer las aceleraciones y velocidades angulares
  mpuSensor.getAcceleration(&ax, &ay, &az);
  mpuSensor.getRotation(&gx, &gy, &gz);

  //  Calculamos los ángulos usando un filtro complementario
  dt = (millis() - tiempo_prev) / 1000.0;
  tiempo_prev = millis();
 
  //Calcular los ángulos con acelerometro
  float accel_ang_x = atan(ay / sqrt(pow(ax, 2) + pow(az, 2)))*(180.0 / 3.14);
  float accel_ang_y = atan(-ax / sqrt(pow(ay, 2) + pow(az, 2)))*(180.0 / 3.14);
 
  //Calcular angulo de rotación con giroscopio y filtro complementario
  mpu_data.angX = 0.98*(ang_x_prev + (gx / 131)*dt) + 0.02*accel_ang_x;
  mpu_data.angY = 0.98*(ang_y_prev + (gy / 131)*dt) + 0.02*accel_ang_y;
 
  ang_x_prev = mpu_data.angX;
  ang_y_prev = mpu_data.angY;
 
  return mpu_data;
}

/*
 * ************************************************************************************************************
 * -------------------------------------   Programa principal   -----------------------------------------------
 * ************************************************************************************************************
 */
void setup()
{
  Serial.begin(115200);

  pinMode(FLEX_PIN, INPUT);
  pinMode(CALIB_PIN, OUTPUT);
  pinMode(READY_PIN, OUTPUT);
  digitalWrite(CALIB_PIN, HIGH);
  digitalWrite(READY_PIN, LOW);

  Wire.begin();
  mpuSensor.initialize();

  if (mpuSensor.testConnection()) Serial.println("Sensor MPU6050 iniciado correctamente");
  else Serial.println("Error al iniciar el sensor MPU6050");

  delay(1000);
  // reset offsets
  mpuSensor.setXAccelOffset(0);
  mpuSensor.setYAccelOffset(0);
  mpuSensor.setZAccelOffset(0);
  mpuSensor.setXGyroOffset(0);
  mpuSensor.setYGyroOffset(0);
  mpuSensor.setZGyroOffset(0);
}

void loop()
{
  if (state==0){
    Serial.println("\nReading sensors for first time...");
    mean_mpu();
    state++;
    delay(1000);
  }

  if (state==1) {
    Serial.println("\nCalculating offsets...");
    calibration_mpu();
    state++;
    delay(1000);
  }

    if (state==2) {
    mean_mpu();
    digitalWrite(CALIB_PIN, LOW);
    digitalWrite(READY_PIN, HIGH);
    Serial.println("\nFINISHED!");
    Serial.print("\nSensor readings with offsets:\t");
    Serial.print(mean_ax); 
    Serial.print("\t");
    Serial.print(mean_ay); 
    Serial.print("\t");
    Serial.print(mean_az); 
    Serial.print("\t");
    Serial.print(mean_gx); 
    Serial.print("\t");
    Serial.print(mean_gy); 
    Serial.print("\t");
    Serial.println(mean_gz);
    Serial.print("Your offsets:\t");
    Serial.print(ax_offset); 
    Serial.print("\t");
    Serial.print(ay_offset); 
    Serial.print("\t");
    Serial.print(az_offset); 
    Serial.print("\t");
    Serial.print(gx_offset); 
    Serial.print("\t");
    Serial.print(gy_offset); 
    Serial.print("\t");
    Serial.println(gz_offset); 
    Serial.println("\nData is printed as: acelX acelY acelZ giroX giroY giroZ");
    Serial.println("Check that your sensor readings are close to 0 0 16384 0 0 0");
    
    mpuSensor.setXAccelOffset(ax_offset);
    mpuSensor.setYAccelOffset(ay_offset);
    mpuSensor.setZAccelOffset(az_offset);
    mpuSensor.setXGyroOffset(gx_offset);
    mpuSensor.setYGyroOffset(gy_offset);
    mpuSensor.setZGyroOffset(gz_offset);
    
    while (1)
    {
      exec_time = millis();
      flex = sense_flex();
      mpu = sense_mpu();
      exec_time = millis()-exec_time;
      Serial.print("Flex Sensor: ");
      Serial.print("R = ");
      Serial.print(flex.flexR);
      Serial.print(" ; Ang = ");
      Serial.println(flex.flexAngle);
      Serial.print("MPU: ");
      Serial.print("AngX = ");
      Serial.print(mpu.angX);
      Serial.print(" ; AngY = ");
      Serial.println(mpu.angY);
      Serial.print("Tiempo de ejecucion = ");
      Serial.print(exec_time);
      Serial.println(" ms");
      Serial.println(" ");
      delay(1000);
    }
  }
}

