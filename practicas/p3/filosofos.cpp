// -----------------------------------------------------------------------------
// Autor: Víctor García Carrera, victorgarcia@correo.ugr.es
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos.cpp
// Implementación del problema de los filósofos (sin camarero) sin interbloqueo.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,					// num_tenedores = num_filosofos
   num_procesos  = 2*num_filosofos ;

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

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
	// CAMBIAR CALCULO IZQ Y DER PARA CASO CAMARERO, UN PROC MAS
	int peticion,										 //dato a enviar
 		id_ten_izq = (id+1)              % num_procesos, //id. tenedor izq.
 		id_ten_der = (id+num_procesos-1) % num_procesos; //id. tenedor der.
 	MPI_Status  estado ;

	while ( true )
	{
		if ( id == 0 ){	// Para evitar el interbloqueo, implementamos que un filósofo arbitrario (el 0) pida primero el tenedor derecho
			
			cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
			//	Envia un mensaje de petición del tenedor derecho
			MPI_Ssend( &peticion,  1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);
			//	Espera bloqueada hasta recibir confirmación del tenedor derecho
			MPI_Recv ( &peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD, &estado );

			cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
			//	Envia un mensaje de petición del tenedor izquierdo
			MPI_Ssend( &peticion,  1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);
			//	Espera bloqueada hasta recibir confirmación del tenedor izquierdo
			MPI_Recv ( &peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD, &estado );

		}
		else {
			cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
			//	Envia un mensaje de petición del tenedor izquierdo
			MPI_Ssend( &peticion,  1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);
			//	Espera bloqueada hasta recibir confirmación del tenedor izquierdo
			MPI_Recv ( &peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD, &estado );

			cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
			//	Envia un mensaje de petición del tenedor derecho
			MPI_Ssend( &peticion,  1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);
			//	Espera bloqueada hasta recibir confirmación del tenedor derecho
			MPI_Recv ( &peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD, &estado );
		}

		cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
		sleep_for( milliseconds( aleatorio<10,100>() ) );

		cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
		//	Envia un mensaje de liberación del tenedor izquierdo
		MPI_Ssend( &peticion,  1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

		cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
		//	Envia un mensaje de liberación del tenedor derecho
		MPI_Ssend( &peticion,  1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

		cout << "Filosofo " << id << " comienza a pensar" << endl;
		sleep_for( milliseconds( aleatorio<10,100>() ) );
 	}
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
	int valor, id_filosofo ;	// valor recibido, identificador del filósofo
	MPI_Status estado ;			// metadatos de las dos recepciones

	while ( true )
	{

	//	Espera bloqueada hasta recibir una petición de cualquier filósofo
	MPI_Recv ( &valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado );
	//	Podemos utilizar el dato recibido 'valor' para alguna comprobación o uso

	//	Guarda en 'id_filosofo' el id. del emisor
	id_filosofo = estado.MPI_SOURCE;

	cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

	//	Enviamos un mensaje de confirmación al filósofo de que ha cogido el tenedor
	MPI_Ssend( &valor,  1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD);

	//	Espera bloqueada hasta recibir la liberación del filósofo 'id_filosofo'
	MPI_Recv ( &valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado );
	//	Podemos utilizar el dato recibido 'valor' para alguna comprobación o uso

	cout <<"Ten. " <<id <<" ha sido liberado por filo. " <<id_filosofo <<endl ;
	}
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
	  // ejecutar la función correspondiente a 'id_propio'
	  if ( id_propio % 2 == 0 )          // si es par
		 funcion_filosofos( id_propio ); //   es un filósofo
	  else                               // si es impar
		 funcion_tenedores( id_propio ); //   es un tenedor
   }
   else
   {
	  if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
	  { cout << "el número de procesos esperados es:    " << num_procesos << endl
			 << "el número de procesos en ejecución es: " << num_procesos_actual << endl
			 << "(programa abortado)" << endl ;
	  }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
