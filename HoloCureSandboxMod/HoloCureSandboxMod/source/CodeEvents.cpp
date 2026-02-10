#pragma comment(lib, "d3d11.lib")
#include "CommonFunctions.h"
#include "CodeEvents.h"
#include "ScriptFunctions.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_stdlib.h"
#include <d3d11.h>
#include <thread>

extern std::unordered_map<std::string, damageData> damagePerFrameMap;
extern CallbackManagerInterface* callbackManagerInterfacePtr;
extern HWND hWnd;

bool isInSandboxMenu = false;
bool prevIsTimePaused = false;
bool prevCanControl = false;
bool prevMouseFollowMode = false;
bool isSandboxMenuButtonPressed = false;

bool isTimePausedButtonPressed = false;
bool isProcessPausedButtonPressed = false;
bool isProcessPaused = false;
bool isNextFrameButtonPressed = false;
bool isNextFrame = false;
int curSandboxMenuPage = 0;
int selectedItemIndex = -1;
int dpsNumFrameCount = 0;

bool isEditorOpen = false;

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

ImGuiIO io;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

std::vector<std::pair<double, std::string>> damageList;
std::string editorCharacterData;

enum sandboxItemType
{
	SANDBOXITEMTYPE_Weapon,
	SANDBOXITEMTYPE_Item,
	SANDBOXITEMTYPE_Collab,
	SANDBOXITEMTYPE_Stamp,
	SANDBOXITEMTYPE_Perk,
};

struct sandboxShopItemData
{
	std::string itemName;
	int optionIcon;
	int curLevel;
	int maxLevel;
	bool canSuper;
	sandboxItemType itemType;

	sandboxShopItemData(std::string itemName, int optionIcon, int curLevel, int maxLevel, bool canSuper, sandboxItemType itemType) : itemName(itemName), optionIcon(optionIcon), curLevel(curLevel), maxLevel(maxLevel), canSuper(canSuper), itemType(itemType)
	{
	}
};

void handleSandboxItemMenuInteract(CInstance* playerManagerInstance, sandboxShopItemData& curItem, int buyOption);

void drawTextOutline(CInstance* Self, double xPos, double yPos, std::string text, double outlineWidth, int outlineColor, double numOutline, double linePixelSeparation, double pixelsBeforeLineBreak, int textColor, double alpha)
{
	RValue** args = new RValue*[10];
	args[0] = new RValue(xPos);
	args[1] = new RValue(yPos);
	args[2] = new RValue(text);
	args[3] = new RValue(outlineWidth);
	args[4] = new RValue(static_cast<double>(outlineColor));
	args[5] = new RValue(numOutline);
	args[6] = new RValue(linePixelSeparation);
	args[7] = new RValue(pixelsBeforeLineBreak);
	args[8] = new RValue(static_cast<double>(textColor));
	args[9] = new RValue(alpha);
	RValue returnVal;
	origDrawTextOutlineScript(Self, nullptr, returnVal, 10, args);
}

void setTimeToThirtyMin()
{
	RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
	timeArr[0] = 0;
	timeArr[1] = 29;
	timeArr[2] = 59;
	RValue stageManager = g_ModuleInterface->CallBuiltin("instance_find", { objStageManagerIndex, 0 });
	RValue mobSpawnChoices;
	g_RunnerInterface.StructCreate(&mobSpawnChoices);
	g_ModuleInterface->CallBuiltin("variable_struct_set", { stageManager, "mobSpawnChoices", mobSpawnChoices });
}

void setTimeToFortyMin()
{
	RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
	timeArr[0] = 0;
	timeArr[1] = 39;
	timeArr[2] = 59;
	RValue stageManager = g_ModuleInterface->CallBuiltin("instance_find", { objStageManagerIndex, 0 });
	RValue mobSpawnChoices;
	g_RunnerInterface.StructCreate(&mobSpawnChoices);
	g_ModuleInterface->CallBuiltin("variable_struct_set", { stageManager, "mobSpawnChoices", mobSpawnChoices });
}

void createAnvil()
{
	RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
	RValue xPos = getInstanceVariable(player, GML_x);
	RValue yPos = getInstanceVariable(player, GML_y);
	RValue depth = getInstanceVariable(player, GML_depth);
	RValue sticker = g_ModuleInterface->CallBuiltin("instance_create_depth", { xPos, yPos.m_Real - 20, depth, objHoloAnvilIndex });
}

void openEditor()
{
	if (g_pd3dDevice == nullptr)
	{
		// Create application window
		//ImGui_ImplWin32_EnableDpiAwareness();
		WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Editor Menu", nullptr };
		::RegisterClassExW(&wc);
		HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Editor Menu", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
		// Initialize Direct3D
		if (!CreateDeviceD3D(hwnd))
		{
			CleanupDeviceD3D();
			::UnregisterClassW(wc.lpszClassName, wc.hInstance);
			return;
		}

		// Show the window
		::ShowWindow(hwnd, SW_SHOWDEFAULT);
		::UpdateWindow(hwnd);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		ImGui::StyleColorsDark();
		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
		isEditorOpen = true;
	}
}

bool isKeyPressed(int vKey)
{
	return GetForegroundWindow() == hWnd && (GetAsyncKeyState(vKey) & 0xFFFE) != 0;
}

