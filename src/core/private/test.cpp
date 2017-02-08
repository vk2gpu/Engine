

struct TestStruct
{
	int a = 0;
	float b = 0.0f;
};

std::vector<int> bleh;
int main()
{
	Array<int, 10> IntArray;
	Array<TestStruct, 100> TestStructArray;

	for(size_t Idx = 0; Idx < IntArray.size(); ++Idx)
	{
		IntArray[Idx] = (int)Idx;
	}

	for(const auto Val : IntArray)
	{
		int a = 0; ++a;
	}

	Vector<int> IntVector;
	Vector<int> IntVector2;

	for(size_t Idx = 0; Idx < 200; ++Idx)
	{
		IntVector.push_back((int)Idx);
	}

	std::swap(IntVector, IntVector2);

	IntVector.reserve(1000);
	IntVector.push_back(10000);
	IntVector.shrink_to_fit();
}
