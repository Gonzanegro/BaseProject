#include "mbed.h"

#define ON    1
#define OFF   0
#define NROBOTONES  4
#define CICLO 6
#define MAXLED      4
#define TIMEHB 150
#define TIMETOSTART 1000

#define TIMEMAX 1501

#define BASETIME 500

#define ESPERAR         0
#define JUEGO           1
#define JUEGOTERMINADO  2
#define TECLAS          3

/**
 * @brief Defino el intervalo entre lecturas para "filtrar" el ruido del Botón
 * 
 */
#define INTERVAL    40
typedef struct{
    unsigned char b0:1; 
    unsigned char b1:1;
    unsigned char b2:1;
    unsigned char b3:1;
    unsigned char b4:1;
    unsigned char b6:1;
    unsigned char b7:1;
}_sFlags;
/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,    //0
    BUTTON_UP,      //1
    BUTTON_FALLING, //2
    BUTTON_RISING   //3
}_eButtonState;


typedef struct{
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;

_sFlags myFlags;
_sTeclas ourButton[NROBOTONES];
// 0001 , 0010,  0100, 1000
uint16_t mask[]={0x0001,0x0002,0x0004,0x0008,0x000F};

uint8_t estadoJuego=ESPERAR;


/**
 * @brief Dato del tipo Enum para asignar los estados de la MEF
 * 
 */
_eButtonState myButton;

/**
 * @brief Inicializa la MEF
 * Le dá un estado inicial a myButton
 */
void startMef(uint8_t indice);

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 * @param buttonState Este parámetro le pasa a la MEF el estado del botón.
 */
void actuallizaMef(uint8_t indice );

/**
 * @brief Función para cambiar el estado del LED cada vez que sea llamada.
 * 
 */
void togleLed(uint8_t indice,uint8_t estate);

DigitalOut LEDHerbit(PC_13);
BusIn botones(PB_6,PB_7,PB_8,PB_9);
BusOut leds(PB_12,PB_13,PB_14,PB_15);

Timer miTimer; //!< Timer para hacer la espera de 40 milisegundos

int tiempoMs=0; //!< variable donde voy a almacenar el tiempo del timmer una vez cumplido

int main()
{
    miTimer.start();
    uint16_t ledAuxRandom=0;
    int tiempoHb=0;
    int tiempoRandom=0;
    int tiempoFin=0;
    int ledAuxRandomTime=0;
    int ledAuxJuegoStart=0;
    int8_t contador=0;
    uint8_t indiceAux=0;
    for(uint8_t indice=0; indice<NROBOTONES;indice++){
        startMef(indice);
    }

    while(ON)
    {
            
        if( (miTimer.read_ms()-tiempoHb) > TIMEHB){ //para encender el led de la bluepill
            LEDHerbit=!LEDHerbit;
            tiempoHb=miTimer.read_ms();
        }  
        
        switch(estadoJuego){
            case ESPERAR:
                if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
                    tiempoMs=miTimer.read_ms();
                    for(uint8_t indice=0; indice<NROBOTONES;indice++){
                        actuallizaMef(indice);
                        if(ourButton[indice].timeDiff >= TIMETOSTART){
                            srand(miTimer.read_us());
                            estadoJuego=TECLAS;   
                        }
                    }
                }
            break;
            case TECLAS:
                for( indiceAux=0; indiceAux<NROBOTONES;indiceAux++){
                    actuallizaMef(indiceAux);
                    if(ourButton[indiceAux].estado!=BUTTON_UP){
                        break;
                    }
                        
                }
                if(indiceAux==NROBOTONES){
                    estadoJuego=JUEGO; //cambia el estado a juego 
                    leds=15;//prende todos los leds 
                    ledAuxJuegoStart=miTimer.read_ms(); //iguala el juego start al tiempo actual 
                    for(int i=0;i<NROBOTONES;i++){ //resetea los botones 
                        startMef(i);   
                    }
                }
            break;
            case JUEGO:
                if(leds==0){
                    if((miTimer.read_ms()-ledAuxJuegoStart)> TIMETOSTART){ //espera 1000 ms para empezar el juego 
                        myFlags.b3=1;                        
                    }    
                    if(myFlags.b3){    //inicia el juego 
                        ledAuxRandom = rand() % (MAXLED);
                        ledAuxRandomTime = (rand() % (TIMEMAX)) + BASETIME;
                        tiempoRandom=miTimer.read_ms();
                        leds=mask[ledAuxRandom];
                        
                    }
                }else{ //si Hay leds que se encendieron 
                    if(leds==15) { //si estan todos encendidos 
                        if((miTimer.read_ms()-ledAuxJuegoStart)> TIMETOSTART){ //cuenta que pase 1 segundo 
                            ledAuxJuegoStart=miTimer.read_ms(); //iguala el timer para la resta 
                            leds=0; //apago el led 
                            myFlags.b3=0;//paso la flag a 0 para que despues espere otro segundo 
                        }
                    }
                    for(int i=0;i<NROBOTONES;i++){
                        actuallizaMef(i);

                        if(ourButton[i].estado == BUTTON_DOWN){ // si el boton esta presionado 
                            if(i==ledAuxRandom){//gana
                                myFlags.b1=1;
                                estadoJuego=JUEGOTERMINADO;
                            }else{//pierde
                                myFlags.b1=0;
                                estadoJuego=JUEGOTERMINADO;
                            }
                        
                        }
                    }   
                                        
                    if((miTimer.read_ms() - tiempoRandom > ledAuxRandomTime) && myFlags.b3){                           
                            myFlags.b1=0;
                            estadoJuego=JUEGOTERMINADO;
                    }
                    if(estadoJuego==JUEGOTERMINADO){
                        leds=OFF;
                        tiempoFin=miTimer.read_ms(); //inicializa la variable tiempoFIN para que tenga el valor actual del timer 
                    }
                
                }
            
            break;
            case JUEGOTERMINADO:
                    if(myFlags.b1){//Si gano (destello 4 leds 3 veces por 500ms)
                            if(miTimer.read_ms()-tiempoFin > BASETIME){
                                    myFlags.b0=!myFlags.b0;
                                    togleLed(MAXLED,myFlags.b0);
                                    contador++;
                                tiempoFin=miTimer.read_ms();
                            }    
                    }else{//si perdio (destello ledrandom por 3 veces en 500ms)
                            if(miTimer.read_ms()-tiempoFin > BASETIME){
                                    myFlags.b0=!myFlags.b0;
                                    togleLed(ledAuxRandom,myFlags.b0);
                                    contador++;
                                tiempoFin=miTimer.read_ms();
                            }                              
                    }
                        //resetea todo despues de festejar 
                    if(contador==CICLO){
                                    contador=OFF;
                                    leds=OFF;
                                    estadoJuego=ESPERAR;
                                    tiempoMs=miTimer.read_ms();
                                    myFlags.b3=OFF;
                                    for (int i = 0; i < NROBOTONES; i++)
                                    {
                                        ourButton[i].timeDiff=OFF;
                                        ourButton[i].timeDown=OFF;
                                        startMef(i);
                                    }
                                    
                                }
            
            break;
            default:
                estadoJuego=ESPERAR;
        }

    }
    return 0;
}

