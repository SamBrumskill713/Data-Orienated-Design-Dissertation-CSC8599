#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

#include "PhysicsSystem.h"
#include "TutorialGameDOD.h"
#include "PhysicsObjectDOD.h"
#include "GameTechRendererDOD.h"

#include "TutorialGameSOA.h"
#include "PhysicsSystemSOA.h"
#include "GameWorldSOA.h"
#include "GameTechRendererSOA.h"

#ifdef USEOPENGL
#include "GameTechRenderer.h"
#define CAN_COMPILE
#endif
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#define CAN_COMPILE
#endif

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>
#include <atomic>

static std::atomic<bool> gReturnToMenu{ false };
static std::atomic<bool> gStartSOABenchmark{ false };

vector<Vector3> testNodes;

void TestPathfinding() {
	NavigationGrid grid("TestLevel.txt");

	NavigationPath outPath;

	Vector3 startPos(80, 0, 10);
	Vector3 endPos(80, 0, 80);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}

void DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];
		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}

void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void {
		std::cout << "I'm in state A!\n";
		data++;
		}
	);

	State* B = new State([&](float dt)->void {
		std::cout << "I'm in state B\n";
		data--;
		}
	);

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool {
		return data > 10;
		}
	);

	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool {
		return data < 0;
		}
	);
	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(1.0f);
	}
}

void TestBehaviourTree() {
	float behaviourTimer;
	float distanceToTarget;
	BehaviourAction* findKey = new BehaviourAction("Find Key",
		[&](float dt, BehaviourState state)->BehaviourState {
			if (state == Initialise) {
				behaviourTimer = rand() % 100;
				state = Ongoing;
			}
			else if (state == Ongoing) {
				behaviourTimer -= dt;
				if (behaviourTimer <= 0.0f) {
					std::cout << "Found the key!\n";
					return Success;
				}
			}
			return state;
		});
	BehaviourAction* goToRoom = new BehaviourAction("Go To Room",
		[&](float dt, BehaviourState state)->BehaviourState {
			if (state == Initialise) {
				std::cout << "Going to the room!\n";
				state = Ongoing;
			}
			else if (state == Ongoing) {
				distanceToTarget -= dt;
				if (distanceToTarget <= 0.0f) {
					std::cout << "Reached the room!\n";
					return Success;
				}
			}
			return state;
		});

	BehaviourAction* openDoor = new BehaviourAction("Open Door",
		[&](float dt, BehaviourState state)->BehaviourState {
			if (state == Initialise) {
				std::cout << "Opening the door!\n";
				return Success;
			}
			return state;
		});

	BehaviourAction* lookForTresure = new BehaviourAction("Look For Treasure",
		[&](float dt, BehaviourState state)->BehaviourState {
			if (state == Initialise) {
				std::cout << "Looking for treasure!\n";
				return Ongoing;
			}
			else if (state == Ongoing) {
				bool found = rand() % 2;
				if (found) {
					std::cout << "Found the treasure!\n";
					return Success;
				}
			}
			return state;
		});

	BehaviourAction* lookForItems = new BehaviourAction("Look For Items",
		[&](float dt, BehaviourState state)->BehaviourState {
			if (state == Initialise) {
				std::cout << "Looking for items!\n";
				return Ongoing;
			}
			else if (state == Ongoing) {
				bool found = rand() % 2;
				if (found) {
					std::cout << "Found some items!\n";
					return Success;
				}
				std::cout << "No items here...\n";
				return Failure;
			}
			return state;
		});

	BehaviourSequence* sequence =
		new BehaviourSequence("Room Sequence");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSelector* selection =
		new BehaviourSelector("Loot Selection");
	selection->AddChild(lookForTresure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence =
		new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 5; ++i) {
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		std::cout << "We're Going on an Adventure!\n";
		while (state == Ongoing) {
			state = rootSequence->Execute(1.0f);
		}
		if (state == Success) {
			std::cout << "What a successful adventure!\n";
		}
		else if (state == Failure) {
			std::cout << "What a waste of time!\n";
		}
	}
	std::cout << "All Done!\n";
}

class PauseScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::U)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Press U to unpause game!\n";
	}
};

class GameScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		pauseReminder -= dt;
		if (pauseReminder < 0) {
			std::cout << "Coins mined: " << coinsMinded << "\n";
			std::cout << "Press P to pause game, or F1 to return to main menu!\n";
			pauseReminder += 1.0f;
		}
		if (Window::GetKeyboard()->KeyDown(KeyCodes::P)) {
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyDown(KeyCodes::F1)) {
			std::cout << "Returning to main menu!\n";
			return PushdownResult::Pop;
		}
		if (rand() % 7 == 0) {
			coinsMinded++;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Preparing to mine coins!\n";
	}
