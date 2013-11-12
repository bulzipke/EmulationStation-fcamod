#include "ViewController.h"
#include "../Log.h"
#include "../SystemData.h"

#include "BasicGameListView.h"
#include "DetailedGameListView.h"

ViewController::ViewController(Window* window)
	: GuiComponent(window), mCurrentView(nullptr), mCameraPos(Eigen::Affine3f::Identity())
{
	mState.viewing = START_SCREEN;
}

void ViewController::goToSystemSelect()
{
	mState.viewing = SYSTEM_SELECT;
	goToSystem(SystemData::sSystemVector.at(0));
}

SystemData* getSystemCyclic(SystemData* from, bool reverse)
{
	std::vector<SystemData*>& sysVec = SystemData::sSystemVector;

	if(reverse)
	{
		auto it = std::find(sysVec.rbegin(), sysVec.rend(), from);
		assert(it != sysVec.rend());
		it++;
		if(it == sysVec.rend())
			it = sysVec.rbegin();
		return *it;
	}else{
		auto it = std::find(sysVec.begin(), sysVec.end(), from);
		assert(it != sysVec.end());
		it++;
		if(it == sysVec.end())
			it = sysVec.begin();
		return *it;
	}
}

void ViewController::goToNextSystem()
{
	assert(mState.viewing == SYSTEM);

	SystemData* system = mState.data.system;
	if(system == NULL)
		return;
	
	goToSystem(getSystemCyclic(system, false));
}

void ViewController::goToPrevSystem()
{
	assert(mState.viewing == SYSTEM);

	SystemData* system = mState.data.system;
	if(system == NULL)
		return;
	
	goToSystem(getSystemCyclic(system, true));
}

void ViewController::goToSystem(SystemData* system)
{
	mState.viewing = SYSTEM;
	mState.data.system = system;

	mCurrentView = getSystemView(system);
}

void ViewController::onFileChanged(FileData* file, FileChangeType change)
{
	for(auto it = mSystemViews.begin(); it != mSystemViews.end(); it++)
	{
		it->second->onFileChanged(file, change);
	}
}

void ViewController::launch(FileData* game)
{
	if(game->getType() != GAME)
	{
		LOG(LogError) << "tried to launch something that isn't a game";
		return;
	}

	// Effect TODO
	game->getSystem()->launchGame(mWindow, game);
}

std::shared_ptr<GameListView> ViewController::getSystemView(SystemData* system)
{
	//if we already made one, return that one
	auto exists = mSystemViews.find(system);
	if(exists != mSystemViews.end())
		return exists->second;

	//if we didn't, make it, remember it, and return it
	std::shared_ptr<GameListView> view;

	if(system != NULL)
	{
		view = std::shared_ptr<GameListView>(new DetailedGameListView(mWindow, system->getRootFolder()));
		view->setTheme(system->getTheme());
	}else{
		LOG(LogError) << "null system"; // should eventually return an "all games" gamelist view
	}

	std::vector<SystemData*>& sysVec = SystemData::sSystemVector;
	int id = std::find(sysVec.begin(), sysVec.end(), system) - sysVec.begin();
	view->setPosition(id * (float)Renderer::getScreenWidth(), 0);

	mSystemViews[system] = view;
	return view;
}



bool ViewController::input(InputConfig* config, Input input)
{
	if(mCurrentView)
		return mCurrentView->input(config, input);

	return false;
}


float clamp(float min, float max, float val)
{
	if(val < min)
		val = min;
	else if(val > max)
		val = max;

	return val;
}

//http://en.wikipedia.org/wiki/Smoothstep
float smoothStep(float edge0, float edge1, float x)
{
	// Scale, and clamp x to 0..1 range
	x = clamp(0, 1, (x - edge0)/(edge1 - edge0));
        
	// Evaluate polynomial
	return x*x*x*(x*(x*6 - 15) + 10);
}

void ViewController::update(int deltaTime)
{
	if(mCurrentView)
	{
		mCurrentView->update(deltaTime);

		// move camera towards current view (should use smoothstep)
		Eigen::Vector3f diff = (mCurrentView->getPosition() + mCameraPos.translation()) * 0.0075f * (float)deltaTime;
		mCameraPos.translate(-diff);
	}
}

void ViewController::render(const Eigen::Affine3f& parentTrans)
{
	Eigen::Affine3f trans = parentTrans * mCameraPos;

	//should really do some clipping here
	for(auto it = mSystemViews.begin(); it != mSystemViews.end(); it++)
		it->second->render(trans);
}
