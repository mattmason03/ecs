#pragma once

#include <bitset>
#include <vector>
#include <array>

namespace ecs{
	const unsigned int MAX_COMPONENTS = 64;

	class BaseComponent;

	template <typename TComp>
	class Component;

	class Entity;

	class EntityManager;

	class Pool;
	template <typename TComp>
	class ComponentPool;

	using Mask = std::bitset<MAX_COMPONENTS>;

	template <typename... TComp>
	struct UnpackHelper;

	template <typename T, typename... TComp>
	struct UnpackHelper < T, TComp... >
	{
	public:
		static Mask BuildMask(){
			Mask mask = UnpackHelper<TComp...>::BuildMask();
			mask.set(T::componentId);
			return mask;
		}

		//ids will be in reverse order
		static std::vector<Pool*> GetPools(EntityManager& manager){
			std::vector<Pool*> vec = UnpackHelper<TComp...>::GetPools(manager);
			vec.push_back(manager.componentPools[T::componentId]);
			return vec;
		}

		template <typename F, typename... Args>
		static void CallFunction(F& func, unsigned int entityIndex, std::vector<Pool*>& pools, unsigned int poolIndex, Args... args){
			UnpackHelper<TComp...>::CallFunction(func, entityIndex, pools, poolIndex - 1, args..., (T*)pools[poolIndex]->Get(entityIndex));
		}

		//template <typename F, typename... Args>
		//static void CallF(F& func, unsigned int entityIndex, std::vector<Pool*>& pools, unsigned int poolIndex, Args... args){
		//	UnpackHelper<TComp...>::CallF(func, entityIndex, pools, poolIndex - 1, args..., (T*)pools[poolIndex]->Get(entityIndex));
		//}
	};

	template <>
	struct UnpackHelper < >
	{
	public:
		static Mask BuildMask(){
			Mask mask;
			return mask;
		}

		static std::vector<Pool*> GetPools(EntityManager& manager){
			std::vector<Pool*> vec;
			return vec;
		}

		template <typename F, typename... Args>
		static void CallFunction(F& func, unsigned int entityIndex, std::vector<Pool*>& pools, unsigned int poolIndex, Args... args){
			func(args...);
		}

		//template <typename F, typename... Args>
		//static void CallF(F& func, unsigned int entityIndex, std::vector<Pool*>& pools, unsigned int poolIndex, Args... args){
		//	func(std::forward<Args>(args)...);
		//}
	};

	class BaseComponent{
	public:
		static unsigned int componentCount;
	};

	template <typename TComp>
	class Component{
	public:
		using Type = TComp;
		static unsigned int componentId;

		static void Register();
	};

	class Entity{
	public:
		Entity(EntityManager* manager, unsigned int id);
		Entity(Entity&& right);
		Entity& Entity::operator=(Entity&& right);

		unsigned int GetId();

		template <typename TComp, typename... TArgs>
		void AddComponent(TArgs&&... args);

		template <typename TComp>
		TComp* GetComponent();

		template <typename TComp>
		void RemoveComponent();

	private:
		unsigned int id;
		EntityManager* manager;
	};

	class EntityManager{
	public:
		Entity CreateEntity();

		Entity GetEntity(unsigned int id);

		template <typename... TComp>
		std::vector<Entity> GetEntities();

		template <typename... TComp, typename F>
		void UpdateEntities(F& f, unsigned int start, unsigned int end);

		template <typename TComp>
		void RegisterComponent();

		template <typename TComp, typename... TArgs>
		void AddComponent(unsigned int id, TArgs&&... args);

		template <typename TComp>
		TComp* GetComponent(unsigned int id);

		template <typename TComp>
		void RemoveComponent(unsigned int id);

		unsigned int GetEntityCount();

		std::array<Pool*, MAX_COMPONENTS> componentPools;

	private:
		unsigned int entityCount;
		std::vector<Mask> entityMasks;
	};

	class Pool{
	public:
		Pool(size_t chunkSize, size_t eltSize);

		void CheckIndex(size_t num);

		char* Get(size_t index);

	protected:
		std::vector<char*> memory;
		size_t count;
		const size_t eltPerChunk;
		const size_t eltSize;
		const size_t chunkSize;

		void Expand();
	};