void framePauseThreadHandler()
{
	while (true)
	{
		if (isKeyPressed('N'))
		{
			if (!isProcessPausedButtonPressed)
			{
				isProcessPaused = !isProcessPaused;
				isProcessPausedButtonPressed = true;
			}
		}
		else
		{
			isProcessPausedButtonPressed = false;
		}
		if (isProcessPaused)
		{
			if (isKeyPressed('M'))
			{
				if (!isNextFrameButtonPressed)
				{
					isNextFrame = true;
					isNextFrameButtonPressed = true;
				}
			}
			else
			{
				isNextFrameButtonPressed = false;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

std::vector<sandboxMenuData*> sandboxOptionList = {
	new sandboxCheckBox(10, 90, false, "Immortal Enemies"),
	new sandboxCheckBox(10, 125, true, "DPS Tracker"),
	new sandboxCheckBox(10, 160, false, "Enable Debug"),
	new sandboxCheckBox(10, 195, false, "Show Mob Stats"),
	new sandboxButton(10, 230, setTimeToThirtyMin, "Go to 30:00"),
	new sandboxButton(10, 265, setTimeToFortyMin, "Go to 40:00"),
	new sandboxButton(10, 300, createAnvil, "Create Anvil"),
	new sandboxButton(10, 335, openEditor, "Open Editor"),
};

bool isLevelUpButtonPressed = false;
bool hasLevelUp = false;

void PlayerManagerStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasLevelUp)
	{
		hasLevelUp = false;
	}
}

void handleImGUI(CInstance* Self)
{
	ImGui::Begin("Editor - ");
	ImGui::InputTextMultiline("character data", &editorCharacterData);
	if (ImGui::Button("Export JSON"))
	{
		gameData curGameData;
		RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
		curGameData.time = ((timeArr[0].ToInt32() * 60 + timeArr[1].ToInt32()) * 60) + timeArr[2].ToInt32();
		RValue statUps = getInstanceVariable(Self, GML_playerStatUps);
		curGameData.statLevels.hp = getInstanceVariable(statUps, GML_HP).ToInt32();
		curGameData.statLevels.atk = getInstanceVariable(statUps, GML_ATK).ToInt32();
		curGameData.statLevels.spd = getInstanceVariable(statUps, GML_SPD).ToInt32();
		curGameData.statLevels.crit = getInstanceVariable(statUps, GML_crit).ToInt32();
		curGameData.statLevels.pickupRange = getInstanceVariable(statUps, GML_pickupRange).ToInt32();
		curGameData.statLevels.haste = getInstanceVariable(statUps, GML_haste).ToInt32();
		RValue playerCharacter = getInstanceVariable(Self, GML_playerCharacter);
		RValue buffs = getInstanceVariable(playerCharacter, GML_buffs);
		RValue buffNames = g_ModuleInterface->CallBuiltin("variable_struct_get_names", { buffs });
		int buffNamesLength = g_ModuleInterface->CallBuiltin("array_length", { buffNames }).ToInt32();
		for (int i = 0; i < buffNamesLength; i++)
		{
			RValue curName = buffNames[i];
			RValue curBuff = g_ModuleInterface->CallBuiltin("variable_struct_get", { buffs, curName });
			RValue config = getInstanceVariable(curBuff, GML_config);
			playerBuffData curBuffData;
			curBuffData.buffName = curName.ToString();
			RValue stacks = getInstanceVariable(config, GML_stacks);
			if (stacks.m_Kind != VALUE_UNDEFINED)
			{
				curBuffData.stacks = stacks.ToInt32();
			}
			RValue maxStacks = getInstanceVariable(config, GML_maxStacks);
			if (maxStacks.m_Kind != VALUE_UNDEFINED)
			{
				curBuffData.maxStacks = maxStacks.ToInt32();
			}
			RValue buffIcon = getInstanceVariable(config, GML_buffIcon);
			if (buffIcon.m_Kind != VALUE_UNDEFINED)
			{
				curBuffData.buffIcon = buffIcon.ToInt32();
			}
			curBuffData.duration = getInstanceVariable(curBuff, GML_timer).ToInt32();
			curGameData.buffDataList.push_back(curBuffData);
		}

		RValue itemMap = getInstanceVariable(Self, GML_ITEMS);
		RValue items = getInstanceVariable(Self, GML_items);
		RValue itemNames = g_ModuleInterface->CallBuiltin("variable_struct_get_names", { items });
		int itemNamesLength = g_ModuleInterface->CallBuiltin("array_length", { itemNames }).ToInt32();
		for (int i = 0; i < itemNamesLength; i++)
		{
			RValue curName = itemNames[i];
			RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemMap, curName });
			RValue level = getInstanceVariable(curItem, GML_level);
			RValue maxLevel = getInstanceVariable(curItem, GML_maxLevel);
			RValue optionIconSuper = getInstanceVariable(curItem, GML_optionIcon_Super);
			bool canSuper = optionIconSuper.m_Kind != VALUE_UNDEFINED && optionIconSuper.ToInt32() != sprBulletTempIndex;
			curGameData.itemDataList.push_back(playerItemData(curName.ToString(), level.ToInt32(), maxLevel.ToInt32(), canSuper));
		}

		RValue attacks = getInstanceVariable(playerCharacter, GML_attacks);
		RValue weapons = getInstanceVariable(Self, GML_weapons);
		RValue attacksNames = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { attacks });
		int attackNamesLength = g_ModuleInterface->CallBuiltin("array_length", { attacksNames }).ToInt32();
		for (int i = 0; i < attackNamesLength; i++)
		{
			RValue curName = attacksNames[i];
			RValue curWeapon = g_ModuleInterface->CallBuiltin("variable_instance_get", { weapons, curName });
			RValue level = getInstanceVariable(curWeapon, GML_level);
			RValue maxLevel = getInstanceVariable(curWeapon, GML_maxLevel);
			std::string optionType = getInstanceVariable(curWeapon, GML_optionType).ToString();
			bool isCollab = optionType.compare("Collab") == 0 || optionType.compare("SuperCollab") == 0;
			curGameData.weaponDataList.push_back(playerWeaponData(curName.ToString(), level.m_Kind == VALUE_UNDEFINED ? -1 : level.ToInt32(), maxLevel.m_Kind == VALUE_UNDEFINED ? -1 : maxLevel.ToInt32(), isCollab));
		}
		nlohmann::json outputJSON = curGameData;
		editorCharacterData = outputJSON.dump(4);
	}
	if (ImGui::Button("Apply JSON"))
	{
		try
		{
			gameData curGameData;
			nlohmann::json inputData = nlohmann::json::parse(editorCharacterData);
			callbackManagerInterfacePtr->LogToFile(MODNAME, "Applying json: %s", editorCharacterData.c_str());
			curGameData = inputData.template get<gameData>();
			RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
			int seconds = curGameData.time % 60;
			curGameData.time /= 60;
			int minutes = curGameData.time % 60;
			curGameData.time /= 60;
			int hours = curGameData.time;
			timeArr[0] = hours;
			timeArr[1] = minutes;
			timeArr[2] = seconds;
			timeArr[3] = 0;
			RValue statUps = getInstanceVariable(Self, GML_playerStatUps);
			setInstanceVariable(statUps, GML_HP, curGameData.statLevels.hp);
			setInstanceVariable(statUps, GML_ATK, curGameData.statLevels.atk);
			setInstanceVariable(statUps, GML_SPD, curGameData.statLevels.spd);
			setInstanceVariable(statUps, GML_crit, curGameData.statLevels.crit);
			setInstanceVariable(statUps, GML_pickupRange, curGameData.statLevels.pickupRange);
			setInstanceVariable(statUps, GML_haste, curGameData.statLevels.haste);
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			RValue playerCharacter = getInstanceVariable(Self, GML_playerCharacter);
			for (int i = 0; i < curGameData.buffDataList.size(); i++)
			{
				playerBuffData buffData = curGameData.buffDataList[i];
				RValue buffsMap = getInstanceVariable(attackController, GML_Buffs);
				RValue buffsMapData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { buffsMap, buffData.buffName.c_str() });
				RValue buffConfig;
				g_RunnerInterface.StructCreate(&buffConfig);
				setInstanceVariable(buffConfig, GML_reapply, true);
				setInstanceVariable(buffConfig, GML_stacks, buffData.stacks);
				setInstanceVariable(buffConfig, GML_maxStacks, buffData.maxStacks);
				setInstanceVariable(buffConfig, GML_buffName, buffData.buffName.c_str());
				setInstanceVariable(buffConfig, GML_buffIcon, buffData.buffIcon);

				RValue buffs = getInstanceVariable(playerCharacter, GML_buffs);
				RValue curBuff = g_ModuleInterface->CallBuiltin("variable_struct_get", { buffs, buffData.buffName.c_str() });
				if (curBuff.m_Kind != VALUE_UNDEFINED)
				{
					RValue config = getInstanceVariable(curBuff, GML_config);
					setInstanceVariable(config, GML_stacks, buffData.stacks);
					setInstanceVariable(config, GML_maxStacks, buffData.maxStacks);
				}

				RValue ApplyBuffMethod = getInstanceVariable(attackController, GML_ApplyBuff);
				RValue ApplyBuffArr = g_ModuleInterface->CallBuiltin("array_create", { 4 });
				ApplyBuffArr[0] = playerCharacter;
				ApplyBuffArr[1] = buffData.buffName.c_str();
				ApplyBuffArr[2] = buffsMapData;
				ApplyBuffArr[3] = buffConfig;
				g_ModuleInterface->CallBuiltin("method_call", { ApplyBuffMethod, ApplyBuffArr });
				
				curBuff = g_ModuleInterface->CallBuiltin("variable_struct_get", { buffs, buffData.buffName.c_str() });
				RValue config = getInstanceVariable(curBuff, GML_config);
				setInstanceVariable(config, GML_stacks, buffData.stacks);
			}

			RValue items = getInstanceVariable(Self, GML_items);
			RValue itemNames = g_ModuleInterface->CallBuiltin("variable_struct_get_names", { items });
			int itemNamesLength = g_ModuleInterface->CallBuiltin("array_length", { itemNames }).ToInt32();
			for (int i = 0; i < itemNamesLength; i++)
			{
				RValue curName = itemNames[i];
				sandboxShopItemData itemData(curName.ToString(), -1, -1, -1, false, SANDBOXITEMTYPE_Item);
				handleSandboxItemMenuInteract(Self, itemData, 1);
			}

			RValue weapons = getInstanceVariable(playerCharacter, GML_attacks);
			RValue weaponNames = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { weapons });
			int weaponNamesLength = g_ModuleInterface->CallBuiltin("array_length", { weaponNames }).ToInt32();
			for (int i = 0; i < weaponNamesLength; i++)
			{
				RValue curName = weaponNames[i];
				RValue curWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { weapons, curName });
				RValue config = getInstanceVariable(curWeapon, GML_config);
				std::string optionType = getInstanceVariable(config, GML_optionType).ToString();
				sandboxShopItemData itemData(curName.ToString(), -1, -1, -1, false, optionType.compare("Collab") == 0 || optionType.compare("SuperCollab") == 0 ? SANDBOXITEMTYPE_Collab : SANDBOXITEMTYPE_Weapon);
				handleSandboxItemMenuInteract(Self, itemData, 1);
			}

			for (int i = 0; i < curGameData.itemDataList.size(); i++)
			{
				playerItemData curItem = curGameData.itemDataList[i];
				sandboxShopItemData itemData(curItem.itemName, -1, curItem.level, curItem.maxLevel, curItem.canSuper, SANDBOXITEMTYPE_Item);
				if (curItem.canSuper && curItem.level == curItem.maxLevel - 1)
				{
					handleSandboxItemMenuInteract(Self, itemData, 0);
				}
				else
				{
					for (int j = 0; j <= curItem.level; j++)
					{
						itemData.curLevel = j;
						handleSandboxItemMenuInteract(Self, itemData, 0);
					}
				}
			}

			for (int i = 0; i < curGameData.weaponDataList.size(); i++)
			{
				playerWeaponData curItem = curGameData.weaponDataList[i];
				sandboxShopItemData itemData(curItem.weaponName, -1, 0, curItem.maxLevel, false, curItem.isCollab ? SANDBOXITEMTYPE_Collab : SANDBOXITEMTYPE_Weapon);
				for (int j = 0; j < curItem.level; j++)
				{
					itemData.curLevel = j;
					handleSandboxItemMenuInteract(Self, itemData, 0);
				}
			}

			RValue UpdatePlayerMethod = getInstanceVariable(Self, GML_UpdatePlayer);
			RValue UpdatePlayerMethodArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
			g_ModuleInterface->CallBuiltin("method_call", { UpdatePlayerMethod, UpdatePlayerMethodArr });
		}
		catch (nlohmann::json::parse_error& e)
		{
			DbgPrintEx(LOG_SEVERITY_ERROR, "Parse Error: %s when parsing character data JSON", e.what());
			callbackManagerInterfacePtr->LogToFile(MODNAME, "Parse Error: %s when parsing character data JSON", e.what());
		}
	}
	ImGui::End();
}

void PlayerManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	CInstance* Self = std::get<0>(Args);
	while (isProcessPaused)
	{
		if (isNextFrame)
		{
			isNextFrame = false;
			break;
		}
		PeekMessageA(nullptr, nullptr, 0, 0, PM_NOREMOVE);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	RValue paused = getInstanceVariable(Self, GML_paused);
	if (paused.ToBoolean())
	{
		return;
	}

	for (auto& damagePerFramePair : damagePerFrameMap)
	{
		damageData& curDamageData = damagePerFramePair.second;
		curDamageData.damageQueue.push_back(curDamageData.curFrameDamage);
		// Keep 3 seconds of damage data
		if (curDamageData.damageQueue.size() > 180)
		{
			curDamageData.damageQueue.pop_front();
		}
		curDamageData.curFrameDamage = 0;
	}

	if (isKeyPressed('U'))
	{
		if (!isLevelUpButtonPressed)
		{
			// Check how the room_goto code works
			isLevelUpButtonPressed = true;
			hasLevelUp = true;
			RValue playerlevel = g_ModuleInterface->CallBuiltin("variable_global_get", { "PLAYERLEVEL" });
			RValue levelrate = g_ModuleInterface->CallBuiltin("variable_instance_get", { Self, "lvlrate1" });
			RValue lvlexponent = g_ModuleInterface->CallBuiltin("variable_instance_get", { Self, "lvlexponent" });
			RValue power = g_ModuleInterface->CallBuiltin("power", { (playerlevel.ToDouble() + 1) * levelrate.ToDouble(), lvlexponent });
			RValue round = g_ModuleInterface->CallBuiltin("round", { power });
			g_ModuleInterface->CallBuiltin("variable_global_set", { "PLAYERLEVEL", round });
		}
	}
	else
	{
		isLevelUpButtonPressed = false;
	}

	bool hasUpdatedTimePaused = false;
	if (isKeyPressed('Y'))
	{
		if (!isSandboxMenuButtonPressed)
		{
			isSandboxMenuButtonPressed = true;
			isInSandboxMenu = !isInSandboxMenu;
			bool isTimePaused = false;
			RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
			if (isInSandboxMenu)
			{
				prevIsTimePaused = g_ModuleInterface->CallBuiltin("variable_global_get", { "timePause" }).ToBoolean();
				isTimePaused = true;
				prevCanControl = getInstanceVariable(player, GML_canControl).ToBoolean();
				prevMouseFollowMode = getInstanceVariable(player, GML_mouseFollowMode).ToBoolean();
				setInstanceVariable(player, GML_canControl, false);
				setInstanceVariable(player, GML_mouseFollowMode, false);
				curSandboxMenuPage = 0;
			}
			else
			{
				isTimePaused = prevIsTimePaused;
				setInstanceVariable(player, GML_canControl, prevCanControl);
				setInstanceVariable(player, GML_mouseFollowMode, prevMouseFollowMode);
				selectedItemIndex = -1;
			}
			hasUpdatedTimePaused = true;
			g_ModuleInterface->CallBuiltin("variable_global_set", { "timePause", isTimePaused });
		}
	}
	else
	{
		isSandboxMenuButtonPressed = false;
	}

	if (!isInSandboxMenu)
	{
		if (isKeyPressed('P'))
		{
			if (!isTimePausedButtonPressed)
			{
				isTimePausedButtonPressed = true;
				bool isTimePaused = g_ModuleInterface->CallBuiltin("variable_global_get", { "timePause" }).ToBoolean();
				g_ModuleInterface->CallBuiltin("variable_global_set", { "timePause", !isTimePaused });
				hasUpdatedTimePaused = true;
			}
		}
		else
		{
			isTimePausedButtonPressed = false;
		}
	}

	if (hasUpdatedTimePaused)
	{
		bool isTimePaused = g_ModuleInterface->CallBuiltin("variable_global_get", { "timePause" }).ToBoolean();
		if (isTimePaused)
		{
			int enemyCount = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("instance_number", { objEnemyIndex }).ToDouble()));
			for (int i = 0; i < enemyCount; i++)
			{
				RValue instance = g_ModuleInterface->CallBuiltin("instance_find", { objEnemyIndex, i });
				int instanceID = static_cast<int>(lround(instance.ToDouble()));
				CInstance* objectInstance = nullptr;
				if (!AurieSuccess(g_ModuleInterface->GetInstanceObject(instanceID, objectInstance)))
				{
					continue;
				}
				RValue result;
				origCompleteStopBaseMobCreateScript(objectInstance, objectInstance, result, 0, nullptr);
			}
		}
		else
		{
			int enemyCount = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("instance_number", { objEnemyIndex }).ToDouble()));
			for (int i = 0; i < enemyCount; i++)
			{
				RValue instance = g_ModuleInterface->CallBuiltin("instance_find", { objEnemyIndex, i });
				int instanceID = static_cast<int>(lround(instance.ToDouble()));
				CInstance* objectInstance = nullptr;
				if (!AurieSuccess(g_ModuleInterface->GetInstanceObject(instanceID, objectInstance)))
				{
					continue;
				}
				RValue result;
				origEndStopBaseMobCreateScript(objectInstance, objectInstance, result, 0, nullptr);
			}
		}
	}

	if (isEditorOpen)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				isEditorOpen = false;
				g_pd3dDevice = nullptr;
				return;
			}
		}

		// Handle window being minimized or screen locked
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			return;
		}
		g_SwapChainOccluded = false;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		handleImGUI(Self);

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present
		HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
		//HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
		g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
	}
}

