#include <stdexcept>
#include "match/ai/PlayerAIController.h"
#include "match/Match.h"
#include "match/MatchHelpers.h"
#include "match/Player.h"
#include "match/PlayerActions.h"
#include "match/ai/AIHelpers.h"

using Common::Vector3;

PlayerAIController::PlayerAIController(Player* p)
	: PlayerController(p),
	mKickInTimer(1.0f)
{
	mPlayState = boost::shared_ptr<AIPlayController>(new AIPlayController(p));
}

/* TODO: this module uses getShirtNumber() as the player index number.
 * Rework it so that either the player index is used instead or
 * the player index is not needed. */

boost::shared_ptr<PlayerAction> PlayerAIController::act(double time)
{
	switch(mPlayer->getMatch()->getMatchHalf()) {
		case MatchHalf::NotStarted:
		case MatchHalf::HalfTimePauseEnd:
		case MatchHalf::FullTimePauseEnd:
			if(mPlayer->getShirtNumber() >= 10 && MatchHelpers::myTeamInControl(*mPlayer))
				return AIHelpers::createMoveActionTo(*mPlayer, Vector3(mPlayer->getShirtNumber() == 10 ?
							-1.0f : 2.0f, 0, 0));
			else
				return AIHelpers::createMoveActionTo(*mPlayer,
						mPlayer->getMatch()->convertRelativeToAbsoluteVector(mPlayer->getHomePosition()));

		case MatchHalf::HalfTimePauseBegin:
		case MatchHalf::FullTimePauseBegin:
		case MatchHalf::Finished:
			return AIHelpers::createMoveActionTo(*mPlayer,
					mPlayer->getMatch()->convertRelativeToAbsoluteVector(mPlayer->getTeam()->getPausePosition()));

		case MatchHalf::FirstHalf:
		case MatchHalf::SecondHalf:
		case MatchHalf::ExtraTimeFirstHalf:
		case MatchHalf::ExtraTimeSecondHalf:
		case MatchHalf::PenaltyShootout:
			switch(mPlayer->getMatch()->getPlayState()) {
				case PlayState::InPlay:
					return mPlayState->act(time);

				default:
					return actOffPlay(time);
			}
	}
	throw std::runtime_error("AI error: no state handler");
}

const std::string& PlayerAIController::getDescription() const
{
	return mPlayState->getDescription();
}

boost::shared_ptr<PlayerAction> PlayerAIController::actOffPlay(double time)
{
	if(MatchHelpers::myTeamInControl(*mPlayer)) {
		bool nearest = MatchHelpers::nearestOwnPlayerTo(*mPlayer,
				mPlayer->getMatch()->getBall()->getPosition());
		bool shouldkickball;

		if(mPlayer->getMatch()->getMatchHalf() == MatchHalf::PenaltyShootout) {
			unsigned int r = mPlayer->getMatch()->getPenaltyShootout().getRoundNumber();
			int shirtnum = 11 - (r % 10);
			shouldkickball = mPlayer->getShirtNumber() == shirtnum;
		} else {
			shouldkickball = mPlayer->getMatch()->getPlayState() == PlayState::OutKickoff ?
				mPlayer->getShirtNumber() == 10 :
				mPlayer->getMatch()->getPlayState() == PlayState::OutGoalkick ?
				mPlayer->isGoalkeeper() : nearest;
		}

		if(shouldkickball) {
			return doRestart(time);
		}
		else {
			if(mPlayer->getMatch()->getMatchHalf() == MatchHalf::PenaltyShootout) {
				return AIHelpers::createMoveActionTo(*mPlayer, Vector3(0, 0, 0));
			} else {
				if(mPlayer->getMatch()->getPlayState() == PlayState::OutKickoff) {
					if(mPlayer->getShirtNumber() >= 10)
						return AIHelpers::createMoveActionTo(*mPlayer, Vector3(mPlayer->getShirtNumber() == 10 ?
									-1.0f : 2.0f, 0, 0));
					else
						return AIHelpers::createMoveActionTo(*mPlayer,
								mPlayer->getMatch()->convertRelativeToAbsoluteVector(mPlayer->getHomePosition()));
				}
				else {
					return mPlayState->actOnRestart(time);
				}
			}
		}
	}
	else {
		if(mPlayer->getMatch()->getPlayState() == PlayState::OutKickoff) {
			return AIHelpers::createMoveActionTo(*mPlayer,
					mPlayer->getMatch()->convertRelativeToAbsoluteVector(mPlayer->getHomePosition()));
		} else if(mPlayer->getMatch()->getMatchHalf() == MatchHalf::PenaltyShootout) {
			if(mPlayer->isGoalkeeper()) {
				return mPlayState->actOnRestart(time);
			} else {
				return AIHelpers::createMoveActionTo(*mPlayer, Vector3(0, 0, 0));
			}
		} else {
			return mPlayState->actOnRestart(time);
		}
	}
}

