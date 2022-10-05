////////////////////////////////////////////////////////////////////////////
//
//    copy from IPC
//
////////////////////////////////////////////////////////////////////////////

#include <oks/pipeline.h>
#include <boost/bind/bind.hpp>

bool
OksPipeline::Worker::setJob( OksJob * job )
{
    boost::mutex::scoped_lock lock( m_mutex );
    if ( !m_running && m_idle )
    {
    	m_job = job;
    	m_idle = false;
    	m_condition.notify_one();
        return true;
    }
    return false;
}

void
OksPipeline::Worker::run( )
{
    {
	boost::mutex::scoped_lock lock( m_mutex );
	m_pipeline.m_barrier.wait();
    }
    while ( true )
    {
	{
	    boost::mutex::scoped_lock lock( m_mutex );
            m_running = true;
        }
	while ( !m_idle ) {
	    m_job->run();
	    delete m_job;
            m_idle = !m_pipeline.getJob( m_job );
	}
	{
	    boost::mutex::scoped_lock lock( m_mutex );
            m_running = false;
            if ( m_shutdown ) 
		break;
	    if ( m_stop ) {
		m_pipeline.m_barrier.wait();
                m_stop = false;
            }
	    if ( m_idle )
            	m_condition.wait( lock ); 
        }
    }
}

void
OksPipeline::Worker::shutdown( )
{
    boost::mutex::scoped_lock lock( m_mutex );
    m_shutdown = true;
    m_condition.notify_one( );
}

void
OksPipeline::Worker::stop( )
{
    boost::mutex::scoped_lock lock( m_mutex );
    m_stop = true;
    m_condition.notify_one( );
}

OksPipeline::OksPipeline( size_t size )
  : m_barrier( size + 1 )
{
    for ( size_t i = 0; i < size; ++i )
    {
        m_workers.push_back( WorkerPtr( new Worker( *this ) ) );
        m_pool.create_thread( boost::bind( &Worker::run, m_workers.back().get() ) );
    }
    m_barrier.wait();
}
 
OksPipeline::~OksPipeline()
{
    waitForCompletion();
    for ( size_t i = 0; i < m_workers.size(); ++i )
    {
	m_workers[i] -> shutdown();
    }
    m_pool.join_all();
}

void
OksPipeline::addJob( OksJob * job )
{
    for ( size_t i = 0; i < m_workers.size(); ++i )
    {
	if ( m_workers[i] -> setJob( job ) )
        {
            return;
        }
    }
    boost::mutex::scoped_lock lock( m_mutex );
    m_jobs.push( job );
}

void
OksPipeline::waitForCompletion()
{
    {
	boost::mutex::scoped_lock lock( m_mutex );
	while ( !m_jobs.empty() )
	{
	    m_condition.wait( lock );
	}
    }
    for ( size_t i = 0; i < m_workers.size(); ++i )
    {
	m_workers[i] -> stop();
    }
    m_barrier.wait();
}

bool
OksPipeline::getJob( OksJob* & job )
{
    boost::mutex::scoped_lock lock( m_mutex );
    if ( !m_jobs.empty() )
    {
    	job = m_jobs.front();
        m_jobs.pop();
	m_condition.notify_one( );
        return true;
    }
    return false;
}
