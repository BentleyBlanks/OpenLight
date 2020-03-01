#include "Scene.h"

Scene::Scene()
{
}

void Scene::AddObject(const GameObject* Object)
{
	Objects.push_back(Object);
}

int Scene::GetNumObject()
{
	return (int)Objects.size();
}

const GameObject* Scene::GetVisibiltyObject(int index)
{
	if (index < 0 || index >= Objects.size())	return nullptr;

	return Objects[index];
}
