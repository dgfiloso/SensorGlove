#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

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
struct sensorFlex_t flex;             //  Estructura para guardar los datos del Flex Sensor
struct sensorMpu_t mpu;               //  Estructura para guardar los datos del MPU6050
unsigned long exec_time;              //  Tiempo de ejecución del sensado y el envío de datos
int dir                 = 0;          //  Indica la dirección del coche

//  Variables del Flex Sensor
const int FLEX_PIN      = A3;         //  Pin de lectura del Flex Sensor
const float VCC         = 4.98;       //  Tensión de alimentación del divisor de tensión
const float R_DIV       = 10000;      //  Resistencia en serie con el Flex Sensor
const float STRAIGHT_R  = 38400.0;    //  Resistencia del Flex Sensor estirado
const float BEND_R      = 184000.0;  //  Resistencia del Flex Sensor doblado 90 grados

//  Variables del MPU6050
MPU6050 mpuSensor;                    //  Objeto para usar las funciones del MPU6050
const int CALIB_PIN = 5;              //  Pin del led que indica que se está calibrando el dispositivo
const int READY_PIN = 4;             //  Pin del led que indica que el dispositivo está listo para usarse

int buffersize=1000;                  //  Cantidad de lecturas para realizar la media, haciendo este valor mayor se consigue más precisión pero el sketch será más lento (default:1000);
int acel_deadzone=8;                  //  Error permitido al acelerómetro, haciendo este valor más pequeño se consigue más precisión pero puede que no converja  (default:8)
int giro_deadzone=1;                  //  Error permitido al giroscopo, haciendo este valor más pequeño se consigue más precisión pero puede que no converja  (default:1)
int mean_ax,mean_ay,mean_az;          //  Valores del acelerómetro para el calibrado
int mean_gx,mean_gy,mean_gz;          //  Valores del giroscopo para el calibrado
int state=0;                          //  Estado de la calibración
int ax_offset,ay_offset,az_offset;    //  Offset del acelerómetro
int gx_offset,gy_offset,gz_offset;    //  Offset del giroscopo

float ang_x_prev, ang_y_prev;         //  Ángulos calculados en el muestreo anterior
long tiempo_prev;                     //  Tiempo calculado en el muestreo anterior

//  Variable del NRF24L01
const int pinCE = 9;                  //  Pin CE 
const int pinCSN = 10;                //  Pin Chip Select del SPI
RF24 radio(pinCE, pinCSN);            //  Objeto de tipo RF24 para usar las funciones del módulo de transmisión
const uint64_t pipe = 0xE8E8F0F0E1LL; //  
int data[2];                          //  Datos que se van a transmitir por RF a 2,4 GHz

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
  radio.begin();
  radio.openWritingPipe(pipe);

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

  //  set offsets
  mpuSensor.setXAccelOffset(-2184);
  mpuSensor.setYAccelOffset(-570);
  mpuSensor.setZAccelOffset(1428);
  mpuSensor.setXGyroOffset(57);
  mpuSensor.setYGyroOffset(4);
  mpuSensor.setZGyroOffset(40);

  digitalWrite(CALIB_PIN, LOW);
  digitalWrite(READY_PIN, HIGH);
  Serial.println("\nCalibration FINISHED!");
}

