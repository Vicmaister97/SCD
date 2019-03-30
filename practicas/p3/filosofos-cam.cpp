// -----------------------------------------------------------------------------
// Autor: Víctor García Carrera, victorgarcia@correo.ugr.es
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-cam.cpp
// Implementación del problema de los filósofos con camarero sin interbloqueo.
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
	num_procesos  = 2*num_filosofos + 1,
	id_camarero = 10,					// identificador del camarero
	etiq_sentarse = 0,				// etiqueta utilizada para los mensajes de peticion de sentarse a la mesa
	etiq_levantarse = 1;				// etiqueta utilizada para los mensajes de peticion de levantarse de la mesa
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
		id_ten_izq = (id+1)              % (num_procesos-1), //id. tenedor izq.
		id_ten_der = (id+num_procesos-2) % (num_procesos-1); //id. tenedor der.
	MPI_Status  estado ;

	// Espera bloqueada inicial para esperar a que esté el camarero antes de todo el proceso
	//sleep_for( milliseconds( 20 ) );

	while ( true )
	{
		// SENTARSE EN LA MESA
		cout <<"Filósofo " <<id << " solicita SENTARSE " <<endl;
		//	Envia un mensaje de petición del tenedor izquierdo
		MPI_Ssend( &peticion,  1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);
		//	Espera bloqueada hasta recibir confirmación del tenedor izquierdo
		MPI_Recv ( &peticion, 1, MPI_INT, id_camarero, 0, MPI_COMM_WORLD, &estado );

		// COGER TENEDORES
		cout <<"Filósofo " <<id << " solicita ten. izq. " <<id_ten_izq <<endl;
		//	Envia un mensaje de petición del tenedor izquierdo
		MPI_Ssend( &peticion,  1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);
		//	Espera bloqueada hasta recibir confirmación del tenedor izquierdo
		MPI_Recv ( &peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD, &estado );

		cout <<"Filósofo " <<id <<" solicita ten. der. " <<id_ten_der <<endl;
		//	Envia un mensaje de petición del tenedor derecho
		MPI_Ssend( &peticion,  1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);
		//	Espera bloqueada hasta recibir confirmación del tenedor derecho
		MPI_Recv ( &peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD, &estado );

		// COMER
		cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
		sleep_for( milliseconds( aleatorio<10,100>() ) );

		// SOLTAR TENEDORES
		cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
		//	Envia un mensaje de liberación del tenedor izquierdo
		MPI_Ssend( &peticion,  1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

		cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
		//	Envia un mensaje de liberación del tenedor derecho
		MPI_Ssend( &peticion,  1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

		// LEVANTARSE DE LA MESA
		cout <<"Filósofo " <<id << " solicita LEVANTARSE " <<endl;
		//	Envia un mensaje de petición del tenedor izquierdo
		MPI_Ssend( &peticion,  1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);
		//	Espera bloqueada hasta recibir confirmación del tenedor izquierdo
		MPI_Recv ( &peticion, 1, MPI_INT, id_camarero, 0, MPI_COMM_WORLD, &estado );

		// PENSAR
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
	//	Enviamos un mensaje de confirmación al filósofo de que puede coger el tenedor
	MPI_Ssend( &valor,  1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD);

	//	Espera bloqueada hasta recibir la liberación del filósofo 'id_filosofo'
	MPI_Recv ( &valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado );
	//	Podemos utilizar el dato recibido 'valor' para alguna comprobación o uso

	cout <<"Ten. " <<id <<" ha sido liberado por filo. " <<id_filosofo <<endl ;
	}
}
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------

void funcion_camarero( )
{
	int num_sentados = 0,		// contador de los filosofos sentados en la mesa
		valor,
		etiq_emisor_aceptable,	// identificador de emisor aceptable por su etiqueta
		id_filosofo;
	MPI_Status estado ;

	while ( true )
	{
		if (num_sentados >= (num_filosofos-1))	// situacion de INTERBLOQUEO, el camarero impide que esten todos los filosofos sentados a la mesa
			etiq_emisor_aceptable = etiq_levantarse;
		else 	// si aun pueden sentarse filosofos
			etiq_emisor_aceptable = MPI_ANY_TAG;	// aceptamos cualquier peticion entrante
	
		//	Espera bloqueada hasta recibir una petición
		MPI_Recv ( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado );
		//	Podemos utilizar el dato recibido 'valor' para alguna comprobación o uso
		//	Guarda en 'id_filosofo' el id. del emisor
		id_filosofo = estado.MPI_SOURCE;

		// 3. procesar el mensaje recibido

		switch( estado.MPI_TAG ) // leer etiqueta del mensaje en metadatos
		{
		case etiq_levantarse:	// si es una petición de levantarse de la mesa
			cout <<"Camarero permite levantarse a " << id_filosofo <<endl ;
			num_sentados--;		// actualizamos el num de filosofos sentados en la mesa
			MPI_Ssend( &valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD);	// enviamos la confirmación de que puede levantarse
			break;

		case etiq_sentarse: // si es una peticion de sentarse a la mesa
			cout <<"Camarero permite sentarse a " << id_filosofo <<endl ;
			num_sentados++;		// actualizamos el num de filosofos sentados en la mesa
			MPI_Ssend( &valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD);	// enviamos la confirmación de que puede sentarse
			break;
		}
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
   	  if ( id_propio == 10)
   	  		funcion_camarero( );
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