void handleSandboxItemMenuInteract(CInstance* playerManagerInstance, sandboxShopItemData& curItem, int buyOption)
{
	switch (curItem.itemType)
	{
		case SANDBOXITEMTYPE_Weapon:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				RValue itemName = curItem.itemName.c_str();
				RValue returnVal;
				RValue** args = new RValue*[1];
				args[0] = &itemName;
				origAddAttackPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
			}
			else
			{
				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue attacks = getInstanceVariable(player, GML_attacks);
				RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curItem.itemName.c_str()});
				if (playerWeapon.m_Kind == VALUE_UNDEFINED)
				{
					return;
				}
				RValue config = getInstanceVariable(playerWeapon, GML_config);
				RValue optionID = getInstanceVariable(config, GML_optionID);
				RValue weapons = getInstanceVariable(playerManagerInstance, GML_weapons);
				RValue curWeapon = g_ModuleInterface->CallBuiltin("variable_struct_get", { weapons, optionID });
				setInstanceVariable(curWeapon, GML_level, 0);
				g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, optionID });
			}
			break;
		}
		case SANDBOXITEMTYPE_Item:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				if (curItem.curLevel == curItem.maxLevel - 1 && curItem.canSuper)
				{
					RValue itemName = curItem.itemName.c_str();
					RValue itemsMap = getInstanceVariable(playerManagerInstance, GML_ITEMS);
					RValue item = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, itemName });
					RValue maxLevel = getInstanceVariable(item, GML_maxLevel);
					setInstanceVariable(item, GML_becomeSuper, true);
					setInstanceVariable(item, GML_optionIcon, getInstanceVariable(item, GML_optionIcon_Super));
					RValue returnVal;
					RValue** args = new RValue*[1];
					args[0] = &itemName;
					origAddItemPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				}
				else
				{
					RValue itemName = curItem.itemName.c_str();
					RValue returnVal;
					RValue** args = new RValue*[1];
					args[0] = &itemName;
					origAddItemPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				}
			}
			else
			{
				RValue itemsMap = getInstanceVariable(playerManagerInstance, GML_ITEMS);
				RValue itemName = curItem.itemName.c_str();
				RValue item = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, itemName });
				setInstanceVariable(item, GML_level, -1.0);
				RValue returnVal;
				RValue** args = new RValue*[1];
				args[0] = &itemName;
				origRemoveOwnedItemPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				RValue UpdatePlayerMethod = getInstanceVariable(playerManagerInstance, GML_UpdatePlayer);
				RValue UpdatePlayerMethodArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
				g_ModuleInterface->CallBuiltin("method_call", { UpdatePlayerMethod, UpdatePlayerMethodArr });
				setInstanceVariable(item, GML_becomeSuper, false);
				setInstanceVariable(item, GML_optionIcon, getInstanceVariable(item, GML_optionIcon_Normal));
			}
			break;
		}
		case SANDBOXITEMTYPE_Collab:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				RValue itemName = curItem.itemName.c_str();
				RValue returnVal;
				RValue** args = new RValue*[1];
				args[0] = &itemName;

				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue attacks = getInstanceVariable(player, GML_attacks);

				RValue weaponCollabs = getInstanceVariable(playerManagerInstance, GML_weaponCollabs);
				RValue weapon = g_ModuleInterface->CallBuiltin("variable_instance_get", { weaponCollabs, itemName });
				RValue config = getInstanceVariable(weapon, GML_config);
				RValue combos = getInstanceVariable(config, GML_combos);
				
				RValue attackComboOne = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, combos[0] });
				RValue attackComboTwo = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, combos[1] });
				RValue tempWeapon;
				RValue tempConfig;
				RValue tempGainedMods = g_ModuleInterface->CallBuiltin("array_create", { 0 });
				g_RunnerInterface.StructCreate(&tempWeapon);
				g_RunnerInterface.StructCreate(&tempConfig);
				g_RunnerInterface.StructAddRValue(&tempConfig, "gainedMods", &tempGainedMods);
				g_RunnerInterface.StructAddRValue(&tempWeapon, "config", &tempConfig);
				if (attackComboOne.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, "attackComboOne", attackComboOne });
				}
				if (attackComboTwo.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, "attackComboTwo", attackComboTwo });
				}
				g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[0], tempWeapon });
				g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[1], tempWeapon });

				origAddCollabPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
				if (attackComboOne.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[0], attackComboOne });
					g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, "attackComboOne" });
				}
				if (attackComboTwo.m_Kind != VALUE_UNDEFINED)
				{
					g_ModuleInterface->CallBuiltin("ds_map_set", { attacks, combos[1], attackComboTwo });
					g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, "attackComboTwo" });
				}
			}
			else
			{
				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue attacks = getInstanceVariable(player, GML_attacks);
				RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curItem.itemName.c_str()});
				if (playerWeapon.m_Kind == VALUE_UNDEFINED)
				{
					return;
				}
				RValue config = getInstanceVariable(playerWeapon, GML_config);
				RValue optionID = getInstanceVariable(config, GML_optionID);
				RValue weaponCollabs = getInstanceVariable(playerManagerInstance, GML_weaponCollabs);
				RValue curWeapon = g_ModuleInterface->CallBuiltin("variable_struct_get", { weaponCollabs, optionID });
				setInstanceVariable(curWeapon, GML_level, 0);
				g_ModuleInterface->CallBuiltin("ds_map_delete", { attacks, optionID });
			}
			break;
		}
		case SANDBOXITEMTYPE_Stamp:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
				RValue xPos = getInstanceVariable(player, GML_x);
				RValue yPos = getInstanceVariable(player, GML_y);
				RValue depth = getInstanceVariable(player, GML_depth);
				RValue sticker = g_ModuleInterface->CallBuiltin("instance_create_depth", { xPos, yPos, depth, objStickerIndex });
				RValue stickersMap = getInstanceVariable(playerManagerInstance, GML_STICKERS);
				RValue stickerData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { stickersMap, curItem.itemName.c_str()});
				setInstanceVariable(sticker, GML_stickerData, stickerData);
				setInstanceVariable(sticker, GML_sprite_index, curItem.optionIcon);
				RValue availableStickers = g_ModuleInterface->CallBuiltin("variable_global_get", { "availableStickers" });
				int availableStickersLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { availableStickers }).m_Real);
				for (int i = 0; i < availableStickersLength; i++)
				{
					if (availableStickers[i].ToString().compare(curItem.itemName) == 0)
					{
						g_ModuleInterface->CallBuiltin("array_delete", { availableStickers, i, 1 });
						break;
					}
				}
			}
			break;
		}
		case SANDBOXITEMTYPE_Perk:
		{
			if (buyOption == 0)
			{
				if (curItem.curLevel == curItem.maxLevel)
				{
					return;
				}
				RValue itemName = curItem.itemName.c_str();
				RValue returnVal;
				RValue** args = new RValue*[1];
				args[0] = &itemName;
				origAddPerkPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
			}
			else
			{
				RValue perksMap = getInstanceVariable(playerManagerInstance, GML_PERKS);
				RValue perk = g_ModuleInterface->CallBuiltin("ds_map_find_value", { perksMap, curItem.itemName.c_str() });
				setInstanceVariable(perk, GML_level, -1.0);
				RValue perks = getInstanceVariable(playerManagerInstance, GML_perks);
				g_ModuleInterface->CallBuiltin("struct_remove", { perks, curItem.itemName.c_str() });
			}
			break;
		}
		default:
		{
			DbgPrintEx(LOG_SEVERITY_ERROR, "ERROR: COULDN'T HANDLE SANDBOX ITEM INTERACT");
		}
	}
}

