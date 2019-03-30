/* Archivo modificado por Victor Garcia Carrera, victorgarcia@correo.ugr.es */

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int tipos_ingred = 3; 		// número de tipos de ingredientes que hay, define cuántos fumadores hay

// inicialización de los semáforos para la sincronización estanquero-fumador
// semáforos que llevan la cuenta de los ingredientes disponibles en el mostrador a consumir

Semaphore ingred[tipos_ingred] = {Semaphore(0), Semaphore(0), Semaphore(0)};	/* 1 representa que ese ingrediente está disponible en el mostrador, 0 que no
																				Todos inicializados a 0, al principio no hay ningún ingrediente */
// semaforo que indica si hay un ingrediente en el mostrador o no
Semaphore mostrador_vacio = Semaphore(1);		// 1 representa que el mostrador está vacio, 0 que está ocupado. Inicializado a 1, al principio mostrador vacío



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
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{
	// calcular milisegundos aleatorios de duración de la acción de fumar)
	chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

	// informa de que comienza a fumar

	cout << "Fumador " << num_fumador << "  :"
		  << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

	// espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
	this_thread::sleep_for( duracion_fumar );

	// informa de que ha terminado de fumar

	cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

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
		fumar(num_fumador);		// tras obtener su ingrediente, fuma. Al estar después del signal de mostrador_vacio, permite a varios fumadores fumar a la vez
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
	hebra_consumidora2( funcion_hebra_fumador, 2 );

	hebra_productora.join();
	hebra_consumidora0.join();
	hebra_consumidora1.join();
	hebra_consumidora2.join();

	return 1;
}
