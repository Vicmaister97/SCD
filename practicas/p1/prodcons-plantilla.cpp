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

const int num_items = 40 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer

int posicion = 0;	// posicion del dato a leer o escribir

unsigned  cont_prod[num_items] = {0},	// contadores de verificación: producidos
          cont_cons[num_items] = {0},	// contadores de verificación: consumidos
	  buffer[tam_vec] = {0};	// buffer donde se almacenarán los datos producidos y a consumir

// inicialización de los semáforos para la sincronización productor-consumidor
Semaphore prod_free = Semaphore(tam_vec);	// semáforo que lleva la cuenta de las posiciones libres (o que ya se han leido y consumido) a escribir en el buffer
Semaphore cons_free = Semaphore(0);		// semáforo que sincroniza al consumidor para consumir los datos cuando los haya


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
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

//FALTA MUTEX DE EXCLUSION A SECCION CRITICA DE POSICION
void  funcion_hebra_productora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
	int dato = producir_dato() ;
	prod_free.sem_wait();		// hacemos down de prod_free para indicar que producimos un dato y esperamos hasta que su escritura no desborde el buffer
	buffer[posicion] = dato;	// escribimos donde indique posicion el dato producido
	posicion++;			// actualizamos posicion para futuras lecturas y escrituras
	cons_free.sem_signal();		// hacemos up de cons_free para indicar al consumidor que tiene un nuevo dato para consumir

   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
	int dato ;
	cons_free.sem_wait();		// hacemos down de cons_free para indicar que consumimos un dato o que espere hasta que haya un dato a consumir
	posicion--;			// actualizamos posicion para poder leer el dato a consumir, el último producido
	dato = buffer[posicion];	// leemos donde indique posicion el dato a consumir
	prod_free.sem_signal();		// hacemos up de prod_free para indicar al productor que ya se ha consumido un dato y puede continuar produciendo
	consumir_dato( dato ) ;
    }
}
//----------------------------------------------------------------------

int main(int argc, char *argv[] )
{
	cout << "--------------------------------------------------------" << endl
	<< "Problema de los productores-consumidores (solución LIFO)." << endl
	<< "--------------------------------------------------------" << endl
	<< flush ;

	thread hebra_productora ( funcion_hebra_productora ),
	hebra_consumidora( funcion_hebra_consumidora );

	hebra_productora.join() ;
	hebra_consumidora.join() ;

	test_contadores();
	return 1;
	/*CASO FIFO ARTIMETICA MODULAR POSICION*/
}
