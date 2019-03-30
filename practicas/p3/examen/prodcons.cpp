// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// EXAMEN Práctica 3. Implementación de algoritmos distribuidos con MPI
// Autor: VICTOR GARCIA CARRERA, victorgarcia@correo.ugr.es
//
// Archivo: prodcons.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario (ESPERA SELECTIVA)
// (versión con 1 productor y mútiples consumidores (4))
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
	 num_procesos 		   = 6 , // 1 prod, 4 cons y 1 buffer
	 num_items             = 20,  // items a producir
	 tam_vector            = 3,
	 num_cons  = 4 ,
	 id_buffer = num_cons,					// el buffer tiene identificador num_cons (4)
	 id_productor = num_cons + 1,			// el productor tiene identificador (5)
	 items_per_cons = num_items/num_cons,	// numero de items que consume cada consumidor
	 etiq_prod = 0,							// etiqueta utilizada para referenciar los mensajes del productor
	 etiq_cons_par = 1,						// etiqueta utilizada para referenciar los mensajes de los consumidores pares
	 etiq_cons_impar = 2;					// etiqueta utilizada para referenciar los mensajes de los consumidores impares

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
// producir produce los numeros en secuencia (0,1,2,...., num_items-1(en este caso 19))
// y lleva espera aleatorio
int producir( )
{
	static int contador = 0 ;
	sleep_for( milliseconds( aleatorio<10,200>()) );
	contador++ ;
	cout << "Productor ha producido valor " << contador << endl << flush;
	return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor( int num_orden)
{
	 for ( unsigned int i= 0 ; i < num_items ; i++ )
	 {
		// producir valor
		int valor_prod = producir( );
		// enviar valor
		cout << "Productor " << "va a enviar valor " << valor_prod << endl << flush;
		MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
	 }

	cout << "Productor HA TERMINADO " << endl << flush;
}
// ---------------------------------------------------------------------

void consumir( int num_cons, int valor_cons )
{
	 // espera bloqueada
	 sleep_for( milliseconds( aleatorio<110,200>()) );
	 cout << "Consumidor " << num_cons << " ha consumido valor " << valor_cons << endl << flush ;
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
		cout << "Consumidor "<< num_orden << " pide consumir" << endl << flush ;

		if (num_orden % 2 == 0)		// si se trata de un consumidor par
			MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons_par, MPI_COMM_WORLD);
		else
			MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons_impar, MPI_COMM_WORLD);			
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
				etiqueta_emisor_aceptable ,		// identificador de emisor aceptable por su etiqueta
				id_emisor,						// identificador del emisor del mensaje recibido
				comp;
	MPI_Status estado ;                 // metadatos del mensaje recibido

	for( unsigned int i=0 ; i < num_items*2 ; i++ )
	{
	// 1. determinar si puede enviar solo prod., solo cons, o todos

		if ( num_celdas_ocupadas == 0 )               			// si buffer vacío
			etiqueta_emisor_aceptable = etiq_prod;     			// $~~~$ solo prod.
		else if ( num_celdas_ocupadas == tam_vector){	  			// si buffer lleno
			if (buffer[primera_ocupada] % 2 == 0)	  			// si el dato a consumir es par
				etiqueta_emisor_aceptable = etiq_cons_par;      // $~~~$ solo cons. pares
			else												// si el dato a consumir es impar
				etiqueta_emisor_aceptable = etiq_cons_impar;      // $~~~$ solo cons. pares
			}
		else
			etiqueta_emisor_aceptable = MPI_ANY_TAG;	// acepta cualquier petición de productor o consumidor
		
		// 2. recibir un mensaje del emisor o emisores aceptables (por la etiqueta del mensaje)

		MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta_emisor_aceptable, MPI_COMM_WORLD, &estado );

		// 3. procesar el mensaje recibido
		// guardamos el identificador del emisor del mensaje
		id_emisor = estado.MPI_SOURCE;

		switch( estado.MPI_TAG ) // leer etiqueta del mensaje en metadatos
		{
		case etiq_prod: // si ha sido el productor: insertar en buffer
			buffer[primera_libre] = valor ;
			primera_libre = (primera_libre+1) % tam_vector ;
			num_celdas_ocupadas++ ;
			cout << "Buffer ha recibido valor " << valor << endl ;
			break;

		default: // si ha sido un consumidor: comprueba la paridad del dato a consumir con la del emisor para aceptar o no su peticion
			if (buffer[primera_ocupada] % 2 == 0){	// si es dato par
				if (id_emisor == 0) comp = 2;		// por fallo de comparacion %0
				else comp = id_emisor;
				if (comp % 2 == 0){				// si el consumidor que pide el dato tiene paridad par
					valor = buffer[primera_ocupada] ;
					primera_ocupada = (primera_ocupada+1) % tam_vector ;
					num_celdas_ocupadas-- ;
					cout << "Buffer va a enviar valor " << valor << " al consumidor " << id_emisor << endl ;
					// Vemos en la struct estado cual es el identificador del consumidor emisor de la petición para responderle
					MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD);
					break;
				}
			}
			if (buffer[primera_ocupada] % 2 != 0){	// si es dato impar
				if (id_emisor % 2 != 0){				// si el consumidor que pide el dato tiene tambien paridad impar
					valor = buffer[primera_ocupada] ;
					primera_ocupada = (primera_ocupada+1) % tam_vector ;
					num_celdas_ocupadas-- ;
					cout << "Buffer va a enviar valor " << valor << " al consumidor " << id_emisor << endl ;
					// Vemos en la struct estado cual es el identificador del consumidor emisor de la petición para responderle
					MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, 0, MPI_COMM_WORLD);
					break;
				}
			else break;
			}
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
		if ( id_propio < num_cons )
			funcion_consumidor(id_propio);
		else if ( id_propio == num_cons )
			funcion_buffer();
		else
			funcion_productor(id_propio);
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
