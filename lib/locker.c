#include	"locker.h"

void init_sem(sem_t *m_sem)
{
	if( sem_init( m_sem, 0, 0 ) != 0 )
	{
		//throw std::exception();
	}
}

void del_sem(sem_t *m_sem)
{
	sem_destroy( m_sem );
}

bool wait_get(sem_t *m_sem)
{
        return sem_wait( m_sem ) == 0;
}

bool post(sem_t *m_sem)
{
	return sem_post( m_sem ) == 0;
}

void init_locker(pthread_mutex_t *m_mutex)
{
        if( pthread_mutex_init( m_mutex, NULL ) != 0 )
        {
		//throw std::exception();
	}
}
void del_locker(pthread_mutex_t *m_mutex)
{
	 pthread_mutex_destroy( m_mutex );
}

bool lock(pthread_mutex_t *m_mutex)
{
	return pthread_mutex_lock( m_mutex ) == 0;
}
bool unlock(pthread_mutex_t *m_mutex)
{
	return pthread_mutex_unlock( m_mutex ) == 0;
}

/*
class cond
{
public:
    cond()
    {
        if( pthread_mutex_init( &m_mutex, NULL ) != 0 )
        {
            throw std::exception();
        }
        if ( pthread_cond_init( &m_cond, NULL ) != 0 )
        {
            pthread_mutex_destroy( &m_mutex );
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_mutex_destroy( &m_mutex );
        pthread_cond_destroy( &m_cond );
    }
    bool wait()
    {
        int ret = 0;
        pthread_mutex_lock( &m_mutex );
        ret = pthread_cond_wait( &m_cond, &m_mutex );
        pthread_mutex_unlock( &m_mutex );
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal( &m_cond ) == 0;
    }

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
*/
