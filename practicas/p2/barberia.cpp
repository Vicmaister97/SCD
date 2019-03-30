/* Archivo creado por Victor Garcia Carrera, victorgarcia@correo.ugr.es 
** Problema de los barberia con una hebra barbero y num_clients fumadoras
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

const int num_clients = 2; 		// número de clientes que hay

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
// Función que simula la acción de esperar fuera de la barbería, como un retardo aleatoria de la hebra

void EsperarFueraBarberia( int num_client)
{
	// calcular milisegundos aleatorios de duración de la acción de esperar
	chrono::milliseconds duracion_esperar( aleatorio<20,200>() );

	mtx.lock();
	cout << "                  Cliente " << num_client << " espera fuera: " << duracion_esperar.count() << " milisegundos" << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_producir' milisegundos
	this_thread::sleep_for( duracion_esperar );

	mtx.lock();
	cout << "                  Cliente " << num_client << " vuelve a entrar en la barbería " << endl;
	mtx.unlock();

}

//-------------------------------------------------------------------------
// Función que simula la acción de CortarPeloACliente, como un retardo aleatoria de la hebra

void CortarPeloACliente( )
{
	// calcular milisegundos aleatorios de duración de la acción de CortarPeloACliente
	chrono::milliseconds duracion_CortarPeloACliente( aleatorio<20,200>() );

	// informa de que comienza a CortarPeloACliente
	mtx.lock();
	cout << "Barbero comienza a cortarle el pelo al cliente, duración de " << duracion_CortarPeloACliente.count() << " milisegundos" << endl;
	mtx.unlock();

	// espera bloqueada un tiempo igual a ''duracion_CortarPeloACliente' milisegundos
	this_thread::sleep_for( duracion_CortarPeloACliente );

	// informa de que ha terminado de CortarPeloACliente
	mtx.lock();
	cout << "Barbero termina de cortarle el pelo al cliente" << endl;
	mtx.unlock();

}


// *****************************************************************************
// clase para monitor Barberia, semántica SU, un barbero y multiples clientes.

// declaramos nuestra clase Barberia como derivada (pública) de la clase HoareMonitor
class Barberia : public HoareMonitor
{
 private:			// atributos privados
 // variables permanentes
 int cliente;			// cliente sentado en la silla, posibles valores: -1(nadie), [0-num_clients]
 bool silla_vacia;		// booleano que indica si la silla del barbero está vacia

 CondVar         	// colas condicion:
	cola_clientes,      //  cola donde espera los clientes (cortandoPelo.get_nwt() > 0)
	cola_barbero ,      //  cola donde espera el barbero (cola_clientes.get_nwt() > 0)
	cortandoPelo ;		//  cola donde espera el cliente que está siendo atendido hasta que terminan de cortarle el pelo

 public:                    // constructor y métodos públicos
	Barberia(  ) ;           // constructor
	void cortarPelo( int cliente );   // extraer un valor (fumador)
	void siguienteCliente( );	// insertar un valor (estanquero)
	void finCliente( );		// espera bloqueada hasta que el mostrador este vacio
} ;
// -----------------------------------------------------------------------------

Barberia::Barberia(  )
{
	cliente = -1 ;
	silla_vacia = true;
	cola_clientes = newCondVar();
	cola_barbero = newCondVar();
	cortandoPelo = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el cliente para que le atienda el barbero

void Barberia::cortarPelo( int num_client )
{
	mtx.lock();
	cout << "                  Cliente " << num_client << " espera su turno" << endl << flush;
	mtx.unlock();
	// esperar bloqueado hasta que silla_vacia == true
	if ( silla_vacia == false )
		cola_clientes.wait();			// espera bloqueada del cliente hasta que pueda ser atendido

	mtx.lock();
	cout << "                  Cliente " << num_client << " se va a sentar en la silla" << endl << flush;
	mtx.unlock();

	// hacer la operación de atender al cliente, actualizando estado del monitor
	assert( silla_vacia == true);
	silla_vacia = false;			// actualizamos el valor de silla_vacia a false para indicar a los otros posibles clientes que ya hay alguien siendo atendido
	cliente = num_client;			// actualizamos el cliente sentado en la silla

	// señalar al barbero que ya tiene un cliente, despertándole si está dormido
	if (cola_barbero.get_nwt() > 0)
		cola_barbero.signal();

	// espera bloqueada hasta que el barbero termina de cortarle el pelo
	cortandoPelo.wait();
}
// -----------------------------------------------------------------------------
// funcion que simula la espera del barbero de un nuevo cliente

void Barberia::siguienteCliente(  )
{

	if ( cola_clientes.get_nwt() == 0 && silla_vacia == true){		// si no hay clientes en cola de espera o no hay nadie sentado
		mtx.lock();
		cout << "Barbero esperando a un cliente" << endl << flush;
		mtx.unlock();
		cola_barbero.wait();				// espera bloqueada del barbero hasta que haya clientes
	}

	// llama a uno de los clientes para cortarle el pelo
	cola_clientes.signal();

	mtx.lock();
	cout << "Barbero va a atender al cliente " << cliente << endl << flush;
	mtx.unlock();
	
}
// -----------------------------------------------------------------------------
// funcion que simula el aviso al cliente de que puede marcharse y la espera hasta que lo haga

void Barberia::finCliente( )
{
	mtx.lock();
	cout << "Barbero informa al cliente de que ha finalizado el corte de pelo" << endl << flush;
	mtx.unlock();

	// avisa al cliente de que ha terminado de cortarle el pelo
	cortandoPelo.signal();

	// indica que la silla está libre
	silla_vacia = true;

}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_barbero( MRef<Barberia> monitor )
{
	while( true ){
		monitor->siguienteCliente();
		CortarPeloACliente();
		monitor->finCliente();
	}
}
// -----------------------------------------------------------------------------

void funcion_hebra_cliente( MRef<Barberia> monitor, int num_client)
{
	while ( true ){
		monitor->cortarPelo( num_client );
		EsperarFueraBarberia( num_client ) ;
	}
}
// -----------------------------------------------------------------------------


//----------------------------------------------------------------------

int main(int argc, char *argv[] )
{	
	cout << "----------------------------------------------------------------------" << endl
	<< "Problema del barbero durmiente con monitor SU.	Victor Garcia Carrera" << endl
	<< "----------------------------------------------------------------------" << endl
	<< flush ;

	// crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   	MRef<Barberia> monitor = Create<Barberia>( );
   	int num_hebras = num_clients + 1;
   	int i;
   	thread hebras[num_hebras];

	// crear y lanzar hebras
	hebras[0] = thread ( funcion_hebra_barbero, monitor);

	for (i = 0 ; i < num_clients ; i++ ){
		hebras[i+1] = thread( funcion_hebra_cliente, monitor , i);
	}

	// esperar a que terminen las hebras (no pasa nunca)
	for( i = 0 ; i < num_hebras ; i++ )
		hebras[i].join();

	return 1;
}
