/* Archivo creado por Victor Garcia Carrera, victorgarcia@correo.ugr.es 
** Ejercicio2 del examen*/

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

const int num_hebras = 8; 		// número de hebras que hay

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


// *****************************************************************************
// clase para monitor Recurso, semántica SU

// declaramos nuestra clase Recurso como derivada (pública) de la clase HoareMonitor
class Recurso : public HoareMonitor
{
 private:					// atributos privados
 // variables permanentes
 int items[2];			// items DISPONIBLES de cada tipo, items[0] del tipo 1, items[1] del tipo 2, inicializados a 2

 CondVar         // variables condicion:
	ocupadasTR1,                //  cola donde esperan las hebras de tipo de recurso 1 (items[0]==0)
	ocupadasTR2 ;               //  cola donde esperan las hebras de tipo de recurso 2 (items[1]==0)

 public:                    // constructor y métodos públicos
	Recurso(  ) ;           // constructor
	void acceder( int tiporecurso );   // funcion que simula el acceso de una hebra a un item de su tipo
	void fin_acceso( int tiporecurso );	// funcion que simula el fin de acceso de una hebra a un item de su tipo
} ;
// -----------------------------------------------------------------------------

Recurso::Recurso(  )
{
	items[0] = 2;
	items[1] = 2;
	ocupadasTR1 = newCondVar();
	ocupadasTR2 = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por las hebras para acceder a su recurso

void Recurso::acceder( int tiporecurso )
{
	mtx.lock();
	cout << "Hebra intenta acceder al recurso de tipo " << tiporecurso << endl << flush;
	mtx.unlock();
	// esperar bloqueado hasta está disponible un item de su tipo, item[tiporecurso-1] > 0
	if ( items[tiporecurso-1] == 0){
		if (tiporecurso == 1)
			ocupadasTR1.wait();			// espera bloqueada de la hebra hasta que esté un item de su tipo
		else
			ocupadasTR2.wait();			// espera bloqueada de la hebra hasta que esté un item de su tipo
	}

	// hacer la operación de acceder al recurso, actualizando estado del monitor
	assert( items[tiporecurso-1] > 0);
	items[tiporecurso-1]--;			// actualizamos el valor de los items disponibles de ese tipo de recurso 

}
// -----------------------------------------------------------------------------

void Recurso::fin_acceso( int tiporecurso )
{
	items[tiporecurso-1]++;		// actualizamos el estado del monitor para indicar que hay un recurso de ese tipo disponible de nuevo
	mtx.lock();
	cout << "Hebra ha terminado de acceder a su recurso, quedan " << items[tiporecurso-1] << " items de tipo " << tiporecurso << endl << flush;
	mtx.unlock();

	// señalamos a la cola de hebras pendientes de ese tipo de recursos que hay un nuevo item disponible
	if (tiporecurso == 1)
		ocupadasTR1.signal();
	else
		ocupadasTR2.signal();

}

// *****************************************************************************
// función de la hebra

void funcion_hebra( MRef<Recurso> monitor, int num_hebra)
{
	int tiporec;
	if ( (num_hebra%2) == 0 ){		// si es una hebra de número par
		tiporec = 1;
	}
	else{							// si es de tipo impar
		tiporec = 2;
	}
	while ( true ){
		monitor->acceder( tiporec );

		// despues de acceder al recurso, calcula milisegundos aleatorios de duración de la acción de acceso
		chrono::milliseconds duracion_espera1( aleatorio<20,200>() );

		// informa de que va a acceder al recurso
		mtx.lock();
		cout << "                  Hebra " << num_hebra << " accede al recurso de tipo " << tiporec << " (" << duracion_espera1.count() << " milisegundos)" << endl;
		mtx.unlock();

		// espera bloqueada un tiempo igual a ''duracion_espera1' milisegundos
		this_thread::sleep_for( duracion_espera1 );

		// informa de que ha terminado de acceder al recurso
		mtx.lock();
		cout << "                  Hebra " << num_hebra << "  : termina de acceder al recurso" << endl;
		mtx.unlock();

		// actualizamos el estado del monitor para indicar que un item de ese tipo se encuentra disponible
		monitor->fin_acceso( tiporec );

		// tras finalizar el acceso, utiliza el recurso
		// calcula milisegundos aleatorios de duración de la acción de utilizar el recurso
		chrono::milliseconds duracion_espera2( aleatorio<20,200>() );

		// informa de que va a acceder al recurso
		mtx.lock();
		cout << "                  Hebra " << num_hebra << " utiliza el recurso (" << duracion_espera2.count() << " milisegundos)" << endl;
		mtx.unlock();

		// espera bloqueada un tiempo igual a ''duracion_espera2' milisegundos
		this_thread::sleep_for( duracion_espera2 );

		// informa de que ha terminado de utilizar el recurso
		mtx.lock();
		cout << "                  Hebra " << num_hebra << "  : termina de utilizar el recurso" << endl;
		mtx.unlock();
		
	}
}
// -----------------------------------------------------------------------------


//----------------------------------------------------------------------

int main(int argc, char *argv[] )
{	
	cout << "----------------------------------------------------------------------" << endl
	<< "Ejercicio2 del examen con monitor SU.	Victor Garcia Carrera" << endl
	<< "----------------------------------------------------------------------" << endl
	<< flush ;

	// crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   	MRef<Recurso> monitor = Create<Recurso>( );
   	int i;
   	thread hebras[num_hebras];

	// crear y lanzar hebras
	for (i = 0 ; i < num_hebras ; i++ ){
		hebras[i] = thread( funcion_hebra, monitor , i);
	}

	// esperar a que terminen las hebras (no pasa nunca)
	for( i = 0 ; i < num_hebras ; i++ )
		hebras[i].join();

	return 1;
}

