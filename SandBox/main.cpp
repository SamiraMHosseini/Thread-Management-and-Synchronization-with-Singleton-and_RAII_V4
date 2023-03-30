#include <conio.h>
#include <thread>
#include <future>
#include <chrono>
#include <iostream>
#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;

struct SharedResource
{
	//Copy
	SharedResource(const SharedResource&) = delete;
	SharedResource& operator=(const SharedResource&) = delete;

	////Move
	SharedResource(SharedResource&&) = default;
	SharedResource& operator=(SharedResource&&) = default;

	SharedResource() :
		tPromise(), tFuture(tPromise.get_future())
	{

	}

	std::promise<void> tPromise;
	std::future<void> tFuture;
};
//Meyer's Singleton pattern 
struct ThreadCountProxy //There is no destructor here
{
public:

	/*
	In object-oriented programming, the "this" pointer refers to the object instance that the method or function is being called on.
	However, static functions are not associated with any particular instance of the class and
	therefore do not have access to the "this" pointer.
	This means that static functions cannot access instance variables or methods of the class.
	*/

	static void Increment() //Because it is a static function, we don't have access to 'this' pointer
	{
		ThreadCountProxy& instance = ThreadCountProxy::privGetInstance();
		std::lock_guard<std::mutex> lock(instance.mtx);
		++instance.count;
		Debug::out("TC:%d \n", instance.count);
	}


	static void Decrement() //Because it is a static function, we don't have access to 'this' pointer
	{
		ThreadCountProxy& instance = ThreadCountProxy::privGetInstance();
		std::lock_guard<std::mutex> lock(instance.mtx);
		--instance.count;
		Debug::out("TC:%d \n", instance.count);
		if (instance.count == 0)
		{
			instance.cv_thread_count.notify_one();
		}

	}
	static void WaitUntilThreadsDone()  //Because it is a static function, we don't have access to 'this' pointer
	{
		ThreadCountProxy& instance = ThreadCountProxy::privGetInstance();
		std::unique_lock<std::mutex> lock_ct(instance.mtx_thread_count);
		instance.cv_thread_count.wait(lock_ct);
	}


private:
	~ThreadCountProxy() = default;

	ThreadCountProxy() :
		count(0), mtx(), mtx_thread_count(), cv_thread_count()
	{

		//This code shows whether the instance is created once or more than once.
		Debug::out("Default Constructor\n");
	}
	//Internally, that's the mechanism that makes it a Singleton!
	static ThreadCountProxy& privGetInstance()
	{
		//Lazy initialization
		static ThreadCountProxy instance; //This line calls the default constructor of class ThreadCountProxy only once
		return instance;
	}

	//Shared Data 

	int count;
	std::mutex mtx; //This mutex is used for the 'count' variable

	std::mutex				 mtx_thread_count; //This mutex is used for the signal
	std::condition_variable  cv_thread_count;
};

class ThreadCount
{
public:
	ThreadCount()
	{
		ThreadCountProxy::Increment();

	}

	~ThreadCount()
	{
		ThreadCountProxy::Decrement();

	}

};

class A : public BannerBase
{
public:
	A() = delete;
	A(const char* const pName, SharedResource& sr_) :
		BannerBase(pName), threadA(), sr(sr_)

	{
		assert(!threadA.joinable());

	}
	void Launch()
	{
		//Spawn a thread 
		//Version 1
		std::function<void()> threadFunc = [this]() { this->operator()(); };
		this->threadA = std::thread(threadFunc);

	}
	//This function then initiates the actual thread starting
	void operator()()
	{
		START_BANNER;

		ThreadCount TC;

		int count = 0;
		while (true)
		{
			Debug::out("%d\n", count);
			count++;


			std::future_status status = sr.tFuture.wait_for(1ms);
			if (status == std::future_status::ready)
			{
				// can't read this multiple times
				//tFuture.get();
				break;
			}
		}
		std::this_thread::sleep_for(2s); //Just for testing purposes
	}

	~A()
	{
		if (threadA.joinable())
		{
			threadA.join();
		}
	}

private:
	std::thread threadA;
	SharedResource& sr;
};

class B : public BannerBase
{
public:
	B() = delete;
	B(const char* const pName, SharedResource& sr_) :
		BannerBase(pName), sr(sr_)

