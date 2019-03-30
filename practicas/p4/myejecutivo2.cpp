// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.

// Modificado por: Víctor García Carrera, victorgarcia@correo.ugr.es
//
// Archivo: ejecutivo1-compr.cpp
// Implementación del primer ejemplo de ejecutivo cíclico:
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  250  100
//   B  250   80
//   C  500   50
//   D  500   40
//   E 1000   20
//  -------------
//
//  Planificación (con Ts == 250 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D E  | A B C   | A B D  |
//  *---------*----------*---------*--------*
//
//
// Historial:
// Creado en Diciembre de 2017
// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

#define MAX_DELAY 20	// retardo maximo en un ciclo secundario

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds(150) );  }
void TareaC() { Tarea( "C", milliseconds(200) );  }
void TareaD() { Tarea( "D", milliseconds(240) );  }		// si aumenta a 250, la planificación ya no es posible por la necesidad de un caso 2

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario
   const milliseconds Ts( 500 );

   milliseconds_f ciclo = Ts;

   while( true ) // ciclo principal
   {
	  cout << endl
		   << "---------------------------------------" << endl
		   << "Comienza iteración del ciclo principal." << endl ;

	  for( int i = 1 ; i <= 4 ; i++ ) // ciclos secundarios (4 iteraciones)
	  {
		 cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

		 // ini_ms = instante de inicio de la iteración actual del ciclo secundario
   		 time_point<steady_clock> ini_ms = steady_clock::now();

		 switch( i )
		 {
			case 1 : TareaA(); TareaB(); TareaC();           break ;
			case 2 : TareaA(); TareaB(); TareaD(); 			 break ;	// caso de minimo tiempo de espera despues de realizar las tareas del ciclo
			case 3 : TareaA(); TareaB(); 		             break ;
			case 4 : TareaA(); TareaB(); TareaC();           break ;
		 }

		 /// end_ms = instante final de la iteración actual del ciclo secundario
		 time_point<steady_clock> end_ms = steady_clock::now();

		 // duracion = tiempo real transcurrido en este ciclo secundario
		 steady_clock::duration duracion = end_ms - ini_ms ;

		 float retardo = milliseconds_f(duracion).count() - ciclo.count();

		 // ’duracion’ tiene unidades desconocidas, para imprimirlo lo convertimos a milisegundos:
		 cout << endl
			<< "Retardo del ciclo == " << retardo << " milisegundos." << endl ;

		 // la duracion esperada de un ciclo secundario es de Ts milisegundos
		 // si la duracion real del ciclo supera la duracion esperada + MAX_DELAY, el programa aborta
		 if ( milliseconds_f(duracion).count() > Ts.count() + MAX_DELAY ){
		 	cerr << "ERROR: superado MAX_DELAY (20 ms)" << endl;
			return -1;
		 }

		 // si la duracion del ciclo ha sido menor de la esperada
		 if ( milliseconds_f(duracion).count() < Ts.count() ){
		 	// fin_esperado = instante esperado de final del ciclo
   		 	time_point<steady_clock> fin_esperado = ini_ms + Ts ;
		 	sleep_until( fin_esperado ); // espera hasta completar el tiempo de un ciclo secundario
		 }

	  }

   }
}
