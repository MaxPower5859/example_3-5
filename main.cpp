//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

//Como el codigo se rimpia al usar la funcion strlen hay que agregar estas librerias
#include <stdio.h> // Para funciones de entrada/salida estándar
#include <string.h> // Para la función strlen

//=====[Defines]===============================================================

#define NUMBER_OF_KEYS                           4
#define BLINKING_TIME_GAS_ALARM               1000
#define BLINKING_TIME_OVER_TEMP_ALARM          500
#define BLINKING_TIME_GAS_AND_OVER_TEMP_ALARM  100
#define NUMBER_OF_AVG_SAMPLES                   100
#define OVER_TEMP_LEVEL                         50
#define TIME_INCREMENT_MS                       10

//=====[Declaration and initialization of public global objects]===============

/* 
Esta bueno usar estas clases, pero se pierde el nombre de las variables
lo cual hace que el codigo se vuelva mas complicado de seguir
 */
BusIn entradas(BUTTON1,D2,D4,D5,D6,D7,PE_12); // 0xPE_12|D7|D6|D5|D4|D2|BUTTON1
/* DigitalIn enterButton(BUTTON1);
DigitalIn alarmTestButton(D2);
DigitalIn aButton(D4);
DigitalIn bButton(D5);
DigitalIn cButton(D6);
DigitalIn dButton(D7);
DigitalIn mq2(PE_12); */

/* 
Si no la otra es hacer esto pero no siento que se gane mucho, es mas comodo usar las DigutalIn
 */
bool mq2 = (entradas.read() >> 6) & 1;
bool alarmTestButton = (entradas.read() >> 1) & 1;

BusInOut salidas(LED1,LED2,LED3); //1, 2, 4 0xLED3|LED2|LED1
/* DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2); */


DigitalInOut sirenPin(PE_10);

/*  
Esta clase llama a otra funcion de bajo nivel donde se configura el baudrate y
el pin de tx y rx.
*/

/* Analisis de Primitivas de la clase UnbufferedSerial
EL contructor de la clase necesita el pin de tx, rx y el baudrate, si no se pone nada por defecto es 9600
UnbufferedSerial(PinName tx,PinName rx,int baud = MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE);

escribe en el bus de datos, devuelve un sszite:t que puede ser de 32bits esto me sirve para saber
si se escribio correctamente el bus, le paso un buffer de datos y el tamaño de el mismo.
 El tipo void * se utiliza cuando no se tiene información sobre el tipo de datos que se encuentra en el búfer. 
 El modificador const indica que la función no modificará los datos en el búfer.
 El override al final de la función indica que esta función está sobrescribiendo una función virtual en una clase base. 
 Esto es típico en el contexto de la programación orientada a objetos y la herencia
ssize_t write(const void *buffer, size_t size) override;

Me sirve para leer el bus de datos
ssize_t read(void *buffer, size_t size) override;

off_t es un tipo de dato utilizado en C++ que se utiliza comúnmente para representar desplazamientos o offsets en operaciones de archivos
offset: Este es uno de los parámetros de la función. Indica la cantidad de bytes que se desea desplazar la posición actual en el archivo. 
Si es positivo, se moverá hacia adelante en el archivo; si es negativo, se moverá hacia atrás.

whence: Este es otro parámetro de la función que indica desde dónde se debe realizar el desplazamiento. Puede tomar uno de los siguientes valores:
SEEK_SET: Mover desde el principio del archivo.
SEEK_CUR: Mover desde la posición actual en el archivo.
SEEK_END: Mover desde el final del archivo.
la función seek se utiliza para cambiar la posición actual de lectura/escritura en un archivo a una ubicación 
específica, especificada por el parámetro offset y el parámetro whence. La función devuelve la nueva posición 
en el archivo después del desplazamiento y puede devolver un valor negativo si ocurre un error durante la operación 
de búsqueda
off_t seek(off_t offset, int whence = SEEK_SET) override.

Devulve el tamaño del archivo
off_t size() override

Devulve verdadeo si el archvio se lee desde la terminal
int isatty() override

cierra el archivo y devuelve 0 si esta bien cerrado
int close() override

la función enable_input se utiliza para controlar si un dispositivo de entrada está habilitado o deshabilitado. 
Esto puede ser útil para ahorrar energía o gestionar el estado de un dispositivo de E/S cuando no se necesita 
entrada. La función devuelve un valor entero que indica si la operación fue exitosa o si se produjo un error, y 
el parámetro enabled determina si se habilita o deshabilita la entrada del dispositivo.
int enable_input(bool enabled) override;

idem a la anterior pero para salida
int enable_output(bool enabled) override;

short poll(short events) const override;
la función poll se utiliza para comprobar eventos de tipo poll en un dispositivo de E/S sin bloquear la ejecución 
del programa. Devuelve una máscara de bits que indica qué eventos han ocurrido en el dispositivo. Esto es útil para 
tomar decisiones basadas en la disponibilidad de lectura, escritura u otros eventos en el dispositivo sin esperar a 
que ocurran.
 */
