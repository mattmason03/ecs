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
	struct MaskHelper;

	template <typename T, typename... TComp>
	struct MaskHelper < T, TComp... >
	{
	public:
		static Mask BuildMask(){
			Mask mask = MaskHelper<TComp...>::BuildMask();
			mask.set(T::componentId);
			return mask;
		}
	};

	template <>
	struct MaskHelper < >
	{
	public:
		static Mask BuildMask(){
			Mask mask;
			return mask;
		}
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

		template <typename TComp>
		void RegisterComponent();

		template <typename TComp, typename... TArgs>
		void AddComponent(unsigned int id, TArgs&&... args);

		template <typename TComp>
		TComp* GetComponent(unsigned int id);

		template <typename TComp>
		void RemoveComponent(unsigned int id);

		unsigned int GetEntityCount();

	private:
		unsigned int entityCount;
		std::vector<Mask> entityMasks;
		std::array<Pool*, MAX_COMPONENTS> componentPools;

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
		Mask mask = MaskHelper<TComp...>::BuildMask();
		std::vector<Entity> entities;
		for (unsigned int i = 0; i < entityCount; ++i){
			if ((mask & entityMasks.at(i)) == mask){
				entities.emplace_back(this, i);
			}
		}
		return entities;
	}

	template <typename TComp>
	void EntityManager::RegisterComponent(){
		TComp::Register();
		componentPools.at(TComp::componentId) = new ComponentPool < TComp >();
	}

	template <typename TComp, typename... TArgs>
	void EntityManager::AddComponent(unsigned int id, TArgs&&... args){
		unsigned int o = TComp::componentId;
		entityMasks.at(id).set(TComp::componentId);
		Pool* pool = componentPools.at(TComp::componentId);
		pool->CheckIndex(id);
		new(pool->Get(id)) TComp(std::forward<TArgs>(args)...);
	}

	template <typename TComp>
	TComp* EntityManager::GetComponent(unsigned int id){
		return (TComp*)componentPools.at(TComp::componentId)->Get(id);
	}

	template <typename TComp>
	void EntityManager::RemoveComponent(unsigned int id){
		entityMasks.at(id).reset(TComp::componentId);
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
		return memory.at(memIndex) + (index % eltPerChunk * eltSize);
	}

	void Pool::Expand(){
		count += eltPerChunk;
		memory.push_back(new char[chunkSize]);
	}

	//ComponentPool

	template <typename TComp>
	ComponentPool<TComp>::ComponentPool()
		:Pool(16384, sizeof(TComp::Type))
	{
	}
}