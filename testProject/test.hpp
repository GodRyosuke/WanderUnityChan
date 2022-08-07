class Parent {
public:
	Parent();
	void print();
	void setIndex(int index) { mIndex = index; }
protected:
	virtual void prePrint();
	virtual void preInnerProcess();

	int mIndex;
private:
	void InnerProcess();
};

class Child : public Parent {
public:
	Child() {}
private:
	void prePrint() override;
	void preInnerProcess() override;
};