UnbufferedSerial uartUsb(USBTX, USBRX, 115200); //FALTA VER QUE REGISTRO MODIFICA

/* 
La Analog In es una clase y esta clase tiene 2 constructores sobrecargados, mas el de defecto de c++.
Luego dentro de los contructores utiliza analogin_init, y aca trabaja con un ADC especifico en la rtx_core, en funcion
del pin con el que se inicializo la clase y luego ya entra en los gpio.
 */

/* Analisis de Primitivas de la clase AnalogIn

El constructor de la clase toma un PIN y por defecto un valor de voltage que se puede ajustar y esta sobrecargado.
AnalogIn(PinName pin, float vref = MBED_CONF_TARGET_DEFAULT_ADC_VREF);

Tengo una funcion para leer el objeto y me devulve un float entre 0 y 1.
float read();

Tambien puedo leer un short de 16bits
unsigned short read_u16();

Tambien puedo leer un valor de voltage pero depende de la placa que se este utilizando
float read_voltage();

void set_reference_voltage(float vref);
Se puede setear el valor de voltage de referencia.

float get_reference_voltage() const;
Me devuelve el valor de voltage de referencia

 */
AnalogIn potentiometer(A0); //FALTA VER QUE REGISTRO MODIFICA
AnalogIn lm35(A1); //FALTA VER QUE REGISTRO MODIFICA

//=====[Declaration and initialization of public global variables]=============

bool alarmState    = OFF;
bool incorrectCode = false;
bool overTempDetector = OFF;

int numberOfIncorrectCodes = 0;
int buttonBeingCompared    = 0;
int codeSequence[NUMBER_OF_KEYS]   = { 1, 1, 0, 0 };
int buttonsPressed[NUMBER_OF_KEYS] = { 0, 0, 0, 0 };
int accumulatedTimeAlarm = 0;

bool gasDetectorState          = OFF;
bool overTempDetectorState     = OFF;

float potentiometerReading = 0.0;
float lm35ReadingsAverage  = 0.0;
float lm35ReadingsSum      = 0.0;
float lm35ReadingsArray[NUMBER_OF_AVG_SAMPLES];
float lm35TempC            = 0.0;


//=====[Declarations (prototypes) of public functions]=========================

void inputsInit();
void outputsInit();

void alarmActivationUpdate();
void alarmDeactivationUpdate();

void uartTask();
void availableCommands();
bool areEqual();

/**
 * @brief Conevierte de celsius a farenheite
 * @note Esta funcion recibe una temperatura en grados celsius y a traves de una formula la pasa a farenheite
 * @param tempInCelsiusDegrees recibe la temperatura como un float.
 * @retval devuelve la temperatura farenheite como float.
 */
float celsiusToFahrenheit( float tempInCelsiusDegrees );

/**
 * @brief Conevierte de mv a grados celsius
 * @note Esta funcion convierte milivolts leidos del ADC a un valor de temperatura usando una formula
 * @param analogReading recibe la coneversion del ADC como un float.
 * @retval devuelve la temperatura como float.
 */
