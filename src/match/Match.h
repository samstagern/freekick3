#ifndef MATCH_H
#define MATCH_H

#include <iostream>
#include <vector>
#include <map>
#include <array>

#include "soccer/Match.h"

#include "match/Clock.h"
#include "match/Pitch.h"
#include "match/Team.h"
#include "match/Player.h"
#include "match/Ball.h"
#include "match/Referee.h"

enum class MatchHalf {
	NotStarted,
	FirstHalf,
	HalfTimePauseBegin,
	HalfTimePauseEnd,
	SecondHalf,
	FullTimePauseBegin,
	FullTimePauseEnd,
	ExtraTimeFirstHalf,
	ExtraTimeSecondHalf,
	PenaltyShootout,
	Finished
};

std::ostream& operator<<(std::ostream& out, const MatchHalf& m);
bool playing(MatchHalf h);

enum class PlayState {
	InPlay,
	OutKickoff,
	OutThrowin,
	OutGoalkick,
	OutCornerkick,
	OutIndirectFreekick,
	OutDirectFreekick,
	OutPenaltykick,
	OutDroppedball
};

std::ostream& operator<<(std::ostream& out, const PlayState& m);
bool playing(PlayState h);

class GoalInfo {
	public:
		GoalInfo(const Match& m, bool pen, bool own);
		const std::string& getScorerName() const;
		const std::string& getShortScorerName() const;
		const std::string& getScoreTime() const;

	private:
		std::string mScorerName;
		std::string mShortScorerName;
		std::string mScoreTime;
};

class PenaltyShootout {
	public:
		PenaltyShootout();
		void addShot(bool goal);
		bool firstTeamKicksNext() const;
		unsigned int getRoundNumber() const; // starts at 0
		int getScore(bool first) const;
		bool isFinished() const;

	private:
		bool mFirstNext;
		unsigned int mGoals[2];
		bool mFinished;
		unsigned int mRoundNumber;
};

class Match : public Soccer::Match {
	public:
		Match(const Soccer::Match& m, double matchtime, bool extratime, bool penalties,
				bool awaygoals, int homeagg, int awayagg);
		Team* getTeam(unsigned int team);
		const Team* getTeam(unsigned int team) const;
		const Player* getPlayer(unsigned int team, unsigned int idx) const;
		Player* getPlayer(unsigned int team, unsigned int idx);
		const Ball* getBall() const;
		Ball* getBall();
		const Referee* getReferee() const;
		void update(double time);
		bool matchOver() const;
		MatchHalf getMatchHalf() const;
		void setMatchHalf(MatchHalf h);
		void setPlayState(PlayState h);
		PlayState getPlayState() const;
		Common::Vector3 convertRelativeToAbsoluteVector(const RelVector3& v) const;
		RelVector3 convertAbsoluteToRelativeVector(const Common::Vector3& v) const;
		float getPitchWidth() const;
		float getPitchHeight() const;
		int kickBall(Player* p, const Common::Vector3& v);
		double getRollInertiaFactor() const;
		double getAirViscosityFactor() const;
		void addGoal(bool forFirst);
		int getScore(bool first) const;
		bool grabBall(Player* p);
		double getTime() const;
		void setGoalScorer(const Player* p);
		const Player* getGoalScorer() const;
		const std::array<std::vector<GoalInfo>, 2>& getGoalInfos() const;
		const PenaltyShootout& getPenaltyShootout() const;
		void addPenaltyShootoutShot(bool goal);
		bool getAwayGoals() const;
		int getAggregateScore(bool first) const;

	private:
		void applyPlayerAction(PlayerAction* a,
				const boost::shared_ptr<Player> p, double time);
		void updateReferee(double time);
		void updateTime(double time);
		void checkPlayerPlayerCollision(boost::shared_ptr<Player> p, boost::shared_ptr<Player> p2);

		boost::shared_ptr<Team> mTeams[2];
		boost::shared_ptr<Ball> mBall;
		std::map<boost::shared_ptr<Player>, boost::shared_ptr<PlayerAction>> mCachedActions;
		Referee mReferee;
		double mTime;
		double mTimeAccelerationConstant;
		MatchHalf mMatchHalf;
		PlayState mPlayState;
		Pitch mPitch;
		int mScore[2];
		std::array<std::vector<GoalInfo>, 2> mGoalInfos;
		const Player* mGoalScorer;
		bool mExtraTime;
		bool mPenalties;
		PenaltyShootout mPenaltyShootout;
		bool mAwayGoals;
		int mHomeAgg;
		int mAwayAgg;
};

#endif