	{
		assert(!threadB.joinable());

	}
	void Launch()
	{
		//Spawn a thread 
		//Version 2
		this->threadB = std::thread(&B::operator(), this);
	}
	//This function then initiates the actual thread starting
	void operator()()
	{
		START_BANNER;

		ThreadCount TC;

		int count = 0x10000;
		while (true)
		{
			Debug::out("0x%x\n", count);
			count--;

			std::future_status status = sr.tFuture.wait_for(1ms);
			if (status == std::future_status::ready)
			{
				// can't read this multiple times
				//tFuture.get();
				break;
			}
		}
	}
	~B()
	{
		if (threadB.joinable())
		{
			threadB.join();
		}
	}
private:
	//Data
	std::thread threadB;
	SharedResource& sr;

};


class C : public BannerBase
{
public:
	C() = delete;
	C(const char* const pName, SharedResource& sr_) :
		BannerBase(pName), threadC(), sr(sr_)

	{
		assert(!threadC.joinable());
	}
	//This function then initiates the actual thread starting
	void launch()
	{
		//Spawn a thread 
		//Version 3
		this->threadC = std::thread(std::ref(*this));
	}
	void operator()()
	{
		START_BANNER;

		ThreadCount TC;

		const char* pFruit[] =
		{
			{"apple"},
			{"orange"},
			{"banana"},
			{"lemon"}
		};

		int count = 0;
		while (true)
		{
			int i = count++ % 4;
			Debug::out("%s\n", pFruit[i]);


			std::future_status status =
				//tFuture.wait_until(std::chrono::system_clock::now());
				sr.tFuture.wait_for(500ms);
			if (status == std::future_status::ready)
			{
				break;
			}
		}
	}
	~C()
	{
		if (threadC.joinable())
		{
			threadC.join();
		}
	}
	//Data:
private:
	std::thread threadC;
	SharedResource& sr;

};

class D : public BannerBase
{
public:
	D() = delete;
	D(const char* const pName, SharedResource& sr_) :
		BannerBase(pName), threadD(), sr(sr_)

	{



	}

	//This function then initiates the actual thread starting
	void Launch()
	{
		//Spawn a thread 
		//Version 4
		this->threadD = std::thread([&]()->void
			{
				this->operator()();

			});
	}
	void operator()()
	{
		START_BANNER;

		ThreadCount TC;

		const char* pStoryOriginal = "<0><1><2><3><4><5><6><7><8>";
		size_t  len = strlen(pStoryOriginal);
		char* pString = new char[len + 1];
		strcpy_s(pString, len + 1, pStoryOriginal);

		int tmpLen = (int)(len - 1);

		while (true)
		{
			if (tmpLen <= 0)
			{
				tmpLen = (int)(len - 1);
				strcpy_s(pString, len + 1, pStoryOriginal);
			}

			Debug::out("%s\n", pString);
			pString[tmpLen--] = 0;
			pString[tmpLen--] = 0;
			pString[tmpLen--] = 0;

			std::future_status status = sr.tFuture.wait_for(1ms);
			if (status == std::future_status::ready)
			{
				// can't read this multiple times
				//tFuture.get();
				break;
			}
		}

		delete pString;
	}

	~D()
	{
		if (threadD.joinable())
		{
			threadD.join();
		}
		else
			assert(false);
	}
	//Data
private:
	std::thread threadD;
	SharedResource& sr;
};


class Controller : public BannerBase
{

public:
	Controller() = delete;
	Controller(const char* const pName, SharedResource& sr_) :
		BannerBase(pName), thread(), sr(sr_)
	{
		assert(!this->thread.joinable());
	}
	void Launch()
	{

		//Version 1 
		std::future<void> fuController = std::async(std::launch::async, std::ref(*this));
		//fuController.wait(); //This line is not required as
		//the Controller thread waits for all A, B, C, D threads to finish in ThreadCountProxy::WaitUntilThreadsDone();
	}
	void operator()()
	{
		START_BANNER;

		// kill them now
		sr.tPromise.set_value();

		// wait until all are done
		ThreadCountProxy::WaitUntilThreadsDone();
	}
	~Controller()
	{
		if (this->thread.joinable())
		{
			this->thread.join();
		}
	}
private:
	std::thread thread;
	SharedResource& sr;
};


int main()
{

	START_BANNER_MAIN("main");

	SharedResource sharedResource;

	A oA("A", sharedResource);
	B oB("B", sharedResource);
	C oC("C", sharedResource);
	D oD("D", sharedResource);

	oA.Launch();
	oB.Launch();
	oC.launch();
	oD.Launch();

	Controller oController("Controller", sharedResource);

	//Key press
	_getch();
	Debug::out("key pressed <-----\n");

	oController.Launch();

}


