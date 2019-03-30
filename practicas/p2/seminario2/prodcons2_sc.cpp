// -----------------------------------------------------------------------------
//
// AUTOR: VICTOR GARCIA CARRERA, victorgarcia@correo.ugr.es
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons2_sc.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con múltiples productores y consumidores.
// Opcion LIFO (stack) y FIFO
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std ;

constexpr int
	num_items = 200 ,	// número de items a producir/consumir
	num_prods = 4,		// número de productores
	num_consumidores = 2;		// número de consumidores

unsigned productores[num_prods] = {0};	// array que indica en cada momento el número de items producidos por cada hebra productora

mutex
	mtx ;                 // mutex de escritura en pantalla
unsigned
	cont_prod[num_items], // contadores de verificación: producidos
	cont_cons[num_items]; // contadores de verificación: consumidos

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato( unsigned num_prod)
{
	int valor = productores[num_prod] + (num_prod*(num_items/num_prods));	// produce el valor correspondiente

	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
	mtx.lock();
	cout << "producido: " << valor << " por el productor ( " << num_prod << " )" << endl << flush ;
	mtx.unlock();
	productores[num_prod]++;
	cont_prod[valor]++;
	return valor;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato, unsigned num_cons )
{
	if ( num_items <= dato )
	{
		cout << " dato === " << dato << ", num_items == " << num_items << endl ;
		assert( dato < num_items );
	}
	cont_cons[dato] ++ ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
	mtx.lock();
	cout << "                  consumido: " << dato << " por el consumidor ( " << num_cons << " )" << endl ;
	mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
	for( unsigned i = 0 ; i < num_items ; i++ )
	{  cont_prod[i] = 0 ;
		cont_cons[i] = 0 ;
	}
}

//----------------------------------------------------------------------

