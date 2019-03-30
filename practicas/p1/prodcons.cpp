/* Archivo modificado por Victor Garcia Carrera, victorgarcia@correo.ugr.es */

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 80 ,	// número de items
		tam_vec = 10 ;		// tamaño del buffer

int posLibre = 0;	/* primera posicion libre en el buffer para alojar un dato
						CASO LIFO: Indica directamente donde escribir, y posLibre-1 indica donde leer	
						CASO FIFO: Indica directamente donde escribir*/
int posOcupada = 0;	/*  primera posicion ocupada en el buffer para leer un dato
						CASO FIFO: Indica directamente donde leer*/

unsigned cont_prod[num_items] = {0},	// contadores de verificación: producidos
		cont_cons[num_items] = {0},		// contadores de verificación: consumidos
		buffer[tam_vec] = {0};			// buffer donde se almacenarán los datos producidos y a consumir

// inicialización de los semáforos para la sincronización productor-consumidor
Semaphore prod_free = Semaphore(tam_vec);	// semáforo que lleva la cuenta de las posiciones libres (o que ya se han leido y consumido) a escribir en el buffer, inicializado a tam_vec
Semaphore cons_free = Semaphore(0);			// semáforo que sincroniza al consumidor para consumir los datos cuando los haya, inicializado a 0
mutex mtx;									// mutex utilizado para garantizar la exclusión mutua en los accesos al buffer (sección crítica)


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

int producir_dato()
{
	static int contador = 0 ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	cout << "producido: " << contador << endl << flush ;

	cont_prod[contador] ++ ;
	return contador++ ;
}

//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
	assert( dato < num_items );
	cont_cons[dato] ++ ;
	this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

	cout << "                  consumido: " << dato << endl ;
	 
}

//----------------------------------------------------------------------

void test_contadores()
{
	bool ok = true ;
	cout << "comprobando contadores ...." ;
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

//----------------------------------------------------------------------

// --- CASO LIFO ---

void  funcion_hebra_productoraLIFO(  )
{
	for( unsigned i = 0 ; i < num_items ; i++ )
	{
		int dato = producir_dato();
		sem_wait(prod_free);	// hacemos down de prod_free para indicar que producimos un dato y esperamos hasta que su escritura no desborde el buffer
		
		// Inicio sección crítica
		mtx.lock();
		buffer[posLibre] = dato;	// escribimos donde indique posLibre el dato producido
		posLibre++;			// actualizamos posLibre para futuras lecturas y escrituras
		mtx.unlock();
		// Fin sección crítica

		cout << "Insertado en el buffer: " << dato << endl << flush ;
		sem_signal(cons_free);		// hacemos up de cons_free para indicar al consumidor que tiene un nuevo dato para consumir
	}
}

//----------------------------------------------------------------------

void funcion_hebra_consumidoraLIFO(  )
{
	for( unsigned i = 0 ; i < num_items ; i++ )
	{
		int dato;
		sem_wait(cons_free);	// hacemos down de cons_free para indicar que consumimos un dato o que espere hasta que haya un dato a consumir
		
		// Inicio sección crítica
		mtx.lock();
		posLibre--;			// actualizamos posLibre para poder leer el dato a consumir, el último producido
		dato = buffer[posLibre];	// leemos donde indique posLibre el dato a consumir
		mtx.unlock();
		// Fin sección crítica

		cout << "Leido del buffer: " << dato << endl << flush ;
		sem_signal(prod_free);		// hacemos up de prod_free para indicar al productor que ya se ha consumido un dato y hay un nuevo espacio en el buffer
		consumir_dato( dato ) ;
	}
}

//----------------------------------------------------------------------

// --- CASO FIFO ---

void  funcion_hebra_productoraFIFO(  )
{
	for( unsigned i = 0 ; i < num_items ; i++ )
	{
		int dato = producir_dato();
		sem_wait(prod_free);		// hacemos down de prod_free para indicar que producimos un dato y esperamos hasta que su escritura no desborde el buffer
		
		// Inicio sección crítica
		mtx.lock();
		buffer[posLibre] = dato;	// escribimos donde indique posLibre el dato producido
		posLibre++;
		posLibre %= tam_vec;		// actualizamos posLibre para futuras escrituras, aritmética modular al ser una cola circular
		mtx.unlock();
		// Fin sección crítica
		
		cout << "Insertado en el buffer: " << dato << endl << flush ;
		sem_signal(cons_free);		// hacemos up de cons_free para indicar al consumidor que tiene un nuevo dato para consumir
	}
}

//----------------------------------------------------------------------

void funcion_hebra_consumidoraFIFO(  )
{
	for( unsigned i = 0 ; i < num_items ; i++ )
	{
		int dato;
		sem_wait(cons_free);		// hacemos down de cons_free para indicar que consumimos un dato o que espere hasta que haya un dato a consumir
		
		// Inicio sección crítica
		mtx.lock();
		dato = buffer[posOcupada];	// leemos donde indique posOcupada el dato a consumir
		posOcupada++;		
		posOcupada %= tam_vec;		// actualizamos posOcupada para futuras lecturas, aritmética modular al ser una cola circular
		mtx.unlock();
		// Fin sección crítica
		
		cout << "Leido del buffer: " << dato << endl << flush ;
		sem_signal(prod_free);		// hacemos up de prod_free para indicar al productor que ya se ha consumido un dato y hay un nuevo espacio en el buffer
		consumir_dato( dato ) ;
	}
}
//----------------------------------------------------------------------

/*
int main(int argc, char *argv[])
{
	cout << "--------------------------------------------------------" << endl
	<< "Problema de los productores-consumidores (solución LIFO)." << endl
	<< "--------------------------------------------------------" << endl
	<< flush ;

	thread hebra_productora ( funcion_hebra_productoraLIFO ),
	hebra_consumidora( funcion_hebra_consumidoraLIFO );

	hebra_productora.join() ;
	hebra_consumidora.join() ;
	cout << "FIN" << endl << flush ;

	test_contadores();
	return 1;
}
*/

int main(int argc, char *argv[])
{
	cout << "--------------------------------------------------------" << endl
	<< "Problema de los productores-consumidores (solución FIFO)." << endl
	<< "--------------------------------------------------------" << endl
	<< flush ;

	thread hebra_productora ( funcion_hebra_productoraFIFO ),
	hebra_consumidora( funcion_hebra_consumidoraFIFO );

	hebra_productora.join() ;
	hebra_consumidora.join() ;
	cout << "FIN" << endl << flush ;

	test_contadores();
	return 1;
}