int getSpriteIndexFromAttackName(std::string& attackID)
{
	if (attackID.empty())
	{
		return sprUnknownIconButtonIndex;
	}

	RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
	RValue attackIndex = getInstanceVariable(attackController, GML_attackIndex);
	if (attackIndex.m_Kind == VALUE_UNDEFINED)
	{
		return sprUnknownIconButtonIndex;
	}
	RValue curWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attackIndex, attackID.c_str()});
	if (curWeapon.m_Kind != VALUE_UNDEFINED)
	{
		RValue config = getInstanceVariable(curWeapon, GML_config);
		int optionIcon = static_cast<int>(lround(getInstanceVariable(config, GML_optionIcon).ToDouble()));
		if (optionIcon == sprBulletTempIndex)
		{
			return sprUnknownIconButtonIndex;
		}
		return optionIcon;
	}
	
	return sprUnknownIconButtonIndex;
}

int cursorLifetime = 0;
int buyOptionIndex = 0;

void PlayerManagerDraw64After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	CInstance* Self = std::get<0>(Args);
	RValue paused = getInstanceVariable(Self, GML_paused);
	RValue player = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerIndex, 0 });
	setInstanceVariable(player, GML_hpSus, 0);
	if (isInSandboxMenu)
	{
		if (paused.ToBoolean())
		{
			return;
		}

		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		RValue attackIndex = getInstanceVariable(attackController, GML_attackIndex);

		if (attackIndex.m_Kind == VALUE_UNDEFINED)
		{
			return;
		}

		RValue inputManager = g_ModuleInterface->CallBuiltin("instance_find", { objInputManagerIndex, 0 });
		RValue actionTwoPressed = getInstanceVariable(inputManager, GML_actionTwoPressed);
		RValue isRightMouseButtonPressed = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 2 });
		if (actionTwoPressed.ToBoolean() || isRightMouseButtonPressed.ToBoolean())
		{
			selectedItemIndex = -1;
		}

		std::vector<sandboxShopItemData> itemDataList;

		RValue attacks = getInstanceVariable(player, GML_attacks);
		RValue playerSave = g_ModuleInterface->CallBuiltin("variable_global_get", { "PlayerSave" });
		RValue unlockedWeapons = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerSave, "unlockedWeapons" });
		int unlockedWeaponsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { unlockedWeapons }).m_Real);
		for (int i = 0; i < unlockedWeaponsLength; i++)
		{
			RValue curWeaponName = unlockedWeapons[i];
			RValue curWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attackIndex, curWeaponName });
			RValue config = getInstanceVariable(curWeapon, GML_config);
			RValue optionIcon = getInstanceVariable(config, GML_optionIcon);
			RValue maxLevel = getInstanceVariable(config, GML_maxLevel);
			int curLevel = 0;
			RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curWeaponName });
			if (playerWeapon.m_Kind != VALUE_UNDEFINED)
			{
				RValue playerWeaponConfig = getInstanceVariable(playerWeapon, GML_config);
				curLevel = static_cast<int>(lround(getInstanceVariable(playerWeaponConfig, GML_level).m_Real));
			}
			itemDataList.push_back(sandboxShopItemData(std::string(curWeaponName.ToString()), static_cast<int>(lround(optionIcon.ToDouble())), curLevel, static_cast<int>(lround(maxLevel.m_Real)), false, SANDBOXITEMTYPE_Weapon));
		}

		RValue weaponCollabs = getInstanceVariable(Self, GML_weaponCollabs);
		RValue weaponCollabNames = g_ModuleInterface->CallBuiltin("struct_get_names", { weaponCollabs });
		int weaponCollabsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { weaponCollabNames }).m_Real);
		for (int i = 0; i < weaponCollabsLength; i++)
		{
			RValue curWeaponName = weaponCollabNames[i];
			RValue curWeapon = g_ModuleInterface->CallBuiltin("variable_struct_get", { weaponCollabs, curWeaponName });
			RValue config = getInstanceVariable(curWeapon, GML_config);
			RValue optionIcon = getInstanceVariable(config, GML_optionIcon);
			int curLevel = 0;
			RValue playerWeapon = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curWeaponName });
			if (playerWeapon.m_Kind != VALUE_UNDEFINED)
			{
				RValue playerWeaponConfig = getInstanceVariable(playerWeapon, GML_config);
				curLevel = static_cast<int>(lround(getInstanceVariable(playerWeaponConfig, GML_level).m_Real));
			}
			itemDataList.push_back(sandboxShopItemData(std::string(curWeaponName.ToString()), static_cast<int>(lround(optionIcon.ToDouble())), curLevel, 1, false, SANDBOXITEMTYPE_Collab));
		}

		RValue itemsMap = getInstanceVariable(Self, GML_ITEMS);
		RValue items = getInstanceVariable(Self, GML_items);
		RValue unlockedItems = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerSave, "unlockedItems" });
		int unlockedItemsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { unlockedItems }).m_Real);
		for (int i = 0; i < unlockedItemsLength; i++)
		{
			RValue curItemsName = unlockedItems[i];
			RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, curItemsName });
			RValue optionIconNormal = getInstanceVariable(curItem, GML_optionIcon_Normal);
			RValue maxLevel = getInstanceVariable(curItem, GML_maxLevel);
			RValue optionIconSuper = getInstanceVariable(curItem, GML_optionIcon_Super);
			bool canSuper = optionIconSuper.m_Kind != VALUE_UNDEFINED && static_cast<int>(optionIconSuper.ToDouble()) != sprBulletTempIndex;
			int curLevel = static_cast<int>(lround(getInstanceVariable(curItem, GML_level).m_Real)) + 1;
			if (canSuper)
			{
				bool isSuper = curLevel >= static_cast<int>(lround(maxLevel.m_Real) + 1);
				itemDataList.push_back(sandboxShopItemData(std::string(curItemsName.ToString()), static_cast<int>(lround(optionIconSuper.ToDouble())), isSuper, 1, true, SANDBOXITEMTYPE_Item));
				if (isSuper)
				{
					curLevel = 0;
				}
			}
			itemDataList.push_back(sandboxShopItemData(std::string(curItemsName.ToString()), static_cast<int>(lround(optionIconNormal.ToDouble())), curLevel, lround(maxLevel.m_Real), false, SANDBOXITEMTYPE_Item));
		}

		RValue availableStickers = g_ModuleInterface->CallBuiltin("variable_global_get", { "availableStickers" });
		RValue stickersMap = getInstanceVariable(Self, GML_STICKERS);
		RValue stickersKeysArr = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { stickersMap });
		int stickersKeysLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { stickersKeysArr }).m_Real);
		for (int i = 0; i < stickersKeysLength; i++)
		{
			RValue curStickerName = stickersKeysArr[i];
			RValue curSticker = g_ModuleInterface->CallBuiltin("ds_map_find_value", { stickersMap, curStickerName });
			RValue optionIcon = getInstanceVariable(curSticker, GML_optionIcon);
			int level = static_cast<int>(!g_ModuleInterface->CallBuiltin("array_contains", { availableStickers, curStickerName }).ToBoolean());
			itemDataList.push_back(sandboxShopItemData(std::string(curStickerName.ToString()), static_cast<int>(lround(optionIcon.ToDouble())), level, 1, false, SANDBOXITEMTYPE_Stamp));
		}

		RValue PerksMap = getInstanceVariable(Self, GML_PERKS);
		RValue PerksArr = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { PerksMap });
		int PerksLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { PerksArr }).m_Real);
		for (int i = 0; i < PerksLength; i++)
		{
			RValue curPerkName = PerksArr[i];
			RValue curPerk = g_ModuleInterface->CallBuiltin("ds_map_find_value", { PerksMap, curPerkName });
			RValue optionIcon = getInstanceVariable(curPerk, GML_optionIcon);
			int curLevel = static_cast<int>(lround(getInstanceVariable(curPerk, GML_level).m_Real)) + 1;
			itemDataList.push_back(sandboxShopItemData(curPerkName.ToString(), static_cast<int>(lround(optionIcon.ToDouble())), curLevel, 3, false, SANDBOXITEMTYPE_Perk));
		}
		
		// Handle drawing sandbox item menu
		for (int i = curSandboxMenuPage * 16; i < itemDataList.size() && i < (curSandboxMenuPage + 1) * 16; i++)
		{
			std::string curWeaponName = itemDataList[i].itemName;
			int maxLevel = itemDataList[i].maxLevel;
			int optionIcon = itemDataList[i].optionIcon;
			int curLevel = itemDataList[i].curLevel;
			int rowIndex = (i - curSandboxMenuPage * 16) / 4;
			int colIndex = i % 4;
			int curXPos = 279 + colIndex * 90;
			int curYPos = 80 + rowIndex * 45;

			g_ModuleInterface->CallBuiltin("draw_set_alpha", { 0.7 });
			g_ModuleInterface->CallBuiltin("draw_set_color", { 0 });
			g_ModuleInterface->CallBuiltin("draw_rectangle", { curXPos - 18, curYPos - 18 + 30, curXPos + 18, curYPos + 18 + 30, false});
			g_ModuleInterface->CallBuiltin("draw_set_alpha", { 1 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopItemBGIndex, 0, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { optionIcon, 0, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopIconIndex, 0, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopLevelsEmptyIndex, maxLevel, curXPos, curYPos + 30 });
			g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopLevelsFilledIndex, curLevel, curXPos, curYPos + 30 });

			RValue** args = new RValue*[4];
			RValue mouseOverType = "shopIcon";
			RValue curButtonX = curXPos;
			RValue curButtonY = curYPos + 30;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 3, args);

			cursorLifetime++;
			if (cursorLifetime >= 62832)
			{
				cursorLifetime = 0;
			}

			if (selectedItemIndex == -1 && returnVal.ToBoolean() || selectedItemIndex == i)
			{
				g_ModuleInterface->CallBuiltin("draw_sprite", { sprShopIconSelectedIndex, 0, curXPos, curYPos + 30 });
				g_ModuleInterface->CallBuiltin("draw_set_alpha", { .05 + abs(.15 * sin(cursorLifetime / 100.0)) });
				g_ModuleInterface->CallBuiltin("draw_rectangle_color", { curXPos - 21, curYPos - 21 + 30, curXPos + 65, curYPos + 21 + 30, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0 });
				g_ModuleInterface->CallBuiltin("draw_set_alpha", { 1 });
			}

			if (selectedItemIndex == -1 && returnVal.ToBoolean())
			{
				RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
				RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
				if (actionOnePressed.ToBoolean() || isIconSelected.ToBoolean())
				{
					selectedItemIndex = i;
					buyOptionIndex = 0;
				}
			}
		}

		int selectedColor[2] = { 0xFFFFFF, 0 };
		if (selectedItemIndex != -1)
		{
			RValue textContainer = g_ModuleInterface->CallBuiltin("variable_global_get", { "TextContainer" });
			RValue shopItemButtons = getInstanceVariable(textContainer, GML_shopItemButtons);
			RValue selectedLanguage = getInstanceVariable(shopItemButtons, GML_selectedLanguage);
			for (int i = 0; i < 2; i++)
			{
				if (i == 1 && itemDataList[selectedItemIndex].itemType == SANDBOXITEMTYPE_Stamp)
				{
					break;
				}
				int buttonPosX = 400 + i * 90;
				int buttonPosY = 310;
				int curColor = selectedColor[static_cast<int>(buyOptionIndex == i)];
				g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudShopButtonIndex, buyOptionIndex == i, buttonPosX, buttonPosY, 1, 1, 0, 0xFFFFFF, 1 });
				g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
				g_ModuleInterface->CallBuiltin("draw_text_color", { buttonPosX, buttonPosY - 5, selectedLanguage[i], curColor, curColor, curColor, curColor, 1 });

				RValue** args = new RValue*[4];
				RValue mouseOverType = "short";
				RValue curButtonX = buttonPosX;
				RValue curButtonY = buttonPosY;
				args[0] = &mouseOverType;
				args[1] = &curButtonX;
				args[2] = &curButtonY;
				RValue returnVal;
				origMouseOverButtonScript(Self, nullptr, returnVal, 3, args);
				if (returnVal.ToBoolean())
				{
					buyOptionIndex = i;
					RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
					RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
					if (actionOnePressed.ToBoolean() || isIconSelected.ToBoolean())
					{
						handleSandboxItemMenuInteract(Self, itemDataList[selectedItemIndex], buyOptionIndex);
					}
				}
			}
		}

		// Draw Next and Prev buttons
		for (int i = 0; i < 2; i++)
		{
			if (i == 0 && curSandboxMenuPage == 0 || i == 1 && curSandboxMenuPage == (itemDataList.size() - 1) / 16)
			{
				continue;
			}
			int buttonPosX = 300 + i * 270;
			int buttonPosY = 310;
			bool isButtonMouseOver = false;
			RValue** args = new RValue*[4];
			RValue mouseOverType = "short";
			RValue curButtonX = buttonPosX;
			RValue curButtonY = buttonPosY;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 3, args);
			if (returnVal.ToBoolean())
			{
				isButtonMouseOver = true;
				RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
				RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
				if (actionOnePressed.ToBoolean() || isIconSelected.ToBoolean())
				{
					if (i == 0)
					{
						curSandboxMenuPage--;
						selectedItemIndex = -1;
					}
					else if (i == 1)
					{
						curSandboxMenuPage++;
						selectedItemIndex = -1;
					}
				}
			}

			int curColor = selectedColor[static_cast<int>(isButtonMouseOver)];
			std::string buttonText = i == 0 ? "Prev" : "Next";
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudShopButtonIndex, isButtonMouseOver, buttonPosX, buttonPosY, 1, 1, 0, 0xFFFFFF, 1 });
			g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
			g_ModuleInterface->CallBuiltin("draw_text_color", { buttonPosX, buttonPosY - 5, buttonText.c_str(), curColor, curColor, curColor, curColor, 1});
		}

		// Handle sandbox option menu
		for (int i = 0; i < sandboxOptionList.size(); i++)
		{
			int xPos = sandboxOptionList[i]->xPos;
			int yPos = sandboxOptionList[i]->yPos;
			bool isButtonMouseOver = false;
			RValue** args = new RValue*[4];
			RValue mouseOverType = "long";
			RValue curButtonX = xPos + 90;
			RValue curButtonY = yPos;
			RValue screenSize = 2;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			args[3] = &screenSize;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 4, args);
			if (returnVal.ToBoolean())
			{
				isButtonMouseOver = true;
				RValue actionOnePressed = getInstanceVariable(inputManager, GML_actionOnePressed);
				RValue isIconSelected = g_ModuleInterface->CallBuiltin("mouse_check_button_pressed", { 1 });
				if (actionOnePressed.ToBoolean() || isIconSelected.ToBoolean())
				{
					if (sandboxOptionList[i]->sandboxMenuDataType == SANDBOXMENUDATATYPE_CheckBox)
					{
						sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[i]);
						checkBox->isChecked = !checkBox->isChecked;
						// Enable Debug option
						if (i == 2)
						{
							g_ModuleInterface->CallBuiltin("variable_global_set", { "debug", checkBox->isChecked });
						}
					}
					else if (sandboxOptionList[i]->sandboxMenuDataType == SANDBOXMENUDATATYPE_Button)
					{
						sandboxButton* button = reinterpret_cast<sandboxButton*>(sandboxOptionList[i]);
						button->onClickFunc();
					}
				}
			}
			int curColor = selectedColor[static_cast<int>(isButtonMouseOver)];
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudOptionButtonIndex, isButtonMouseOver, xPos + 90, yPos, 1, 1, 0, 0xFFFFFF, 1 });
			if (sandboxOptionList[i]->sandboxMenuDataType == SANDBOXMENUDATATYPE_CheckBox)
			{
				sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[i]);
				g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudToggleButtonIndex, isButtonMouseOver * 2 + checkBox->isChecked, xPos + 10 + 11, yPos + 11 + 2, 1, 1, 0, 0xFFFFFF, 1 });
			}
			g_ModuleInterface->CallBuiltin("draw_text_color", { xPos + 100, yPos + 9, sandboxOptionList[i]->text.c_str(), curColor, curColor, curColor, curColor, 1});
		}
	}

	if (!paused.ToBoolean() && !isInSandboxMenu)
	{
		sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[1]);
		if (checkBox->isChecked)
		{
			if (dpsNumFrameCount >= 10)
			{
				dpsNumFrameCount = 0;
				damageList.clear();
				for (auto& damagePerFramePair : damagePerFrameMap)
				{
					double totalDamage = 0;
					int count = 0;
					for (auto damage : damagePerFramePair.second.damageQueue)
					{
						totalDamage += damage;
						count++;
					}
					damageList.push_back(std::pair<double, std::string>(totalDamage / count * 60, damagePerFramePair.first));
				}
				sort(damageList.begin(), damageList.end());
			}

			for (int i = 0; i < 8 && i < damageList.size(); i++)
			{
				int curIndex = static_cast<int>(damageList.size()) - i - 1;
				int spriteIndex = getSpriteIndexFromAttackName(damageList[curIndex].second);
				g_ModuleInterface->CallBuiltin("draw_sprite", { spriteIndex, 0, 30, 110 + i * 30 });
				g_ModuleInterface->CallBuiltin("draw_set_halign", { 0 });
				g_ModuleInterface->CallBuiltin("draw_text_color", { 30 + 30, 110 + i * 30, std::format("{:.5e}", damageList[curIndex].first).c_str(), 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 1});
				if (spriteIndex == sprUnknownIconButtonIndex)
				{
					std::string text = "UNKNOWN ID";
					if (!damageList[curIndex].second.empty())
					{
						text = damageList[curIndex].second;
					}
					g_ModuleInterface->CallBuiltin("draw_text_color", { 30 + 100, 110 + i * 30, text.c_str(), 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 1});
				}
			}
		}
	}
	dpsNumFrameCount++;
}

