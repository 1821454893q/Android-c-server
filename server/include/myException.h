#ifndef MYEXCEPTION_H
#define MYEXCEPTION_H
#include "myhead.h"
class myException : public std::exception
{
public:
	myException(std::string msg) throw()
	{
		this->msg = msg;
	}
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT override
	{
		return msg.c_str();
	}
	int size() const { return msg.size(); }
private:
	std::string msg;
};
#endif // !myException