	template <typename TComp>
	class ComponentPool : public Pool{
	public:
		ComponentPool();
	};


	////
	//Implementation
	////

	//BaseComponent

	unsigned int BaseComponent::componentCount = 0;

	//Component

	template <typename TComp>
	unsigned int Component<TComp>::componentId = UINT_MAX;

	template <typename TComp>
	void Component<TComp>::Register(){
		if (componentId == UINT_MAX)
			componentId = BaseComponent::componentCount++;
	}

	//Entity

	Entity::Entity(EntityManager* manager, unsigned int id)
		:manager(manager), id(id) 
	{}

	Entity::Entity(Entity&& right)
		: manager(right.manager), id(right.id)
	{
	}

	Entity& Entity::operator=(Entity&& right){
		if (this != &right){
			manager = right.manager;
			id = right.id;
		}
		return *this;
	}

	unsigned int Entity::GetId(){
		return id;
	}

	template <typename TComp, typename... TArgs>
	void Entity::AddComponent(TArgs&&... args){
		manager->AddComponent<TComp>(id, args...);
	}

	template <typename TComp>
	TComp* Entity::GetComponent(){
		return manager->GetComponent<TComp>(id);
	}

	template <typename TComp>
	void Entity::RemoveComponent(){
		manager->RemoveComponent<TComp>();
	}

	//EntityManager

	Entity EntityManager::CreateEntity(){
		Entity entity(this, entityCount++);
		entityMasks.emplace_back();
		return entity;
	}

	Entity EntityManager::GetEntity(unsigned int id){
		return Entity(this, id);
	}

	template <typename... TComp>
	std::vector<Entity> EntityManager::GetEntities(){
		Mask mask = UnpackHelper<TComp...>::BuildMask();
		std::vector<Entity> entities;
		entities.reserve(entityCount);
		for (unsigned int i = 0; i < entityCount; ++i){
			if ((mask & entityMasks[i]) == mask){
				entities.emplace_back(this, i);
			}
		}
		return entities;
	}

	template <typename... TComp, typename F>
	void EntityManager::UpdateEntities(F& f, unsigned int start, unsigned int end){
		Mask mask = UnpackHelper<TComp...>::BuildMask();
		std::vector<Pool*> pools = UnpackHelper<TComp...>::GetPools(*this);
		for (; start != end && start < entityCount; ++start){
			if ((mask & entityMasks[start]) == mask){
				UnpackHelper<TComp...>::CallFunction(f, start, pools, pools.size() - 1);
			}
		}
	}

	template <typename TComp>
	void EntityManager::RegisterComponent(){
		TComp::Register();
		componentPools[TComp::componentId] = new ComponentPool < TComp >();
	}

	template <typename TComp, typename... TArgs>
	void EntityManager::AddComponent(unsigned int id, TArgs&&... args){
		unsigned int o = TComp::componentId;
		entityMasks[id].set(TComp::componentId);
		Pool* pool = componentPools[TComp::componentId];
		pool->CheckIndex(id);
		new(pool->Get(id)) TComp(std::forward<TArgs>(args)...);
	}

	template <typename TComp>
	TComp* EntityManager::GetComponent(unsigned int id){
		return (TComp*)componentPools[TComp::componentId]->Get(id);
	}

	template <typename TComp>
	void EntityManager::RemoveComponent(unsigned int id){
		entityMasks[id].reset(TComp::componentId);
	}

	unsigned int EntityManager::GetEntityCount(){
		return entityCount;
	}

	//Pool

	Pool::Pool(size_t chunkSize, size_t eltSize)
		:chunkSize(chunkSize), eltSize(eltSize), eltPerChunk(chunkSize / eltSize), count(0)
	{
	}

	void Pool::CheckIndex(size_t index){
		index++;
		while (index > count){
			Expand();
		}
	}

	char* Pool::Get(size_t index){
		unsigned int memIndex = index / eltPerChunk;
		return memory[memIndex] + (index % eltPerChunk * eltSize);
	}

	void Pool::Expand(){
		count += eltPerChunk;
		memory.push_back(new char[chunkSize]);
	}

	//ComponentPool

	template <typename TComp>
	ComponentPool<TComp>::ComponentPool()
		:Pool(8192, sizeof(TComp::Type))
	{
	}
}