void PlayerMouse54Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (isInSandboxMenu)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}

void PlayerManagerAlarm0Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", 0 });
	g_ModuleInterface->CallBuiltin("variable_global_set", { "haluLevel", 0 });
}

void BaseMobDrawAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	sandboxCheckBox* checkBox = reinterpret_cast<sandboxCheckBox*>(sandboxOptionList[3]);
	if (checkBox->isChecked)
	{
		CInstance* Self = std::get<0>(Args);
		RValue xPos = getInstanceVariable(Self, GML_x);
		RValue yPos = getInstanceVariable(Self, GML_y);
		RValue curHP = getInstanceVariable(Self, GML_currentHP);
		RValue maxHP = getInstanceVariable(Self, GML_HP);
		RValue atk = getInstanceVariable(Self, GML_ATK);
		RValue spd = getInstanceVariable(Self, GML_SPD);
		std::string text = std::format("HP: {} / {}", static_cast<int>(lround(curHP.ToDouble())), static_cast<int>(lround(maxHP.ToDouble())));
		g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
		drawTextOutline(Self, xPos.ToDouble(), yPos.ToDouble() + 5, text, 1, 0x000000, 14, 0, 400, 0xFFFFFF, 1);
		text = std::format("ATK: {:.3e}", atk.ToDouble());
		drawTextOutline(Self, xPos.ToDouble(), yPos.ToDouble() + 15, text, 1, 0x000000, 14, 0, 400, 0xFFFFFF, 1);
		text = std::format("SPD: {:.3e}", spd.ToDouble());
		drawTextOutline(Self, xPos.ToDouble(), yPos.ToDouble() + 25, text, 1, 0x000000, 14, 0, 400, 0xFFFFFF, 1);
	}
}

