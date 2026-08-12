#pragma once

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"

//------------------------------------------------------------------------------------------------------------------
class App
{

public:

	App();
	~App();
	
	void Startup();
	void Shutdown();
	void RunFrame();

	void SetCursorVisibility();
	void RunMainFrame();
	void RequestQuit();
	static bool RequestQuitEvent(EventArgs& args);
	void ResetGame();
	
	void LoadConfigFile(char const* configFilePath);
	bool isQuitting() const;

private:

	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

private:

	bool m_isQuitting = false;

};