void startMef(uint8_t indice){
   ourButton[indice].estado=BUTTON_UP;
}

void actuallizaMef(uint8_t indice){

    switch (ourButton[indice].estado)
    {
    case BUTTON_DOWN:
        if(botones.read() & mask[indice] )
           ourButton[indice].estado=BUTTON_RISING;
    
    break;
    case BUTTON_UP:
        if(!(botones.read() & mask[indice]))
            ourButton[indice].estado=BUTTON_FALLING;
    
    break;
    case BUTTON_FALLING:
        if(!(botones.read() & mask[indice]))
        {
            ourButton[indice].timeDown=miTimer.read_ms();
            ourButton[indice].estado=BUTTON_DOWN;
            //Flanco de bajada
        }
        else
            ourButton[indice].estado=BUTTON_UP;    

    break;
    case BUTTON_RISING:
        if(botones.read() & mask[indice]){
            ourButton[indice].estado=BUTTON_UP;
            //Flanco de Subida
            ourButton[indice].timeDiff=miTimer.read_ms()-ourButton[indice].timeDown;
           /*
            if(ourButton[indice].timeDiff >= TIMETOSTART)
                togleLed(indice);
                */
        }

        else
            ourButton[indice].estado=BUTTON_DOWN;
    
    break;
    
    default:
    startMef(indice);
        break;
    }
}

void togleLed(uint8_t indice,uint8_t estate){
    if(estate)
        leds=mask[indice];
    else 
        leds=OFF;
}