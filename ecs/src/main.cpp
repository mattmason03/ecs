#include "ecs.h"

#include <iostream>

int main(void){
	ecs::EntityManager manager;

	struct Pos : ecs::Component < Pos > {
		Pos(int x, int y) :x(x), y(y){}
		int x, y;
	};

	struct Vel : ecs::Component < Vel > {
		Vel(float f) :f(f){}
		float f;
	};

	struct A : ecs::Component < A > {
		A(char a) :a(a){}
		char a;
	};

	manager.RegisterComponent < Pos >();
	manager.RegisterComponent < Vel >();
	manager.RegisterComponent <A>();

	ecs::Entity entity = manager.CreateEntity();

	entity.AddComponent<Pos>(3, 5);
	entity.AddComponent<A>('a');
	entity = manager.CreateEntity();
	entity.AddComponent<Pos>(4, -1);
	entity.AddComponent<Vel>(5.5f);
	entity = manager.CreateEntity();
	entity.AddComponent<Vel>(3.7f);
	entity.AddComponent<A>('b');


	for (int i = 0; i < 1000; ++i){
		for (ecs::Entity e : manager.GetEntities<Pos>()){
			std::cout << e.GetId() << " " << e.GetComponent<Pos>()->x << "," << e.GetComponent<Pos>()->y << std::endl;
		}

		for (ecs::Entity e : manager.GetEntities<Vel>()){
			std::cout << e.GetId() << " " << e.GetComponent<Vel>()->f << std::endl;
		}

		for (ecs::Entity e : manager.GetEntities<A>()){
			std::cout << e.GetId() << " " << e.GetComponent<A>()->a << std::endl;
		}
	}
}