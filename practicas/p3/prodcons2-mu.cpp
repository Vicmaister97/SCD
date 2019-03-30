// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
// Autor: VICTOR GARCIA CARRERA, victorgarcia@correo.ugr.es
//
// Archivo: prodcons2-mu.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario (ESPERA SELECTIVA)
// (versión con múltiples productores(4) y consumidores (5))
//
// ## LA IMPRESIÓN POR PANTALLA NO SE CORRESPONDE CON EL ORDEN REAL DE LOS EVENTOS ##
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
	 num_procesos 		   = 10 , // 4 prods, 5 cons y 1 buffer
	 num_items             = 60,  // items a producir
	 tam_vector            = 10,
	 num_prods = 4 ,
	 num_cons  = 5 ,
	 id_buffer = num_prods,					// el buffer tiene identificador num_prods
	 items_per_prod = num_items/num_prods,	// numero de items que produce cada productor
	 items_per_cons = num_items/num_cons,	// numero de items que consume cada consumidor
	 etiq_prod = 0,							// etiqueta utilizada para referenciar los mensajes de los productores
	 etiq_cons = 1;							// etiqueta utilizada para referenciar los mensajes de los consumidores
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
// producir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir( int num_orden)
{
	 static int contador = (num_orden*items_per_prod)-1 ;
	 sleep_for( milliseconds( aleatorio<10,100>()) );
	 contador++ ;
	 cout << "Productor (" << num_orden << ") ha producido valor " << contador << endl << flush;
	 return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor( int num_orden)
{
	 for ( unsigned int i= 0 ; i < items_per_prod ; i++ )
	 {
		// producir valor
		int valor_prod = producir(num_orden);
		// enviar valor
		cout << "Productor (" << num_orden << ") va a enviar valor " << valor_prod << endl << flush;
		MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
	 }

	cout << "Productor (" << num_orden << ") HA TERMINADO " << endl << flush;
}
// ---------------------------------------------------------------------

void consumir( int num_cons, int valor_cons )
{
	 // espera bloqueada
	 sleep_for( milliseconds( aleatorio<110,200>()) );
	 cout << "Consumidor (" << num_cons << ") ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor( int num_orden)
{
	 int         peticion,
				 valor_rec = 1 ;
	 MPI_Status  estado ;

	 // Asumimos que un consumidor puede consumir el dato que sea
	 for( unsigned int i=0 ; i < items_per_cons; i++ )
	 {
		cout << "Consumidor "<< num_orden << " LIBRE, pide consumir" << endl << flush ;
		MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
		MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD,&estado );
		cout << "Consumidor "<< num_orden << " ha recibido valor " << valor_rec << endl << flush ;
		consumir( num_orden, valor_rec );
	 }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
	int         buffer[tam_vector],      // buffer con celdas ocupadas y vacías
				valor,                   // valor recibido o enviado
				primera_libre       = 0, // índice de primera celda libre
				primera_ocupada     = 0, // índice de primera celda ocupada
				num_celdas_ocupadas = 0, // número de celdas ocupadas
				etiqueta_emisor_aceptable ;    // identificador de emisor aceptable por su etiqueta
	MPI_Status estado ;                 // metadatos del mensaje recibido

	for( unsigned int i=0 ; i < num_items*2 ; i++ )
	{
	// 1. determinar si puede enviar solo prod., solo cons, o todos

		if ( num_celdas_ocupadas == 0 )               // si buffer vacío
			etiqueta_emisor_aceptable = etiq_prod;       // $~~~$ solo prod.
		else // si buffer lleno
			etiqueta_emisor_aceptable = etiq_cons;      // $~~~$ solo cons.
		
		// 2. recibir un mensaje del emisor o emisores aceptables (por la etiqueta del mensaje)

		MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta_emisor_aceptable, MPI_COMM_WORLD, &estado );

		// 3. procesar el mensaje recibido

		switch( estado.MPI_TAG ) // leer etiqueta del mensaje en metadatos
		{
		case etiq_prod: // si ha sido el productor: insertar en buffer
			buffer[primera_libre] = valor ;
			primera_libre = (primera_libre+1) % tam_vector ;
			num_celdas_ocupadas++ ;
			cout << "Buffer ha recibido valor " << valor << endl ;
			break;

		case etiq_cons: // si ha sido el consumidor: extraer y enviarle
			valor = buffer[primera_ocupada] ;
			primera_ocupada = (primera_ocupada+1) % tam_vector ;
			num_celdas_ocupadas-- ;
			cout << "Buffer va a enviar valor " << valor << endl ;
			// Vemos en la struct estado cual es el identificador del consumidor emisor de la petición para responderle
			MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD);
			break;
		}
	}
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
	 int id_propio, num_procesos_actual;

	 // inicializar MPI, leer identif. de proceso y número de procesos
	 MPI_Init( &argc, &argv );
	 MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
	 MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

	 if ( num_procesos == num_procesos_actual )
	 {
		// ejecutar la operación apropiada a 'id_propio'
		if ( id_propio < num_prods )
			funcion_productor(id_propio);
		else if ( id_propio == num_prods )
			funcion_buffer();
		else
		 	funcion_consumidor(id_propio-num_prods-1);
	 }
	 else
	 {
		if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
		{ cout << "el número de procesos esperados es:    " << num_procesos << endl
			 << "el número de procesos en ejecución es: " << num_procesos_actual << endl
			 << "(programa abortado)" << endl ;
		}
	 }

	 // al terminar el proceso, finalizar MPI
	 MPI_Finalize( );
	 return 0;
}
