#include "test.hpp"
#include <iostream>

Parent::Parent()
	:mIndex(0)
{

}

void Parent::prePrint() {
	std::cout << "this is preprint\n";
}

void Parent::print()
{
	prePrint();
	InnerProcess();
	std::cout << "hello" << std::endl;
	std::cout << "index: " << mIndex << std::endl;
}

void Parent::InnerProcess()
{
	preInnerProcess();
	mIndex += 5;
}

void Child::prePrint()
{
	std::cout << "child preprint\n";
}

void Parent::preInnerProcess()
{
	mIndex = 1000;
}

void Child::preInnerProcess()
{
	mIndex = 10;
}

int main(int argc, char** argv)
{
	Child* child = new Child();
	child->print();

	return 0;
}