#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


typedef struct _xMPI_Request
{
    MPI_Request req;
    MPI_Request * myself;
    /* Put extra State HERE */
}xMPI_Request;

xMPI_Request * xMPI_Request_new(MPI_Request * parent)
{
    xMPI_Request * ret = malloc( sizeof(xMPI_Request));

    if( !ret )
    {
        perror("malloc");
        abort();
    }

    ret->req = MPI_REQUEST_NULL;
    ret->myself = parent;

    return ret;
}

/** Extended Generalized Request Interface **/

int xMPI_Request_query_fn( void * pxreq, MPI_Status * status )
{
    xMPI_Request * xreq = (xMPI_Request*) pxreq;
    int flag;
    return MPI_Request_get_status(xreq->req, &flag, status );
}

int xMPI_Request_poll_fn( void * pxreq, MPI_Status * status )
{
    int flag;
    xMPI_Request * xreq = (xMPI_Request*) pxreq;
    int ret = MPI_Test( &xreq->req, &flag, status );

    if( ret != MPI_SUCCESS )
        return ret;

    if( flag )
    {
        //fprintf(stderr, "Completed\n");
        MPI_Grequest_complete(*xreq->myself);
    }

    return ret;
}

int xMPI_Request_wait_fn( int cnt, void ** array_of_states, double timeout, MPI_Status * st )
{
    /* Simple implementation */
    int i;

    int completed = 0;
    char _done_array[128] = {0};
    char *done_array = _done_array;
    if( 128 <= cnt )
    {
        done_array = malloc( cnt * sizeof(char));
        if( !done_array )
        {
            perror("malloc");
            return MPI_ERR_INTERN;
        }
    }

    int r;
    MPI_Comm_rank( MPI_COMM_WORLD , &r );

    while( completed != cnt )
    {
        for( i = 0 ; i < cnt ; i++ )
        {
            if( done_array[i] )
                continue;

            xMPI_Request * xreq = (xMPI_Request*) array_of_states[i];
            int flag = 0;
            
            //fprintf(stderr, "[%d] NOW Testing %d\n", r, i );
            MPI_Test( &xreq->req , &flag , MPI_STATUS_IGNORE );
            
            if( flag )
            {
                //fprintf(stderr, "[%d] Completed %d\n", r , i);
                MPI_Grequest_complete(*xreq->myself);
                completed++;
                done_array[i] = 1;
            }
        }
    }

    if( done_array != _done_array )
        free( done_array );


    return MPI_SUCCESS;
}

int xMPI_Request_free_fn( void * pxreq )
{
    free( pxreq );
    return MPI_SUCCESS;
}



int xMPI_Request_cancel_fn( void * pxreq, int complete )
{
    if(!complete)
        return MPI_SUCCESS;
    xMPI_Request * xreq = (xMPI_Request*) pxreq;
    return MPI_Cancel( &xreq->req );
}


int xMPI_Isend( void* buf , int count , MPI_Datatype datatype , int dest , int tag , MPI_Comm comm , MPI_Request* request )
{
    xMPI_Request * xreq = xMPI_Request_new(request);

#ifdef MPC
    int ret = MPIX_Grequest_start( xMPI_Request_query_fn,
                                   xMPI_Request_free_fn,
                                   xMPI_Request_cancel_fn,
                                   xMPI_Request_poll_fn,
                                   xreq,
                                   request);
#else
  int ret = MPIX_Grequest_start( xMPI_Request_query_fn,
                                   xMPI_Request_free_fn,
                                   xMPI_Request_cancel_fn,
                                   xMPI_Request_poll_fn,
                                   xMPI_Request_wait_fn,
                                   xreq,
                                   request);
#endif
    if( ret != MPI_SUCCESS )
        return ret;


    return MPI_Isend( buf , count , datatype , dest , tag , comm , &xreq->req );
}



int xMPI_Irecv( void* buf , int count , MPI_Datatype datatype , int source , int tag , MPI_Comm comm , MPI_Request* request )
{
    xMPI_Request * xreq = xMPI_Request_new(request);
#ifdef MPC
    int ret = MPIX_Grequest_start( xMPI_Request_query_fn,
                                   xMPI_Request_free_fn,
                                   xMPI_Request_cancel_fn,
                                   xMPI_Request_poll_fn,
                                   xreq,
                                   request);
#else
  int ret = MPIX_Grequest_start( xMPI_Request_query_fn,
                                   xMPI_Request_free_fn,
                                   xMPI_Request_cancel_fn,
                                   xMPI_Request_poll_fn,
                                   xMPI_Request_wait_fn,
                                   xreq,
                                   request);
#endif
    if( ret != MPI_SUCCESS )
        return ret;

    return MPI_Irecv( buf , count , datatype , source , tag , comm , &xreq->req );
}

#define CNT 16

int main( int argc, char *argv[])
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if( size != 2 )
    {
        fprintf(stderr, "This program runs on two processes\n");
        return 1;
    }

    int i;
    MPI_Request reqs[CNT];
    int dat[CNT];

    if( rank == 0 )
    {
        for( i = 0 ; i < CNT ; i++ )
        {
            dat[i] = i * 2;
        }

        for( i = 0 ; i < CNT ; i++ )
        {
            xMPI_Isend( &dat[i] , 1 , MPI_INT , 1 , 123 , MPI_COMM_WORLD, &reqs[i] );
        }
    }
    else
    {
        for( i = 0 ; i < CNT ; i++ )
        {
            xMPI_Irecv( &dat[i] , 1 , MPI_INT , 0 , 123 , MPI_COMM_WORLD, &reqs[i] );
        }
    }

    MPI_Waitall( CNT , reqs , MPI_STATUSES_IGNORE );

    if( rank )
    {

        for( i = 0 ; i < CNT ; i++ )
        {
            printf("dat[%d] = %d\n", i, dat[i]);

            if( dat[i] != i * 2 )
            {
                fprintf(stderr, "Error in data at %d got %d expected %d\n", i, dat[i], i*2);
                return 1;
            }
        }

        printf("ALL OK !\n");
    }

    MPI_Finalize();

    return 0;
}