void parseJSONToVar(const nlohmann::json& inputJson, const char* varName, std::string& outputStr)
{
	try
	{
		inputJson.at(varName).get_to(outputStr);
	}
	catch (nlohmann::json::type_error& e)
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Type Error: %s when parsing var %s to string", e.what(), varName);
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Type Error: %s when parsing var %s to string", e.what(), varName);
	}
	catch (nlohmann::json::out_of_range& e)
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Out of Range Error: %s when parsing var %s to string", e.what(), varName);
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Out of Range Error: %s when parsing var %s to string", e.what(), varName);
	}
}

void parseJSONToVar(const nlohmann::json& inputJson, const char* varName, int& outputInt)
{
	try
	{
		inputJson.at(varName).get_to(outputInt);
	}
	catch (nlohmann::json::type_error& e)
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Type Error: %s when parsing var %s to int", e.what(), varName);
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Type Error: %s when parsing var %s to int", e.what(), varName);
	}
	catch (nlohmann::json::out_of_range& e)
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Out of Range Error: %s when parsing var %s to int", e.what(), varName);
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Out of Range Error: %s when parsing var %s to int", e.what(), varName);
	}
}

void parseJSONToVar(const nlohmann::json& inputJson, const char* varName, bool& outputBoolean)
{
	try
	{
		inputJson.at(varName).get_to(outputBoolean);
	}
	catch (nlohmann::json::type_error& e)
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Type Error: %s when parsing var %s to boolean", e.what(), varName);
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Type Error: %s when parsing var %s to boolean", e.what(), varName);
	}
	catch (nlohmann::json::out_of_range& e)
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Out of Range Error: %s when parsing var %s to boolean", e.what(), varName);
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Out of Range Error: %s when parsing var %s to boolean", e.what(), varName);
	}
}

