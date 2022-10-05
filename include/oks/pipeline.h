#ifndef OKS_PIPELINE_H
#define OKS_PIPELINE_H

////////////////////////////////////////////////////////////////////////////
//
//    copy from IPC
//
////////////////////////////////////////////////////////////////////////////

#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/condition.hpp>
#include <memory>
#include <queue>

class OksJob
{
  public:

    virtual ~OksJob() { ; }
    virtual void run(void) = 0;

};

class OksPipeline
{
  public:
    OksPipeline( size_t size );

    ~OksPipeline();

    void waitForCompletion();

    void addJob( OksJob * job );

  private:

    struct Worker
    {
    	Worker( OksPipeline & pipeline )
          : m_pipeline( pipeline ),
            m_idle( true ),
            m_shutdown( false ),
            m_stop( false ),
            m_running( false )
        { ; }

        void run();

	bool setJob( OksJob * job );

        void shutdown();

        void stop();

      private:
        OksPipeline &		m_pipeline;
	boost::mutex		m_mutex;
	boost::condition	m_condition;
        bool			m_idle;
        bool			m_shutdown;
        bool			m_stop;
        bool			m_running;
        OksJob *		m_job;
    };

    typedef std::shared_ptr<Worker> WorkerPtr;
        
  private:
    friend struct Worker;
    
    bool getJob( OksJob* & job );
    
  private:
    boost::mutex		m_mutex;
    boost::condition		m_condition;
    boost::barrier		m_barrier;
    boost::thread_group		m_pool;
    std::vector<WorkerPtr>	m_workers;
    std::queue<OksJob*>		m_jobs;
};

#endif
