/* Archivo creado por Victor Garcia Carrera, victorgarcia@correo.ugr.es 
/* Examen Practica1 SCD	*/

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int tipos_ingred = 3; 					// número de tipos de ingredientes que hay, define cuántos fumadores hay
int tam_buzon = 5;
int buzon[5] = {-1, -1, -1, -1, -1};	// -1 indica que el buzón esta vacio. El array se llenará con los números de los fumadores que introduzcan un sobre
int posLibre = 0;	// primera posicion libre en el buffer para alojar un dato. Indica donde insertar el sobre en el buzón
int posOcupada = 0;	//  primera posicion ocupada en el buffer para leer un dato. Indica donde extraer el sobre en el buzón

int iter = 1;					// iteraciones traficante para gestionar la impresión por pantalla cada 4 iteraciones
int pos = -1;					// para leer el sobre en el buzon
int recogidos[3] = {0, 0, 0};	// para mantener una cuenta de los sobres aportados por cada fumador

// inicialización de los semáforos para la sincronización estanquero-fumador
// semáforos que llevan la cuenta de los ingredientes disponibles en el mostrador a consumir

Semaphore ingred[tipos_ingred] = {Semaphore(0), Semaphore(0), Semaphore(0)};	/* 1 representa que ese ingrediente está disponible en el mostrador, 0 que no
																				Todos inicializados a 0, al principio no hay ningún ingrediente */
// semaforo que indica si hay un ingrediente en el mostrador o no
Semaphore mostrador_vacio = Semaphore(1);		// 1 representa que el mostrador está vacio, 0 que está ocupado. Inicializado a 1, al principio mostrador vacío

Semaphore fumador_free = Semaphore(tam_buzon);	// semáforo que lleva la cuenta de las posiciones libres a escribir en el buffer, inicializado a tam_buzon
Semaphore buzon_free = Semaphore(0);			// semáforo que sincroniza al traficante para coger los sobres cuando los haya, inicializado a 0
mutex mtx;		// mutex para controlar el acceso al buzón secreto (sección crítica)


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
	int producto;
	while ( true ){
		sem_wait(mostrador_vacio);					// antes de producir, el estanquero comprueba que el mostrador está vacio
		cout << "Mostrador vacio, se va a generar un nuevo ingrediente" << endl;
		producto = aleatorio<0, tipos_ingred-1>();	// produce un nuevo ingrediente
		cout << "El ingrediente " << producto
		 << " se encuentra disponible" << endl;
		sem_signal(ingred[producto]);				// señaliza al fumador correspondiente que su ingrediente está disponible
	}
}

//-------------------------------------------------------------------------
// Función que simula la acción de introducir un sobre en el buzón secreto

void traficar( int num_fumador )
{
	sem_wait(fumador_free);		// hacemos down de fumador_free para indicar que esperamos hasta que un fumador pueda insertar su sobre en el buzón
		
	// Inicio sección crítica
	mtx.lock();
	buzon[posLibre] = num_fumador;		// escribimos donde indique posLibre el sobre producido
	posLibre++;
	posLibre %= tam_buzon;		// actualizamos posLibre para futuras escrituras, aritmética modular al ser una cola circular
	mtx.unlock();
	// Fin sección crítica
	
	cout << "El fumador " << num_fumador << " ha introducido su sobre" << endl << flush ;
	sem_signal(buzon_free);		// hacemos up de buzon_free para indicar al traficante que tiene un nuevo sobre para coger
	
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void funcion_hebra_fumador( int num_fumador )
{
	while( true )
	{
		cout << "Fumador " << num_fumador << " esperando a su ingrediente" << endl;
		sem_wait(ingred[num_fumador]);		// el fumador espera hasta que su ingrediente esté disponible
		cout << "El fumador " << num_fumador << " ha retirado su ingrediente" << endl;
		sem_signal(mostrador_vacio);	// el fumador indica al estanquero que ha retirado su ingrediente
		traficar(num_fumador);		// tras obtener su ingrediente, fuma. Al estar después del signal de mostrador_vacio, permite a varios fumadores fumar a la vez
	}
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del traficante
void funcion_hebra_traficante(  )
{
	while( true )
	{
		// calcular milisegundos aleatorios de duración de la acción de esperar
		chrono::milliseconds duracion_viaje( aleatorio<100,300>() );
		// espera bloqueada un tiempo igual a ''duracion_viaje' milisegundos
		this_thread::sleep_for( duracion_viaje );
		cout << "El traficante vuelve al pais" << endl;

		sem_wait(buzon_free);		// hacemos down de buzon_free para indicar que cogemos un sobre o espera a que haya uno
		
		// Inicio sección crítica
		mtx.lock();
		pos = buzon[posOcupada];	// leemos donde indique posOcupada el sobre
		recogidos[pos]++;			// actualizamos el numero de sobres de ese fumador
		posOcupada++;		
		posOcupada %= tam_buzon;		// actualizamos posOcupada para futuras lecturas, aritmética modular al ser una cola circular
		mtx.unlock();
		// Fin sección crítica

		sem_signal(fumador_free);		// hacemos up de fumador_free para indicar a los fumadores que ya se ha cogido un sobre y hay un nuevo espacio en el buzon

		if (iter == 4){		// tras 4 iteraciones
			for (pos = 0; pos < 3; pos++){
				cout << "El fumador " << pos << " ha aportado " << recogidos[pos] << " sobres" << endl;
				recogidos[pos] = 0;		// reestablecemos el numero de sobres aportados
			}
			iter = 0;		// reestablecemos las iteraciones
		}

		iter++;
	}
}

//----------------------------------------------------------------------

int main(int argc, char *argv[] )
{	
	cout << "--------------------------------------------------------" << endl
	<< "Problema de los fumadores." << endl
	<< "--------------------------------------------------------" << endl
	<< flush ;

	thread hebra_productora ( funcion_hebra_estanquero ),
	hebra_consumidora0( funcion_hebra_fumador, 0 ),
	hebra_consumidora1( funcion_hebra_fumador, 1 ),
	hebra_consumidora2( funcion_hebra_fumador, 2 ),
	hebra_traficante (funcion_hebra_traficante);

	hebra_productora.join();
	hebra_consumidora0.join();
	hebra_consumidora1.join();
	hebra_consumidora2.join();
	hebra_traficante.join();

	return 1;
}