void test_contadores()
{
	bool ok = true ;
	cout << "comprobando contadores ...." << flush ;

	for( unsigned i = 0 ; i < num_items ; i++ )
	{
		if ( cont_prod[i] != 1 )
		{
			cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
			ok = false ;
		}
		if ( cont_cons[i] != 1 )
		{
			cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
			ok = false ;
		}
	}
	if (ok)
		cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, multiples prod. y cons.

class ProdCons2SC
{
 private:
 static const int           // constantes:
	num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
	buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
	primera_libre ;          //  indice de celda de la próxima inserción
 mutex
	cerrojo_monitor ;        // cerrojo del monitor
 condition_variable         // colas condicion:
	ocupadas,                //  cola donde espera el consumidor (n>0)
	libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
	ProdCons2SC(  ) ;           // constructor
	int  leer();                // extraer un valor (sentencia L) (consumidor)
	void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons2SC::ProdCons2SC(  )
{
	primera_libre = 0 ;
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons2SC::leer(  )
{
	// ganar la exclusión mutua del monitor con una guarda
	unique_lock<mutex> guarda( cerrojo_monitor );

	// esperar bloqueado hasta que 0 < primera_libre
	while ( primera_libre == 0 )	// necesario bucle while para que cuando sale la hebra compruebe de nuevo que se cumple la condición (por la semántica SC puede que ya no)
		ocupadas.wait( guarda );

	// hacer la operación de lectura, actualizando estado del monitor
	assert( 0 < primera_libre  );
	primera_libre-- ;
	const int valor = buffer[primera_libre] ;

	// señalar al productor que hay un hueco libre, por si está esperando
	libres.notify_one();

	mtx.lock();
	cout << "                  Leido del buffer: " << valor << endl ;
	mtx.unlock();

	// devolver valor
	return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons2SC::escribir( int valor )
{
	// ganar la exclusión mutua del monitor con una guarda
	unique_lock<mutex> guarda( cerrojo_monitor );

	// esperar bloqueado hasta que primera_libre < num_celdas_total
	while ( primera_libre == num_celdas_total )		// necesario bucle while para que cuando sale la hebra compruebe de nuevo que se cumple la condición (por la semántica SC puede que ya no)
		libres.wait( guarda );

	//cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
	assert( primera_libre < num_celdas_total );

	// hacer la operación de inserción, actualizando estado del monitor
	buffer[primera_libre] = valor ;
	primera_libre++ ;

	mtx.lock();
	cout << "Escrito en el buffer: " << valor << endl ;
	mtx.unlock();

	// señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
	ocupadas.notify_one();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( ProdCons2SC * monitor, unsigned num_prod )
{

	for( unsigned i = 0 ; i < (num_items/num_prods) ; i++ )		// produce el numero de veces que le corresponde
	{
		int valor = producir_dato(num_prod) ;
		monitor->escribir( valor );
	}
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( ProdCons2SC * monitor, unsigned num_cons)
{
	for( unsigned i = 0 ; i < (num_items/num_consumidores) ; i++ )	// consume el numero de veces que le corresponde
	{
		int valor = monitor->leer();
		consumir_dato( valor, num_cons ) ;
	}
}
// -----------------------------------------------------------------------------

/*-----------------------------------------------------------------------------*/
// clase para monitor buffer, version FIFO, semántica SC, multiples prods. y cons.

class ProdCons2SCFIFO
{
 private:
 static const int           // constantes:
	num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
	buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
	num_celdas_ocupadas,     //  numero de datos producidos en el buffer sin consumir
	primera_libre ,          //  indice de celda de la próxima inserción
	primera_ocupada;     /*    primera posicion ocupada en el buffer para leer un dato
											CASO FIFO: Indica directamente donde leer*/
 mutex
	cerrojo_monitor ;        // cerrojo del monitor
 condition_variable         // colas condicion:
	ocupadas,                //  cola donde espera el consumidor (n>0)
	libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
	ProdCons2SCFIFO(  ) ;           // constructor
	int  leer();                // extraer un valor (sentencia L) (consumidor)
	void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons2SCFIFO::ProdCons2SCFIFO(  )
{
	num_celdas_ocupadas = 0;
	primera_libre = 0 ;
	primera_ocupada = 0;
}

// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons2SCFIFO::leer(  )
{
	// ganar la exclusión mutua del monitor con una guarda
	unique_lock<mutex> guarda( cerrojo_monitor );

	// esperar bloqueado hasta que 0 < num_celdas_ocupadas
	while ( num_celdas_ocupadas == 0 )
		ocupadas.wait( guarda );

	// hacer la operación de lectura, actualizando estado del monitor
	const int valor = buffer[primera_ocupada] ;
	primera_ocupada++;
	primera_ocupada %= num_celdas_total;	// actualizamos primera_libre para futuras escrituras, aritmética modular al ser una cola circular
	num_celdas_ocupadas--;					// actualizamos el numero de celdas ocupadas

	// señalar al productor que hay un hueco libre, por si está esperando
	libres.notify_one();

	mtx.lock();
	cout << "                  Leido del buffer: " << valor << endl ;
	mtx.unlock();

	// devolver valor
	return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons2SCFIFO::escribir( int valor )
{
	// ganar la exclusión mutua del monitor con una guarda
	unique_lock<mutex> guarda( cerrojo_monitor );

	// esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
	while ( num_celdas_ocupadas == num_celdas_total )
		libres.wait( guarda );

	//cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
	assert( num_celdas_ocupadas < num_celdas_total );

	// hacer la operación de inserción, actualizando estado del monitor
	buffer[primera_libre] = valor ;		// escribimos donde indique primera_libre el dato producido
	primera_libre++;
	primera_libre %= num_celdas_total;	// actualizamos primera_libre para futuras escrituras, aritmética modular al ser una cola circular

	num_celdas_ocupadas++;				// actualizamos el numero de celdas ocupadas

	mtx.lock();
	cout << "Escrito en el buffer: " << valor << endl ;
	mtx.unlock();

	// señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
	ocupadas.notify_one();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productoraFIFO( ProdCons2SCFIFO * monitor, unsigned num_prod )
{

	for( unsigned i = 0 ; i < (num_items/num_prods) ; i++ )
	{
		int valor = producir_dato(num_prod) ;
		monitor->escribir( valor );
	}
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidoraFIFO( ProdCons2SCFIFO * monitor, unsigned num_cons  )
{
	for( unsigned i = 0 ; i < (num_items/num_consumidores) ; i++ )
	{
		int valor = monitor->leer();
		consumir_dato( valor, num_cons ) ;
	}
}
// -----------------------------------------------------------------------------

/*
int main()
{
	cout << "-------------------------------------------------------------------------------" << endl
		  << "Problema de los productores-consumidores (multi prod/cons, Monitor SC, buffer LIFO). " << endl
		  << "-------------------------------------------------------------------------------" << endl
		  << flush ;

	ProdCons2SC monitor ;
	unsigned i;
	int num_hebras = num_prods + num_consumidores;
	thread hebras[num_hebras];

	// crear y lanzar hebras
	for ( i = 0; i < num_prods ; i++ ){
		hebras[i] = thread ( funcion_hebra_productora, &monitor , i);
	}

	for ( ; i < num_hebras ; i++ ){
		hebras[i] = thread( funcion_hebra_consumidora, &monitor , i-num_prods);
	}

	// esperar a que terminen las hebras (no pasa nunca)
	for( i = 0 ; i < num_hebras ; i++ )
		hebras[i].join();

	// comprobar que cada item se ha producido y consumido exactamente una vez
	test_contadores() ;
}
*/


// -----------------------------------------------------------------------------
int main()
{
	cout << "-------------------------------------------------------------------------------" << endl
		  << "Problema de los productores-consumidores (multi prod/cons, Monitor SC, buffer FIFO). " << endl
		  << "-------------------------------------------------------------------------------" << endl
		  << flush ;

	ProdCons2SCFIFO monitor ;
	unsigned i;
	int num_hebras = num_prods + num_consumidores;
	thread hebras[num_hebras];

	// crear y lanzar hebras
	for ( i = 0; i < num_prods ; i++ ){
		hebras[i] = thread ( funcion_hebra_productoraFIFO, &monitor , i);
	}

	for ( ; i < num_hebras ; i++ ){
		hebras[i] = thread( funcion_hebra_consumidoraFIFO, &monitor , i-num_prods);
	}

	// esperar a que terminen las hebras (no pasa nunca)
	for( i = 0 ; i < num_hebras ; i++ )
		hebras[i].join();

	// comprobar que cada item se ha producido y consumido exactamente una vez
	test_contadores() ;
}