protected:
	int coinsMinded = 0;
	float pauseReminder = 1;
};

class IntroScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
			*newState = new GameScreen();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Welcome to the Game!\n";
		std::cout << "Press Space to start, or Escape to quit.\n";
	}
};

void TestPushdownAutomata(Window* w) {
	PushdownMachine machine(new IntroScreen());
	while (w->UpdateWindow()) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (!machine.Update(dt)) {
			return;
		}
	}
}

class TestPacketReceiver : public PacketReceiver
{
public:
	TestPacketReceiver(std::string name)
	{
		this->name = name;
	}

	void ReceivePacket(int type, GamePacket* payload, int source)
	{
		if (type == String_Message)
		{
			StringPacket* realPacket = (StringPacket*)payload;

			std::string msg = realPacket->GetStringFromData();

			std::cout << name << " recieved message: " << msg << std::endl;
		}
	}
protected:
	std::string name;
};

void TestNetworking()
{
	///*
	NetworkBase::Initialise();

	TestPacketReceiver serverReceiver("Server");
	TestPacketReceiver clientReceiver("Client");

	int port = NetworkBase::GetDefaultPort();

	GameServer* server = new GameServer(port, 1);
	GameClient* client = new GameClient();

	server->RegisterPacketHandler(String_Message, &serverReceiver);
	client->RegisterPacketHandler(String_Message, &clientReceiver);

	bool canConnect = client->Connect(127, 0, 0, 1, port);

	for (int i = 0; i < 100; i++)
	{
		StringPacket p("Server says hello! " + std::to_string(i));
		server->SendGlobalPacket(p);

		p = StringPacket("Client says hello! " + std::to_string(i));
		client->SendPacket(p);

		server->UpdateServer();
		client->UpdateClient();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	NetworkBase::Destroy();
	//*/
}

class IntroMenuState : public PushdownState {
public:
	explicit IntroMenuState(TutorialGame*& game, GameWorld* gw, PhysicsSystem* phys, Window* win,
		GameTechRendererInterface* rend)
		: gameRefPtr(game), world(gw), physics(phys), window(win), renderer(rend) {
	}

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		HandleInput();
		DrawMenu();

		if (confirmPressed) {
			const std::string& choice = options[currentIndex];

			if (choice == "Play") {
				if (gameRefPtr) {
					gameRefPtr->InitWorld();
				}
				confirmPressed = false;
				return PushdownResult::Pop; // proceed to gameplay
			}

			if (choice == "DOD(AOS) Benchmark") {
				confirmPressed = false;
				gReturnToMenu = true; // Use this flag to signal DOD mode
				return PushdownResult::Pop;
			}

			if (choice == "DOD(SOA) Benchmark") {
				confirmPressed = false;
				gReturnToMenu = false;
				gStartSOABenchmark = true;
				return PushdownResult::Pop;
			}

			if (choice == "Host Online") {
				// Swap TutorialGame -> NetworkedGame and host
				if (gameRefPtr) {
					delete gameRefPtr;
					gameRefPtr = nullptr;
				}
				gameRefPtr = new NetworkedGame(*world, *renderer, *physics);
				static_cast<NetworkedGame*>(gameRefPtr)->StartAsServer();
				confirmPressed = false;
				return PushdownResult::Pop;
			}

			if (choice == "Join Online") {
				// Swap TutorialGame -> NetworkedGame and connect to localhost
				if (gameRefPtr) {
					delete gameRefPtr;
					gameRefPtr = nullptr;
				}
				gameRefPtr = new NetworkedGame(*world, *renderer, *physics);
				static_cast<NetworkedGame*>(gameRefPtr)->StartAsClient(127, 0, 0, 1);
				confirmPressed = false;
				return PushdownResult::Pop;
			}

			if (choice == "Quit") {
				Window::DestroyGameWindow();
				confirmPressed = false;
				return PushdownResult::NoChange;
			}
		}

		if (quitRequested) {
			Window::DestroyGameWindow();
			quitRequested = false;
			return PushdownResult::NoChange;
		}

		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		currentIndex = 0;
		confirmPressed = false;
		quitRequested = false;
	}

private:
	void HandleInput() {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::UP)) {
			if (currentIndex > 0) currentIndex--;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::DOWN)) {
			if (currentIndex + 1 < static_cast<int>(options.size())) currentIndex++;
		}
		confirmPressed = Window::GetKeyboard()->KeyPressed(KeyCodes::RETURN) ||
			Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE);
		quitRequested = Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE);
	}

	void DrawMenu() const {
		Debug::Print("CSC8503 Game Technology", Vector2(30, 20), Debug::WHITE);
		Debug::Print("Use Up/Down to choose, ENTER to confirm", Vector2(20, 30), Debug::WHITE);

		const float startY = 50.0f;
		const float lineStep = 8.0f;
		for (int i = 0; i < static_cast<int>(options.size()); ++i) {
			const bool selected = (i == currentIndex);
			const Vector4 normalCol = Debug::WHITE;
			const Vector4 highlightCol = Debug::YELLOW;
			std::string text = selected ? ("> " + options[i]) : ("  " + options[i]);
			Debug::Print(text, Vector2(40, startY + i * lineStep), selected ? highlightCol : normalCol);
		}
	}

	TutorialGame*& gameRefPtr;
	GameWorld* world = nullptr;
	PhysicsSystem* physics = nullptr;
	Window* window = nullptr;
	GameTechRendererInterface* renderer = nullptr;

	std::vector<std::string> options{ "Play", "DOD(AOS) Benchmark", "DOD(SOA) Benchmark", "Host Online", "Join Online", "Quit" };
	int currentIndex = 0;
	bool confirmPressed = false;
	bool quitRequested = false;
};