void to_json(nlohmann::json& outputJson, const gameData& inputGameData)
{
	outputJson = nlohmann::json{
		{ "time", inputGameData.time },
		{ "statLevels", inputGameData.statLevels },
		{ "buffData", inputGameData.buffDataList },
		{ "weaponData", inputGameData.weaponDataList },
		{ "itemData", inputGameData.itemDataList },
	};
}

void from_json(const nlohmann::json& inputJson, gameData& outputGameData)
{
	parseJSONToVar(inputJson, "time", outputGameData.time);
	from_json(inputJson["statLevels"], outputGameData.statLevels);
	from_json(inputJson["buffData"], outputGameData.buffDataList);
	from_json(inputJson["weaponData"], outputGameData.weaponDataList);
	from_json(inputJson["itemData"], outputGameData.itemDataList);
}

void to_json(nlohmann::json& outputJson, const playerStatLevels& inputStatLevels)
{
	outputJson = nlohmann::json{
		{ "hp", inputStatLevels.hp },
		{ "atk", inputStatLevels.atk },
		{ "spd", inputStatLevels.spd },
		{ "crit", inputStatLevels.crit },
		{ "pickupRange", inputStatLevels.pickupRange },
		{ "haste", inputStatLevels.haste },
	};
}

void from_json(const nlohmann::json& inputJson, playerStatLevels& outputStatLevels)
{
	parseJSONToVar(inputJson, "hp", outputStatLevels.hp);
	parseJSONToVar(inputJson, "atk", outputStatLevels.atk);
	parseJSONToVar(inputJson, "spd", outputStatLevels.spd);
	parseJSONToVar(inputJson, "crit", outputStatLevels.crit);
	parseJSONToVar(inputJson, "pickupRange", outputStatLevels.pickupRange);
	parseJSONToVar(inputJson, "haste", outputStatLevels.haste);
}

void to_json(nlohmann::json& outputJson, const playerBuffData& inputBuffData)
{
	outputJson = nlohmann::json{
		{ "name", inputBuffData.buffName },
		{ "duration", inputBuffData.duration },
		{ "stacks", inputBuffData.stacks },
		{ "maxStacks", inputBuffData.maxStacks },
		{ "buffIcon", inputBuffData.buffIcon },
	};
}

void from_json(const nlohmann::json& inputJson, playerBuffData& outputBuffData)
{
	parseJSONToVar(inputJson, "name", outputBuffData.buffName);
	parseJSONToVar(inputJson, "duration", outputBuffData.duration);
	parseJSONToVar(inputJson, "stacks", outputBuffData.stacks);
	parseJSONToVar(inputJson, "maxStacks", outputBuffData.maxStacks);
	parseJSONToVar(inputJson, "buffIcon", outputBuffData.buffIcon);
}

void to_json(nlohmann::json& outputJson, const playerWeaponData& inputWeaponData)
{
	outputJson = nlohmann::json{
		{ "name", inputWeaponData.weaponName },
		{ "level", inputWeaponData.level },
		{ "maxLevel", inputWeaponData.maxLevel },
		{ "isCollab", inputWeaponData.isCollab },
	};
}

void from_json(const nlohmann::json& inputJson, playerWeaponData& outputWeaponData)
{
	parseJSONToVar(inputJson, "name", outputWeaponData.weaponName);
	parseJSONToVar(inputJson, "level", outputWeaponData.level);
	parseJSONToVar(inputJson, "maxLevel", outputWeaponData.maxLevel);
	parseJSONToVar(inputJson, "isCollab", outputWeaponData.isCollab);
}

void to_json(nlohmann::json& outputJson, const playerItemData& inputItemData)
{
	outputJson = nlohmann::json{
		{ "name", inputItemData.itemName },
		{ "level", inputItemData.level },
		{ "maxLevel", inputItemData.maxLevel },
		{ "canSuper", inputItemData.canSuper },
	};
}

void from_json(const nlohmann::json& inputJson, playerItemData& outputItemData)
{
	parseJSONToVar(inputJson, "name", outputItemData.itemName);
	parseJSONToVar(inputJson, "level", outputItemData.level);
	parseJSONToVar(inputJson, "maxLevel", outputItemData.maxLevel);
	parseJSONToVar(inputJson, "canSuper", outputItemData.canSuper);
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::DestroyWindow(hWnd);
		isEditorOpen = false;
		g_pd3dDevice = nullptr;
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}