float analogReadingScaledWithTheLM35Formula( float analogReading );

//=====[Main function, the program entry point after power on or reset]========

int main()
{
    //Configura todas las entradas del sistema (PullDown y OpenDrain)
    inputsInit();
    //Setea las variables que se van a usar como salidas 
    outputsInit();
    while (true) {
        /*Actualiza las variables qie se usan como salida en funcion de las entradas
        Lee el sensor de temperatura y los demas botones y el potenciometro.
        En particular con el sensor de temperatura se realiza un promedio para reducri el ruido 
        Tambien tiene el inconveniente de que al inciar tenes valores incoherentes
        */

        alarmActivationUpdate();
        alarmDeactivationUpdate();
        uartTask();
        delay(TIME_INCREMENT_MS);
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
/*     alarmTestButton.mode(PullDown);
    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown); */
    entradas.mode(PullDown);
    sirenPin.mode(OpenDrain);
    sirenPin.input();
}

void outputsInit()
{
/*     alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF; */

/*     salidas.write(1);
    salidas.write(2);
    salidas.write(4); */

    
    salidas.write(0);
}

void alarmActivationUpdate()
{
    static int lm35SampleIndex = 0;
    int i = 0;

    lm35ReadingsArray[lm35SampleIndex] = lm35.read();
    lm35SampleIndex++;
    if ( lm35SampleIndex >= NUMBER_OF_AVG_SAMPLES) {
        lm35SampleIndex = 0;
    }
    
       lm35ReadingsSum = 0.0;
    for (i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        lm35ReadingsSum = lm35ReadingsSum + lm35ReadingsArray[i];
    }
    lm35ReadingsAverage = lm35ReadingsSum / NUMBER_OF_AVG_SAMPLES;
       lm35TempC = analogReadingScaledWithTheLM35Formula ( lm35ReadingsAverage );    
    
    if ( lm35TempC > OVER_TEMP_LEVEL ) {
        overTempDetector = ON;
    } else {
        overTempDetector = OFF;
    }

    if( !mq2) {
        gasDetectorState = ON;
        alarmState = ON;
    }
    if( overTempDetector ) {
        overTempDetectorState = ON;
        alarmState = ON;
    }
    if( alarmTestButton ) {             
        overTempDetectorState = ON;
        gasDetectorState = ON;
        alarmState = ON;
    }    
    if( alarmState ) { 
        accumulatedTimeAlarm = accumulatedTimeAlarm + TIME_INCREMENT_MS;
        sirenPin.output();                                     
        sirenPin = LOW;                                        
    
        if( gasDetectorState && overTempDetectorState ) {
            if( accumulatedTimeAlarm >= BLINKING_TIME_GAS_AND_OVER_TEMP_ALARM ) {
                accumulatedTimeAlarm = 0;
                /* alarmLed = !alarmLed; */
                salidas.write(!(salidas.read() & 1));
            }
        } else if( gasDetectorState ) {
            if( accumulatedTimeAlarm >= BLINKING_TIME_GAS_ALARM ) {
                accumulatedTimeAlarm = 0;
                salidas.write(!(salidas.read() & 1));
            }
        } else if ( overTempDetectorState ) {
            if( accumulatedTimeAlarm >= BLINKING_TIME_OVER_TEMP_ALARM  ) {
                accumulatedTimeAlarm = 0;
                salidas.write(!(salidas.read() & 1));
            }
        }
    } else{
        //alarmLed = OFF;
        salidas.write(0);
        gasDetectorState = OFF;
        overTempDetectorState = OFF;
        sirenPin.input();                                  
    }
}
/* 
void alarmDeactivationUpdate()
{
    if ( numberOfIncorrectCodes < 5 ) {
        if ( aButton && bButton && cButton && dButton && !enterButton ) {
            incorrectCodeLed = OFF;
        }
        if ( enterButton && !incorrectCodeLed && alarmState ) {
            buttonsPressed[0] = aButton;
            buttonsPressed[1] = bButton;
            buttonsPressed[2] = cButton;
            buttonsPressed[3] = dButton;
            if ( areEqual() ) {
                alarmState = OFF;
                numberOfIncorrectCodes = 0;
            } else {
                incorrectCodeLed = ON;
                numberOfIncorrectCodes++;
            }
        }
    } else {
        systemBlockedLed = ON;
    }
}

 */
void alarmDeactivationUpdate()
{
    if ( numberOfIncorrectCodes < 5 ) {
        if ( (entradas.read() >> 2) & 1 && (entradas.read() >> 3) & 1 && (entradas.read() >> 4) & 1 && (entradas.read() >> 5) & 1 && !(entradas.read() & 1) ) {
            salidas.write(salidas.read() & 0x011) ;
        }
        if ( entradas.read() & 1 && !(salidas.read() >> 2 & 1) && alarmState ) {
            buttonsPressed[0] = (entradas.read() >> 2) & 1;
            buttonsPressed[1] = (entradas.read() >> 3) & 1;
            buttonsPressed[2] = (entradas.read() >> 4) & 1;
            buttonsPressed[3] = (entradas.read() >> 5) & 1;
            if ( areEqual() ) {
                alarmState = OFF;
                numberOfIncorrectCodes = 0;
            } else {
                salidas.write(salidas.read() | 0x100);
                numberOfIncorrectCodes++;
            }
        }
    } else {
        salidas.write(salidas.read() | 0x010);
    }
}

void uartTask()
{
    char receivedChar = '\0';
    char str[100];
    int stringLength;
    /* 
    Esta es una clase y utiliza un constructor que requiere el bausrate y los pines que van a usar como tx y rx
     */
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
        switch (receivedChar) {
        case '1':
            if ( alarmState ) {
                uartUsb.write( "The alarm is activated\r\n", 24);
            } else {
                uartUsb.write( "The alarm is not activated\r\n", 28);
            }
            break;

        case '2':
            if ( !mq2 ) {
                uartUsb.write( "Gas is being detected\r\n", 22);
            } else {
                uartUsb.write( "Gas is not being detected\r\n", 27);
            }
            break;

        case '3':
            if ( overTempDetector ) {
                uartUsb.write( "Temperature is above the maximum level\r\n", 40);
            } else {
                uartUsb.write( "Temperature is below the maximum level\r\n", 40);
            }
            break;
            
        case '4':
            uartUsb.write( "Please enter the code sequence.\r\n", 33 );
            uartUsb.write( "First enter 'A', then 'B', then 'C', and ", 41 ); 
            uartUsb.write( "finally 'D' button\r\n", 20 );
            uartUsb.write( "In each case type 1 for pressed or 0 for ", 41 );
            uartUsb.write( "not pressed\r\n", 13 );
            uartUsb.write( "For example, for 'A' = pressed, ", 32 );
            uartUsb.write( "'B' = pressed, 'C' = not pressed, ", 34);
            uartUsb.write( "'D' = not pressed, enter '1', then '1', ", 40 );
            uartUsb.write( "then '0', and finally '0'\r\n\r\n", 29 );

            incorrectCode = false;

            for ( buttonBeingCompared = 0;
                  buttonBeingCompared < NUMBER_OF_KEYS;
                  buttonBeingCompared++) {

                uartUsb.read( &receivedChar, 1 );
                uartUsb.write( "*", 1 );

                if ( receivedChar == '1' ) {
                    if ( codeSequence[buttonBeingCompared] != 1 ) {
                        incorrectCode = true;
                    }
                } else if ( receivedChar == '0' ) {
                    if ( codeSequence[buttonBeingCompared] != 0 ) {
                        incorrectCode = true;
                    }
                } else {
                    incorrectCode = true;
                }
            }

            if ( incorrectCode == false ) {
                uartUsb.write( "\r\nThe code is correct\r\n\r\n", 25 );
                alarmState = OFF;
                //incorrectCodeLed = OFF;
                salidas.write(salidas.read() & 0x011);
                numberOfIncorrectCodes = 0;
            } else {
                uartUsb.write( "\r\nThe code is incorrect\r\n\r\n", 27 );
                //incorrectCodeLed = ON;
                salidas.write(salidas.read() | 0x100);
                numberOfIncorrectCodes++;
            }                
            break;

        case '5':
            uartUsb.write( "Please enter new code sequence\r\n", 32 );
            uartUsb.write( "First enter 'A', then 'B', then 'C', and ", 41 );
            uartUsb.write( "finally 'D' button\r\n", 20 );
            uartUsb.write( "In each case type 1 for pressed or 0 for not ", 45 );
            uartUsb.write( "pressed\r\n", 9 );
            uartUsb.write( "For example, for 'A' = pressed, 'B' = pressed,", 46 );
            uartUsb.write( " 'C' = not pressed,", 19 );
            uartUsb.write( "'D' = not pressed, enter '1', then '1', ", 40 );
            uartUsb.write( "then '0', and finally '0'\r\n\r\n", 29 );

            for ( buttonBeingCompared = 0; 
                  buttonBeingCompared < NUMBER_OF_KEYS; 
                  buttonBeingCompared++) {

                uartUsb.read( &receivedChar, 1 );
                uartUsb.write( "*", 1 );

                if ( receivedChar == '1' ) {
                    codeSequence[buttonBeingCompared] = 1;
                } else if ( receivedChar == '0' ) {
                    codeSequence[buttonBeingCompared] = 0;
                }
            }

            uartUsb.write( "\r\nNew code generated\r\n\r\n", 24 );
            break;
 
        case 'p':
        case 'P':
            potentiometerReading = potentiometer.read();
            sprintf ( str, "Potentiometer: %.2f\r\n", potentiometerReading );
            stringLength = strlen(str);
            uartUsb.write( str, stringLength );
            break;

        case 'c':
        case 'C':
            sprintf ( str, "Temperature: %.2f \xB0 C\r\n", lm35TempC );
            stringLength = strlen(str);
            uartUsb.write( str, stringLength );
            break;

        case 'f':
        case 'F':
            sprintf ( str, "Temperature: %.2f \xB0 F\r\n", 
                celsiusToFahrenheit( lm35TempC ) );
            stringLength = strlen(str);
            uartUsb.write( str, stringLength );
            break;

        default:
            availableCommands();
            break;

        }
    }
}

