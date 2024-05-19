#pragma once

#include "GameCore.h"

using namespace gcore;

class ModelViewer : public IGameApp
{
public:
	ModelViewer(void) {}

	void Startup(void) override;
	void Cleanup(void) override;

	void Update(float deltaT) override;
	void RenderScene(void) override;
};