// called when this player should restart the game
boost::shared_ptr<PlayerAction> PlayerAIController::doRestart(double time)
{
	Vector3 shoulddiff;

	// if the ball is far out, idle
	if(MatchHelpers::distanceToPitch(*mPlayer->getMatch(),
				mPlayer->getMatch()->getBall()->getPosition()) > 1.0f) {
		return boost::shared_ptr<PlayerAction>(new IdlePA());
	}

	const Vector3& ballpos = mPlayer->getMatch()->getBall()->getPosition();
	switch(mPlayer->getMatch()->getPlayState()) {
		case PlayState::OutThrowin:
			{
				if(ballpos.x < 0.0f)
					shoulddiff.x = -1.0f;
				else
					shoulddiff.x = 1.0f;

				break;
			}

		case PlayState::OutIndirectFreekick:
		case PlayState::OutDirectFreekick:
		case PlayState::OutPenaltykick:
		case PlayState::OutDroppedball:
		case PlayState::OutGoalkick:
			{
				if(MatchHelpers::attacksUp(*mPlayer))
					shoulddiff.y = -1.0f;
				else
					shoulddiff.y = 1.0f;
				break;
			}

		case PlayState::OutCornerkick:
			{
				if(ballpos.x < 0.0f)
					shoulddiff.x = -1.0f;
				else
					shoulddiff.x = 1.0f;
				if(ballpos.y < 0.0f)
					shoulddiff.y = -1.0f;
				else
					shoulddiff.y = 1.0f;

				break;
			}

		case PlayState::OutKickoff:
			shoulddiff.x = -1.0f;
			break;

		case PlayState::InPlay:
		default:
			break;
	}

	shoulddiff.normalize();
	shoulddiff *= MAX_KICK_DISTANCE * 0.8f;
	Vector3 shouldpos(mPlayer->getMatch()->getBall()->getPosition() + shoulddiff);
	return gotoKickPositionOrKick(time, shouldpos);
}

boost::shared_ptr<PlayerAction> PlayerAIController::gotoKickPositionOrKick(double time, const Vector3& pos)
{
	// called with the position where the player should restart from
	Vector3 tgt(pos - mPlayer->getPosition());
	if(tgt.length() < 0.5f) {
		// near target - see if we can kick the ball (should be true)
		// if we can, wait for a while, then restart
		// if we can't, run to the ball
		float dist = (mPlayer->getMatch()->getBall()->getPosition() -
				mPlayer->getPosition()).length();
		bool shouldkickball = MatchHelpers::playersPositionedForRestart(*mPlayer->getMatch(), *mPlayer);
		if(shouldkickball) {
			if(dist < MAX_KICK_DISTANCE * 1.0f) {
				if(mKickInTimer.checkAndRewind()) {
					return mPlayState->act(time);
				}
				else {
					mKickInTimer.doCountdown(time);
					return boost::shared_ptr<PlayerAction>(new IdlePA());
				}
			}
			else {
				// We end up here when the player is supposed to move close (but not to) the ball
				// (shouldpos), but AIHelpers::createMoveActionTo() stays still a bit further
				// away from that position - creating a deadlock if the player then idles.
				// Correct by moving to the ball.
				return AIHelpers::createMoveActionToBall(*mPlayer);
			}
		}
	}

	return AIHelpers::createMoveActionTo(*mPlayer, pos);
}

void PlayerAIController::matchHalfChanged(MatchHalf m)
{
	mPlayState->matchHalfChanged(m);
}