void availableCommands()
{
    /* Diferencias entre uartUSB.write y printf
    uartUsb.write se utiliza para enviar datos binarios o caracteres sin formateo a través de una conexión UART.
    printf se utiliza para formatear y mostrar datos de manera legible por humanos en la consola o en otros 
    flujos de salida, lo que facilita la visualización y la depuración de datos.
    Tampoco hay que poner la cantidad de caracteras que imprimis por pantalla con printf
     */

    uartUsb.write( "Available commands:\r\n", 21 );
    uartUsb.write( "Press '1' to get the alarm state\r\n", 34 );
    uartUsb.write( "Press '2' to get the gas detector state\r\n", 41 );
    uartUsb.write( "Press '3' to get the over temperature detector state\r\n", 54 );
    uartUsb.write( "Press '4' to enter the code sequence\r\n", 38 );
    uartUsb.write( "Press '5' to enter a new code\r\n", 31 );
    uartUsb.write( "Press 'P' or 'p' to get potentiometer reading\r\n", 47 );
    uartUsb.write( "Press 'f' or 'F' to get lm35 reading in Fahrenheit\r\n", 52 );
    uartUsb.write( "Press 'c' or 'C' to get lm35 reading in Celsius\r\n\r\n", 51 );
}

bool areEqual()
{
    int i;

    for (i = 0; i < NUMBER_OF_KEYS; i++) {
        if (codeSequence[i] != buttonsPressed[i]) {
            return false;
        }
    }

    return true;
}

float analogReadingScaledWithTheLM35Formula( float analogReading )
{
    return ( analogReading * 3.3 / 0.01 );
}

float celsiusToFahrenheit( float tempInCelsiusDegrees )
{
    return ( tempInCelsiusDegrees * 9.0 / 5.0 + 32.0 );
}