class EndGameState : public PushdownState {
public:
	EndGameState(TutorialGame* game, Window* win, bool didWin, int score)
		: gameRef(game), window(win), winState(didWin), finalScore(score) {
	}

	void OnAwake() override {}

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		const Vector4 titleCol = winState ? Debug::GREEN : Debug::RED;
		Debug::Print(winState ? "YOU WIN!" : "GAME OVER", Vector2(40, 30), titleCol);
		Debug::Print("Score: " + std::to_string(finalScore), Vector2(40, 40), Debug::WHITE);

		Debug::Print("Press ENTER to Restart", Vector2(40, 55), Debug::YELLOW);
		Debug::Print("Press M to Main Menu", Vector2(40, 63), Debug::YELLOW);
		Debug::Print("Press ESC to Quit", Vector2(40, 71), Debug::YELLOW);

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::RETURN) ||
			Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
			if (gameRef) {
				gameRef->ClearEndState(); // re-init world and clear flags
			}
			return PushdownResult::Pop; // back to gameplay
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::M)) {
			// Clear end flags and let GamePlayState push the menu
			if (gameRef) {
				gameRef->ClearEndState();
			}
			gReturnToMenu = true;
			return PushdownResult::Pop; // remove EndGameState from stack
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
			Window::DestroyGameWindow();
			return PushdownResult::NoChange;
		}
		return PushdownResult::NoChange;
	}

private:
	TutorialGame* gameRef = nullptr;
	Window* window = nullptr;
	bool winState = false;
	int finalScore = 0;
};

// --- GamePlayState ---
class GamePlayState : public PushdownState {
public:
	GamePlayState(TutorialGame* game, GameWorld* gw, PhysicsSystem* phys, Window* win, GameTechRendererInterface* rend)
		: gameRef(game), world(gw), physics(phys), window(win), renderer(rend) {
	}

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			return PushdownResult::NoChange;
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::PRIOR)) {
			window->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::NEXT)) {
			window->ShowConsole(false);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::T)) {
			window->SetWindowPosition(0, 0);
		}

		float fps = (dt > 0.0) ? 1.0f / dt : 0.0f;
		window->SetTitle("GameTech FPS: " + std::to_string((int)fps) + " | Gametech frame time: " + std::to_string(1000.0f * dt));

		// Core game updates
		gameRef->UpdateGame(dt);
		world->UpdateWorld(dt);
		physics->Update(dt);

		// If a return to menu was requested, push the IntroMenuState
		if (gReturnToMenu.load()) {
			gReturnToMenu = false;
			*newState = new IntroMenuState(gameRef, world, physics, window, renderer);
			return PushdownResult::Push;
		}
		return PushdownResult::NoChange;
	}

private:
	TutorialGame* gameRef = nullptr;
	GameWorld* world = nullptr;
	PhysicsSystem* physics = nullptr;
	Window* window = nullptr;
	GameTechRendererInterface* renderer = nullptr; 
};

