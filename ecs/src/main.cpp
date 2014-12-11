#include "ecs.h"

#include <iostream>
#include <chrono>

int main(void){
	ecs::EntityManager manager;

	struct Pos : ecs::Component < Pos > {
		Pos(int x, int y) :x(x), y(y){}
		Pos(){}
		int x, y;
	};

	struct Vel : ecs::Component < Vel > {
		Vel(float f) :f(f){}
		Vel(){}
		float f;
	};

	struct A : ecs::Component < A > {
		A(char a) :a(a){}
		char a;
	};

	std::chrono::time_point<std::chrono::system_clock> start, end;

	manager.RegisterComponent < Pos >();
	manager.RegisterComponent < Vel >();
	manager.RegisterComponent <A>();

	//ecs::Entity entity = manager.CreateEntity();
	/*
	entity.AddComponent<Pos>(3, 5);
	entity.AddComponent<A>('a');
	entity = manager.CreateEntity();
	entity.AddComponent<Pos>(4, -1);
	entity.AddComponent<Vel>(5.5f);
	entity = manager.CreateEntity();
	entity.AddComponent<Vel>(3.7f);
	entity.AddComponent<A>('b');*/


	//for (int i = 0; i < entityCount; ++i){
	//	for (ecs::Entity e : manager.GetEntities<Pos>()){
	//		std::cout << e.GetId() << " " << e.GetComponent<Pos>()->x << "," << e.GetComponent<Pos>()->y << std::endl;
	//	}

	//	for (ecs::Entity e : manager.GetEntities<Vel>()){
	//		std::cout << e.GetId() << " " << e.GetComponent<Vel>()->f << std::endl;
	//	}

	//	for (ecs::Entity e : manager.GetEntities<A>()){
	//		std::cout << e.GetId() << " " << e.GetComponent<A>()->a << std::endl;
	//	}
	//}

	

	const size_t entityCount = 1000;
	const size_t iterationCount = 30;

	start = std::chrono::system_clock::now();

	std::array<Pos, entityCount> posArray;
	std::array<Vel, entityCount> velArray;

	for (int i = 0; i < entityCount; ++i){
		ecs::Entity entity = manager.CreateEntity();
		entity.AddComponent<Pos>(i, i - 1);
		entity.AddComponent<Vel>((float)i);

		Pos pos(i, i - 1);
		Vel vel((float)i);
		posArray.at(i) = pos;
		velArray.at(i) = vel;
	}

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> init = end - start;
	start = std::chrono::system_clock::now();

	for (int i = 0; i < iterationCount; i++){
		for (ecs::Entity e : manager.GetEntities<Pos, Vel>()){
			e.GetComponent<Pos>()->x += 4;
			e.GetComponent<Pos>()->y *= 2;
			e.GetComponent<Vel>()->f += 1;
		}
	}

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> vect = end - start;
	start = std::chrono::system_clock::now();

	for (int i = 0; i < iterationCount; i++){
		manager.UpdateEntities<Pos, Vel>(
			[&start](Pos* position, Vel* vel)
		{
			position->x += 4;
			position->y *= 2;
			vel->f += 1;
		}
		, 0, entityCount);
	}

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> lambda = end - start;
	start = std::chrono::system_clock::now();

	for (int i = 0; i < iterationCount; i++){
		for (int i = 0; i < entityCount * 6; i++){
			posArray.at(i % entityCount).x += 4;
			posArray.at(i % entityCount).y *= 2;
			velArray.at(i % entityCount).f += 1;
		}
	}

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> arr = end - start;

	int i = 0;
	for (ecs::Entity e : manager.GetEntities<Pos, Vel>()){
		std::cout << posArray[i].x << posArray[i].y << velArray[i].f << e.GetComponent<Pos>()->x << e.GetComponent<Pos>()->y << e.GetComponent<Vel>()->f << std::endl;
		i++;
	}

	std::cout << init.count() << std::endl;
	std::cout << vect.count() << std::endl;
	std::cout << lambda.count() << std::endl;
	std::cout << arr.count() << std::endl;

	char a;
	std::cin >> a;
}