#include "pch.h"
#include "Overlay.h"
#include "Loader.h"
#include "achievement_manager_ui.h"
#include <thread>
#include <future>

// Do I need this extern declaration?
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Adapted from: https://github.com/rdbo/ImGui-DirectX-11-Kiero-Hook
namespace Overlay{

#define POPUP_DURATION_MS	3000

Present oPresent;
HWND pWindow = nullptr;
WNDPROC oWndProc = nullptr;
ID3D11Device* pD3D11Device = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;

bool showAchievementManager = false;
bool showInitPopup = true;

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	if(uMsg == WM_KEYUP){
		// Shift + F5 pressed?
		if(GetKeyState(VK_SHIFT) & 0x8000 && wParam == VK_F5){
			// Hide the popup
			showInitPopup = false;

			// Toggle the overlay
			showAchievementManager = !showAchievementManager;
		}
	}

	if(showAchievementManager){
		// A mouse input fix I adapted from here:
		// https://niemand.com.ar/2019/01/01/how-to-hook-directx-11-imgui/
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		POINT mPos;
		GetCursorPos(&mPos);
		ScreenToClient(pWindow, &mPos);
		ImGui::GetIO().MousePos.x = (float) mPos.x;
		ImGui::GetIO().MousePos.y = (float) mPos.y;
		return true;
	} else{
		return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
	}
}

HRESULT __stdcall hookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags){
	static bool init = false;
	if(!init){
		if(SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**) &pD3D11Device))){
			pD3D11Device->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			pWindow = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer);
#pragma warning(suppress: 6387)
			pD3D11Device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC) SetWindowLongPtr(pWindow, GWLP_WNDPROC, (LONG_PTR) WndProc);
			AchievementManagerUI::initImGui(pWindow, pD3D11Device, pContext);
			init = true;
		} else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	// Now that we are hooked, it's time to render the Achivement Manager

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if(showAchievementManager)
		AchievementManagerUI::renderOverlay();

	if(showInitPopup)
		AchievementManagerUI::renderInitPopup();

	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


	return oPresent(pSwapChain, SyncInterval, Flags);
}

void InitThread(LPVOID lpReserved){
	while(kiero::init(kiero::RenderType::D3D11) != kiero::Status::Success);
	Logger::ovrly("Kiero: Successfully initialized");

	kiero::bind(8, (void**) &oPresent, hookedPresent);
	Logger::ovrly("Kiero: Successfully binded");

	// Hide the popup after POPUP_DURATION_MS time
	static auto hidePopupJob = std::async(std::launch::async, [&] (){
		Sleep(POPUP_DURATION_MS);
		showInitPopup = false;
	});
}

void Overlay::init(HMODULE hMod, Achievements& achievements, UnlockAchievementFunction* unlockAchievement){
	AchievementManagerUI::init(achievements, unlockAchievement);
	std::thread(InitThread, hMod).detach();
}

void Overlay::shutdown(){
	Logger::ovrly("Kiero: Shutting down");
	AchievementManagerUI::shutdownImGui();
	kiero::shutdown();
	// TODO: Clear the achievement vector as well?
}

}