int main() {
	WindowInitialisation initInfo;
	initInfo.width = 1280;
	initInfo.height = 720;
	initInfo.windowTitle = "CSC8503 Game technology!";

	Window* w = Window::CreateGameWindow(initInfo);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	GameWorld* world = new GameWorld();
	PhysicsSystem* physics = new PhysicsSystem(*world);

#ifdef USEVULKAN
	GameTechVulkanRenderer* renderer = new GameTechVulkanRenderer(*world);
#elif USEOPENGL
	GameTechRenderer* renderer = new GameTechRenderer(*world);
#endif

	TutorialGame* g = new TutorialGame(*world, *renderer, *physics);

	// Main loop - menu and gameplay
	while (w->UpdateWindow()) {
		// Show menu
		{
			PushdownMachine menuMachine(new IntroMenuState(g, world, physics, w, renderer));
			while (w->UpdateWindow()) {
				float dt = w->GetTimer().GetTimeDeltaSeconds();
				if (!menuMachine.Update(dt)) {
					break;
				}

				renderer->Update(dt);
				renderer->Render();
				renderer->SetVerticalSync(VerticalSyncState::VSync_OFF);
				Debug::UpdateRenderables(dt);
			}
		}

		// Check what was selected
		if (gReturnToMenu.load()) {
			// DOD mode selected
			gReturnToMenu = false;

			// [Keep existing DOD benchmark code unchanged...]
			world->Clear();
			w->UpdateWindow();

			GameWorldDOD* worldDOD = new GameWorldDOD();
			RendererSystemDOD rendererDOD;
			GameTechRendererData frameData;
			rendererDOD.Initialise(w);
			rendererDOD.SetVerticalSync(0);

			PhysicsSystemDOD* physicsDOD = new PhysicsSystemDOD(*worldDOD);
			TutorialGameDOD* gameDOD = new TutorialGameDOD(*worldDOD, rendererDOD, *physicsDOD);

			bool dodBenchmarkRunning = true;
			int frameCount = 0;
			double totalTime = 0.0;
			std::vector<float> frameTimes;
			frameTimes.reserve(500);

			while (w->UpdateWindow() && dodBenchmarkRunning) {
				float dt = w->GetTimer().GetTimeDeltaSeconds();

				if (dt > 0.1f) {
					std::cout << "Skipping massive frame: " << dt << "s" << std::endl;
					continue;
				}

				gameDOD->UpdateGame(dt);

				frameData.viewMatrix = worldDOD->GetMainCamera().BuildViewMatrix();
				frameData.projMatrix = worldDOD->GetMainCamera().BuildProjectionMatrix(w->GetScreenAspect());
				frameData.cameraPos = worldDOD->GetMainCamera().GetPosition();

				rendererDOD.RenderFrame(*worldDOD, frameData);
				rendererDOD.swapBuffers();
				Debug::UpdateRenderables(dt);

				frameCount++;
				totalTime += dt;
				frameTimes.push_back(dt);

				if (frameCount % 1 == 0) {
					float currentFps = (dt > 0.0f) ? 1.0f / dt : 0.0f;
					w->SetTitle("GameTech DOD FPS: " + std::to_string((int)currentFps) +
						" (Frame " + std::to_string(frameCount) + ")");
				}

				if (Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
					std::sort(frameTimes.begin(), frameTimes.end());
					float minFrameTime = frameTimes.front();
					float maxFrameTime = frameTimes.back();
					float avgFrameTime = totalTime / frameCount;
					float medianFrameTime = frameTimes[frameCount / 2];

					std::cout << "\n=== DOD Benchmark Results ===" << std::endl;
					std::cout << "Total Frames: " << frameCount << std::endl;
					std::cout << "Total Time: " << totalTime << " seconds" << std::endl;
					std::cout << "Average FPS: " << (frameCount / totalTime) << std::endl;
					std::cout << "\nFrame Time Statistics:" << std::endl;
					std::cout << "  Min: " << (minFrameTime * 1000.0f) << " ms (" << (1.0f / minFrameTime) << " FPS)" << std::endl;
					std::cout << "  Max: " << (maxFrameTime * 1000.0f) << " ms (" << (1.0f / maxFrameTime) << " FPS)" << std::endl;
					std::cout << "  Avg: " << (avgFrameTime * 1000.0f) << " ms (" << (1.0f / avgFrameTime) << " FPS)" << std::endl;
					std::cout << "  Med: " << (medianFrameTime * 1000.0f) << " ms (" << (1.0f / medianFrameTime) << " FPS)" << std::endl;
					std::cout << "============================\n" << std::endl;

					dodBenchmarkRunning = false;
				}
			}
			delete gameDOD;
			gameDOD = nullptr;
			delete physicsDOD;
			physicsDOD = nullptr;
			delete worldDOD;
			worldDOD = nullptr;
			rendererDOD.Destroy();
		}
		else if (gStartSOABenchmark.load()) {
			// SOA benchmark mode selected
			gStartSOABenchmark = false;
			world->Clear();
			w->UpdateWindow();

			GameWorldSOA* worldSOA = new GameWorldSOA();
			RendererSystemSOA rendererSOA;
			GameTechRendererDataSOA frameData;
			rendererSOA.Initialise(w);
			rendererSOA.SetVerticalSync(0);

			PhysicsSystemSOA* physicsSOA = new PhysicsSystemSOA(*worldSOA);
			TutorialGameSOA* gameSOA = new TutorialGameSOA(*worldSOA, rendererSOA, *physicsSOA);

			bool soaBenchmarkRunning = true;
			int frameCount = 0;
			double totalTime = 0.0;
			std::vector<float> frameTimes;
			frameTimes.reserve(500);

			while (w->UpdateWindow() && soaBenchmarkRunning) {
				float dt = w->GetTimer().GetTimeDeltaSeconds();

				if (dt > 0.1f) {
					std::cout << "Skipping massive frame: " << dt << "s" << std::endl;
					continue;
				}

				gameSOA->UpdateGame(dt);

				frameData.viewMatrix = worldSOA->GetMainCamera().BuildViewMatrix();
				frameData.projMatrix = worldSOA->GetMainCamera().BuildProjectionMatrix(w->GetScreenAspect());
				frameData.cameraPos = worldSOA->GetMainCamera().GetPosition();

				rendererSOA.RenderFrame(*worldSOA, frameData);
				rendererSOA.SwapBuffers();
				Debug::UpdateRenderables(dt);

				frameCount++;
				totalTime += dt;
				frameTimes.push_back(dt);

				if (frameCount % 1 == 0) {
					float currentFps = (dt > 0.0f) ? 1.0f / dt : 0.0f;
					w->SetTitle("GameTech SOA FPS: " + std::to_string((int)currentFps) +
						" (Frame " + std::to_string(frameCount) + ")");
				}

				if (Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
					std::sort(frameTimes.begin(), frameTimes.end());
					float minFrameTime = frameTimes.front();
					float maxFrameTime = frameTimes.back();
					float avgFrameTime = totalTime / frameCount;
					float medianFrameTime = frameTimes[frameCount / 2];

					std::cout << "\n=== SOA Benchmark Results ===" << std::endl;
					std::cout << "Total Frames: " << frameCount << std::endl;
					std::cout << "Total Time: " << totalTime << " seconds" << std::endl;
					std::cout << "Average FPS: " << (frameCount / totalTime) << std::endl;
					std::cout << "\nFrame Time Statistics:" << std::endl;
					std::cout << "  Min: " << (minFrameTime * 1000.0f) << " ms (" << (1.0f / minFrameTime) << " FPS)" << std::endl;
					std::cout << "  Max: " << (maxFrameTime * 1000.0f) << " ms (" << (1.0f / maxFrameTime) << " FPS)" << std::endl;
					std::cout << "  Avg: " << (avgFrameTime * 1000.0f) << " ms (" << (1.0f / avgFrameTime) << " FPS)" << std::endl;
					std::cout << "  Med: " << (medianFrameTime * 1000.0f) << " ms (" << (1.0f / medianFrameTime) << " FPS)" << std::endl;
					std::cout << "============================\n" << std::endl;

					soaBenchmarkRunning = false;
				}
			}
			delete gameSOA;
			gameSOA = nullptr;
			delete physicsSOA;
			physicsSOA = nullptr;
			delete worldSOA;
			worldSOA = nullptr;
			rendererSOA.Destroy();
		}
		else {
			// Standard gameplay
			PushdownMachine gameMachine(new GamePlayState(g, world, physics, w, renderer));

			while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
				float dt = w->GetTimer().GetTimeDeltaSeconds();
				if (!gameMachine.Update(dt)) {
					break;
				}

				renderer->Update(dt);
				renderer->Render();
				renderer->SetVerticalSync(VerticalSyncState::VSync_OFF);
				Debug::UpdateRenderables(dt);
			}
		}
	}

	Window::DestroyGameWindow();
	return 0;
}