void loop()
{
//  if (state==0){
//    Serial.println("\nReading sensors for first time...");
//    mean_mpu();
//    state++;
//    delay(1000);
//  }
//
//  if (state==1) {
//    Serial.println("\nCalculating offsets...");
//    calibration_mpu();
//    state++;
//    delay(1000);
//  }
//
//    if (state==2) {
//    mean_mpu();
//    digitalWrite(CALIB_PIN, LOW);
//    digitalWrite(READY_PIN, HIGH);
//    Serial.println("\nFINISHED!");
//    Serial.print("\nSensor readings with offsets:\t");
//    Serial.print(mean_ax); 
//    Serial.print("\t");
//    Serial.print(mean_ay); 
//    Serial.print("\t");
//    Serial.print(mean_az); 
//    Serial.print("\t");
//    Serial.print(mean_gx); 
//    Serial.print("\t");
//    Serial.print(mean_gy); 
//    Serial.print("\t");
//    Serial.println(mean_gz);
//    Serial.print("Your offsets:\t");
//    Serial.print(ax_offset); 
//    Serial.print("\t");
//    Serial.print(ay_offset); 
//    Serial.print("\t");
//    Serial.print(az_offset); 
//    Serial.print("\t");
//    Serial.print(gx_offset); 
//    Serial.print("\t");
//    Serial.print(gy_offset); 
//    Serial.print("\t");
//    Serial.println(gz_offset); 
//    Serial.println("\nData is printed as: acelX acelY acelZ giroX giroY giroZ");
//    Serial.println("Check that your sensor readings are close to 0 0 16384 0 0 0");
//    
//    mpuSensor.setXAccelOffset(ax_offset);
//    mpuSensor.setYAccelOffset(ay_offset);
//    mpuSensor.setZAccelOffset(az_offset);
//    mpuSensor.setXGyroOffset(gx_offset);
//    mpuSensor.setYGyroOffset(gy_offset);
//    mpuSensor.setZGyroOffset(gz_offset);
//    
//    while (1)
//    {
//      exec_time = millis();
      flex = sense_flex();
      mpu = sense_mpu();

      if (mpu.angY > 45) {
        dir = 0;          //  Hacia delante
      } else if (mpu.angY < -45) {
        dir = 1;          //  Hacia atrás
      }

      if (mpu.angX > 10) {
        data[1] = 1;      //  Girar a la derecha
      } else if (mpu.angX < -10) {
        data[1] = 2;      //  Girar a la izquierda
      } else {
        data[1] = 0;      //  Recto
      }

      
      if ( dir == 0)
      {
        if ((0 <= flex.flexAngle) && (flex.flexAngle < 13)) {
          data[0] = 6;                            //  Máxima aceleración hacia delante
        } else if ((13 <= flex.flexAngle) && (flex.flexAngle < 26)) {
          data[0] = 5;
        } else if ((26 <= flex.flexAngle) && (flex.flexAngle < 39)) {
          data[0] = 4;
        } else if ((39 <= flex.flexAngle) && (flex.flexAngle < 52)) {
          data[0] = 3;
        } else if ((52 <= flex.flexAngle) && (flex.flexAngle < 65)) {
          data[0] = 2;
        } else if ((65 <= flex.flexAngle) && (flex.flexAngle < 78)) {
          data[0] = 1;
        } else if (78 <= flex.flexAngle) {
          data[0] = 0;                            //  Parado
        }  
      }
      else if (dir == 1)
      {
        if ((0 <= flex.flexAngle) && (flex.flexAngle < 13)) {
          data[0] = 12;                           //  Máxima aceleración hacia atrás  
        } else if ((13 <= flex.flexAngle) && (flex.flexAngle < 26)) {
          data[0] = 11;
        } else if ((26 <= flex.flexAngle) && (flex.flexAngle < 39)) {
          data[0] = 10;
        } else if ((39 <= flex.flexAngle) && (flex.flexAngle < 52)) {
          data[0] = 9;
        } else if ((52 <= flex.flexAngle) && (flex.flexAngle < 65)) {
          data[0] = 8;
        } else if ((65 <= flex.flexAngle) && (flex.flexAngle < 78)) {
          data[0] = 7;
        } else if (78 <= flex.flexAngle) {
          data[0] = 0;                            //  Parado
        } 
      }     
      
      radio.write(data, sizeof(data));

//      exec_time = millis()-exec_time;
//      Serial.print("Flex Sensor: ");
//      Serial.print("R = ");
//      Serial.print(flex.flexR);
//      Serial.print(" ; Ang = ");
//      Serial.println(flex.flexAngle);
//      Serial.print("MPU: ");
//      Serial.print("AngX = ");
//      Serial.print(mpu.angX);
//      Serial.print(" ; AngY = ");
//      Serial.println(mpu.angY);
//      Serial.print("Tiempo de ejecucion = ");
//      Serial.print(exec_time);
//      Serial.println(" ms");
//      Serial.print("Direccion: ");
//      Serial.println(dir);
//      Serial.print("Enviado: data[0]=");
//      Serial.print(data[0]);
//      Serial.print(" ; data[1]=");
//      Serial.println(data[1]);
//      Serial.println(" ");
      
      delay(10);
//    }
//  }
}

