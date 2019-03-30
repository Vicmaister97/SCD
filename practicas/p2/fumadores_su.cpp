/* Archivo creado por Victor Garcia Carrera, victorgarcia@correo.ugr.es 
** Problema de los fumadores con una hebra estanquera y tres fumadoras
** implementado con monitores con semántica SU (Señalar y Espera Urgente)*/

#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <condition_variable>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

const int tipos_ingred = 3; 		// número de tipos de ingredientes que hay, define cuántos fumadores hay

mutex
	mtx ;                 // mutex de escritura en pantalla

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
// Función que produce un nuevo ingrediente

int producir_ingrediente(  )
{
	int producto;

	// calcular milisegundos aleatorios de duración de la acción de producir
	chrono::milliseconds duracion_producir( aleatorio<20,200>() );

	mtx.lock();
	cout << "Se va a generar un ingrediente (" << duracion_producir.count() << " milisegundos)" << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_producir' milisegundos
	this_thread::sleep_for( duracion_producir );

	producto = aleatorio<0, tipos_ingred-1>();	// produce un nuevo ingrediente

	mtx.lock();
	cout << "Ingrediente generado: " << producto << endl;
	mtx.unlock();

	return producto;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{
	// calcular milisegundos aleatorios de duración de la acción de fumar
	chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

	// informa de que comienza a fumar
	mtx.lock();
	cout << "Fumador " << num_fumador << "  :"
		  << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
	this_thread::sleep_for( duracion_fumar );

	// informa de que ha terminado de fumar
	mtx.lock();
	cout << "Fumador " << num_fumador << "  : termina de fumar" << endl;
	mtx.unlock();

}


// *****************************************************************************
// clase para monitor Estanco, semántica SU, un productor y multiples consumidores.

// declaramos nuestra clase Estanco como derivada (pública) de la clase HoareMonitor
class Estanco : public HoareMonitor
{
 private:					// atributos privados
 // variables permanentes
 int ingrediente;			// ingrediente presente en el mostrador, posibles valores: -1(inic), [0-2]
 bool mostrador_vacio;	// booleano que indica si hay un ingrediente en el mostrador

 CondVar         // colas condicion:
	ocupadas,                //  cola donde espera los fumadores (mostrador_vacio=true)
	libres ;                 //  cola donde espera el estanquero (mostrador_vacio=false)

 public:                    // constructor y métodos públicos
	Estanco(  ) ;           // constructor
	void obtenerIngrediente( int ingred );   // extraer un valor (fumador)
	void poner_ingrediente( int valor );	// insertar un valor (estanquero)
	void esperarRecogidaIngrediente( );		// espera bloqueada hasta que el mostrador este vacio
} ;
// -----------------------------------------------------------------------------

Estanco::Estanco(  )
{
	ingrediente = -1 ;
	mostrador_vacio = true;
	ocupadas = newCondVar();
	libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el fumador para coger un ingrediente

void Estanco::obtenerIngrediente( int ingred )
{
	mtx.lock();
	cout << "                  Fumador " << ingred << " espera su ingrediente" << endl << flush;
	mtx.unlock();
	// esperar bloqueado hasta que mostrador_vacio == false && ingred == mostrador.ingrediente
	while ( mostrador_vacio == true || ingrediente != ingred)	// bucle para asegurar que el fumador solo sale cuando se cumplen ambas condiciones
		ocupadas.wait();			// espera bloqueada del fumador hasta que esté su ingrediente

	// hacer la operación de coger el ingrediente, actualizando estado del monitor
	assert( mostrador_vacio == false && ingred == ingrediente);
	mostrador_vacio = true;			// actualizamos el valor de mostrador_vacio a true para indicar que ya no hay ningun ingrediente disponible

	mtx.lock();
	cout << "                  Fumador " << ingred << " ha recogido su ingrediente" << endl << flush;
	mtx.unlock();

	// señalar al estanquero que el mostrador está libre, por si está esperando
	libres.signal();
}
// -----------------------------------------------------------------------------

void Estanco::poner_ingrediente( int valor )
{
	int i;
	// hacer la operación de poner el ingrediente, actualizando estado del monitor
	ingrediente = valor;
	mostrador_vacio = false ;

	mtx.lock();
	cout << "Ingrediente puesto: " << valor << endl << flush;
	mtx.unlock();

	// señalar a todos los fumadores que ya hay un ingrediente disponible (por si estan esperando)
	for (i = 0; i < tipos_ingred; i++)
		if ( mostrador_vacio == true)		// si se ha notificado al indicado
			break;
		ocupadas.signal();

}
// -----------------------------------------------------------------------------
void Estanco::esperarRecogidaIngrediente( )
{
	while ( mostrador_vacio == false )	// mientras que no esté vacío el mostrador
		libres.wait();					// espera bloqueada del estanquero hasta que pueda poner el ingrediente

	assert( mostrador_vacio == true );

}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
	int valor;
	while( true ){
		valor = producir_ingrediente() ;
		monitor->poner_ingrediente( valor );
		monitor->esperarRecogidaIngrediente();
	}
}
// -----------------------------------------------------------------------------

void funcion_hebra_fumador( MRef<Estanco> monitor, int num_cons)
{
	while ( true ){
		monitor->obtenerIngrediente( num_cons );
		fumar( num_cons ) ;
	}
}
// -----------------------------------------------------------------------------


//----------------------------------------------------------------------

int main(int argc, char *argv[] )
{	
	cout << "----------------------------------------------------------------------" << endl
	<< "Problema de los fumadores con monitor SU.	Victor Garcia Carrera" << endl
	<< "----------------------------------------------------------------------" << endl
	<< flush ;

	// crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   	MRef<Estanco> monitor = Create<Estanco>( );

	thread hebra_estanquero ( funcion_hebra_estanquero, monitor),
	hebra_fumador0( funcion_hebra_fumador, monitor, 0 ),
	hebra_fumador1( funcion_hebra_fumador, monitor, 1 ),
	hebra_fumador2( funcion_hebra_fumador, monitor, 2 );

	// espera a que terminen, nunca pasa
	hebra_estanquero.join();
	hebra_fumador0.join();
	hebra_fumador1.join();
	hebra_fumador2.join();

	return 1;
}
