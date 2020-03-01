#pragma once

#include <vector>

class GameObject;

class Scene
{
public:
	Scene();
	
	void AddObject(const GameObject* Object);

	int GetNumObject();

	const GameObject* GetVisibiltyObject(int index = -1);

private:
	std::vector<const GameObject*> Objects;
};