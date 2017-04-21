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
}

struct sensorMpu_t {
  float angX;
  float angY;
}

/*
 * ************************************************************************************************************
 * -------------------------------------------   Variables   --------------------------------------------------
 * ************************************************************************************************************
 */
//  Variables generales
struct sensorFlex_t flex;           //  Estructura para guardar los datos del Flex Sensor
struct sensorMpu_t mpu;             //  Estructura para guardar los datos del MPU6050

//  Variables del Flex Sensor
const int FLEX_PIN      = A0;       //  Pin de lectura del Flex Sensor
const float VCC         = 4.98;     //  Tensión de alimentación del divisor de tensión
const float R_DIV       = 10000;    //  Resistencia en serie con el Flex Sensor
const float STRAIGHT_R  = 37300.0;  //  Resistencia del Flex Sensor estirado
const float BEND_R      = 90000.0;  //  Resistencia del Flex Sensor doblado 90 grados

//  Variables del MPU6050
MPU6050 mpuSensor;                  //  Objeto para usar las funciones del MPU6050
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
  Serial.println("Resistance: " + String(flex_data.flexR) + " ohms");

  //  Estimación del ángulo
  flex_data.flexAngle = map(flex_data.flexR, STRAIGHT_RESISTANCE, BEND_RESISTANCE, 0, 90.0);
  Serial.println("Bend: " + String(flex_data.flexAngle) + " degrees");
  Serial.println();

  return flex_data;
}

/*
 * ************************************************************************************************************
 * -------------------------------------   Funciones para el MPU6050   ----------------------------------------
 * ************************************************************************************************************
 */
void calibrate_mpu()
{
  
}

struct sensorMpu_t sense_mpu()
{
  struct sensorMpu_t mpu_data;
  

  
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

  Wire.begin();
  mpuSensor.initialize();

  if (mpuSensor.testConnection()) Serial.println("Sensor MPU6050 iniciado correctamente");
  else Serial.println("Error al iniciar el sensor MPU6050");
}

void loop()
{
  flex = sense_flex();
  mpu = sense_mpu();
  
}

