#include <fstream>

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "soccer/gui/Menu.h"
#include "soccer/gui/LeagueScreen.h"
#include "soccer/gui/CupScreen.h"
#include "soccer/gui/SeasonScreen.h"

namespace Soccer {

SeasonScreen::SeasonScreen(boost::shared_ptr<ScreenManager> sm, boost::shared_ptr<Season> s)
	: Screen(sm),
	mSeason(s)
{
	addButton("Back",  Common::Rectangle(0.01f, 0.90f, 0.23f, 0.06f));
	addButton("Save",  Common::Rectangle(0.01f, 0.83f, 0.23f, 0.06f));
	mNextRoundButton = addButton("Next Round", Common::Rectangle(0.26f, 0.90f, 0.73f, 0.06f));
	addMatchPlan();
}

void SeasonScreen::onReentry()
{
	addMatchPlan();
}

void SeasonScreen::addMatchPlan()
{
	for(auto l : mMatchPlanLabels) {
		removeButton(l);
	}
	mMatchPlanLabels.clear();

	auto s = mSeason->getLeague()->getSchedule();
	float x = 0.75f;
	float y = 0.03f;
	for(unsigned int i = 0; i < s.getNumberOfRounds(); i++) {
		auto r = s.getRound(i);
		for(auto m : r->getMatches()) {
			if(m->getTeam(0) == mSeason->getTeam() ||
					m->getTeam(1) == mSeason->getTeam()) {
				CompetitionScreen::addMatchLabels(*m, x, y, 0.6f,
						*this, mMatchPlanLabels);
				y += 0.03f;
				if(y >= 0.85f)
					return;
			}
		}
	}
}

std::string SeasonScreen::ScreenName = std::string("Season Screen");

void SeasonScreen::buttonPressed(boost::shared_ptr<Button> button)
{
	const std::string& buttonText = button->getText();
	if(buttonText == "Back") {
		mScreenManager->dropScreensUntil("Main Menu");
	}
	else if(buttonText == "Save") {
		save();
	}
	else if(buttonText == "Next Round") {
		mScreenManager->addScreen(boost::shared_ptr<Screen>(new LeagueScreen(mScreenManager,
						mSeason->getLeague(),
						true)));
	}
}

const std::string& SeasonScreen::getName() const
{
	return ScreenName;
}

void SeasonScreen::save()
{
	std::string filename(Menu::getSaveDir());
	filename += "/Season.sav";
	std::ofstream ofs(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
	out.push(boost::iostreams::bzip2_compressor());
	out.push(ofs);
	boost::archive::binary_oarchive oa(out);
	oa << mSeason;
	std::cout << "Saved to " << filename << "\n";
}

}

