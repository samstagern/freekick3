#include <string>
#include <stdexcept>

#include "soccer/DataExchange.h"
#include "soccer/Match.h"
#include "soccer/Player.h"

namespace Soccer {

static float getPlayerSkill(const TiXmlElement* skillselem, const char* skillname)
{
	const TiXmlElement* skillelem = skillselem->FirstChildElement(skillname);
	if(!skillelem)
		throw std::runtime_error("Error parsing player skills");
	const char* t = skillelem->GetText();
	if(!t)
		throw std::runtime_error("Error parsing player skills");
	return atof(t);
}

std::shared_ptr<Player> DataExchange::parsePlayer(const TiXmlElement* pelem)
{
	PlayerSkills sk;
	PlayerPosition position;
	int id;

	if(pelem->QueryIntAttribute("id", &id) != TIXML_SUCCESS)
		throw std::runtime_error("Error parsing player ID");

	const TiXmlElement* skillselem = pelem->FirstChildElement("Skills");
	const TiXmlElement* nameelem = pelem->FirstChildElement("Name");
	const TiXmlElement* poselem = pelem->FirstChildElement("Position");
	if(!skillselem || !nameelem || !poselem)
		throw std::runtime_error("Error parsing player skill/name/position");

	const char* pos = poselem->GetText();
	const char* name = nameelem->GetText();
	if(!pos || !name)
		throw std::runtime_error("Error parsing player position/name");

	if(!strcmp(pos, "goalkeeper"))
		position = PlayerPosition::Goalkeeper;
	else if(!strcmp(pos, "defender"))
		position = PlayerPosition::Defender;
	else if(!strcmp(pos, "midfielder"))
		position = PlayerPosition::Midfielder;
	else if(!strcmp(pos, "forward"))
		position = PlayerPosition::Forward;
	else
		throw std::runtime_error("Error parsing player position");

	sk.KickPower = getPlayerSkill(skillselem, "KickPower");
	sk.RunSpeed = getPlayerSkill(skillselem, "RunSpeed");
	sk.BallControl = getPlayerSkill(skillselem, "BallControl");
	return std::shared_ptr<Player>(new Player(id, name, position, sk));
}

std::shared_ptr<Team> DataExchange::parseTeam(const TiXmlElement* teamelem)
{
	int id;

	if(teamelem->QueryIntAttribute("id", &id) != TIXML_SUCCESS)
		throw std::runtime_error("Error parsing team");

	const TiXmlElement* nameelem = teamelem->FirstChildElement("Name");
	const TiXmlElement* playerselem = teamelem->FirstChildElement("Players");
	if(!playerselem || !nameelem)
		throw std::runtime_error("Error parsing team");

	const char* name = nameelem->GetText();
	if(!name)
		throw std::runtime_error("Error parsing team");

	std::vector<int> playerids;
	std::vector<std::shared_ptr<Player>> players;
	for(const TiXmlElement* pelem = playerselem->FirstChildElement(); pelem; pelem = pelem->NextSiblingElement()) {
		int playerid;
		if(pelem->QueryIntAttribute("id", &playerid) != TIXML_SUCCESS)
			throw std::runtime_error("Error parsing player in team");
		const TiXmlElement* nameelem = pelem->FirstChildElement("Name");
		if(nameelem) {
			players.push_back(parsePlayer(pelem));
		}
		else {
			playerids.push_back(playerid);
		}
	}

	std::shared_ptr<Team> team;
	if(playerids.empty()) {
		team.reset(new Team(id, name, players));
	}
	else {
		team.reset(new Team(id, name, playerids));
		for(auto p : players)
			team->addPlayer(p);
	}
	return team;
}

std::shared_ptr<Match> DataExchange::parseMatchDataFile(const char* fn)
{
	TiXmlDocument doc(fn);
	std::stringstream ss;
	ss << "Error parsing match file " << fn;

	if(!doc.LoadFile())
		throw std::runtime_error(ss.str());

	TiXmlHandle handle(&doc);

	TiXmlElement* teamelem = handle.FirstChild("Match").FirstChild("Teams").FirstChild("Team").ToElement();
	if(!teamelem)
		throw std::runtime_error(ss.str());

	std::vector<std::shared_ptr<Team>> teams;

	for(; teamelem; teamelem = teamelem->NextSiblingElement()) {
		if(teams.size() > 2) {
			throw std::runtime_error(ss.str());
		}
		teams.push_back(parseTeam(teamelem));
	}
	if(teams.size() != 2) {
		throw std::runtime_error(ss.str());
	}

	const TiXmlElement* matchreselem = handle.FirstChild("Match").FirstChild("MatchResult").ToElement();
	if(!matchreselem)
		throw std::runtime_error(ss.str());

	MatchResult mres;
	int played;
	if(matchreselem->QueryIntAttribute("played", &played) != TIXML_SUCCESS)
		throw std::runtime_error(ss.str());

	mres.Played = played;
	if(played) {
		const TiXmlElement* homeelem = matchreselem->FirstChildElement("Home");
		const TiXmlElement* awayelem = matchreselem->FirstChildElement("Away");
		if(!homeelem || !awayelem)
			throw std::runtime_error(ss.str());
		mres.HomeGoals = atoi(homeelem->GetText());
		mres.AwayGoals = atoi(awayelem->GetText());
	}

	std::shared_ptr<Match> m(new Match(teams[0], teams[1], TeamTactics(), TeamTactics()));
	m->setResult(mres);
	return m;
}

TiXmlElement* DataExchange::createTeamElement(const Team& t, bool reference_players)
{
	TiXmlElement* teamelem = new TiXmlElement("Team");

	teamelem->SetAttribute("id", t.getId());

	{
		TiXmlElement* nameelem = new TiXmlElement("Name");
		nameelem->LinkEndChild(new TiXmlText(t.getName()));
		teamelem->LinkEndChild(nameelem);
	}

	TiXmlElement* playerselem = new TiXmlElement("Players");

	int j = 0;
	while(1) {
		std::shared_ptr<Player> p = t.getPlayer(j);
		if(!p)
			break;
		j++;

		{
			TiXmlElement* playerelem;
			if(reference_players) {
				playerelem = new TiXmlElement("Player");
				playerelem->SetAttribute("id", p->getId());
			}
			else {
				playerelem = createPlayerElement(*p);
			}
			playerselem->LinkEndChild(playerelem);
		}
	}
	teamelem->LinkEndChild(playerselem);

	return teamelem;
}

TiXmlElement* DataExchange::createPlayerElement(const Player& p)
{
	TiXmlElement* playerelem = new TiXmlElement("Player");

	playerelem->SetAttribute("id", p.getId());

	TiXmlElement* nameelem = new TiXmlElement("Name");
	nameelem->LinkEndChild(new TiXmlText(p.getName()));
	playerelem->LinkEndChild(nameelem);

	TiXmlElement* poselem = new TiXmlElement("Position");
	switch(p.getPlayerPosition()) {
		case PlayerPosition::Goalkeeper:
			poselem->LinkEndChild(new TiXmlText("goalkeeper"));
			break;

		case PlayerPosition::Defender:
			poselem->LinkEndChild(new TiXmlText("defender"));
			break;

		case PlayerPosition::Midfielder:
			poselem->LinkEndChild(new TiXmlText("midfielder"));
			break;

		case PlayerPosition::Forward:
			poselem->LinkEndChild(new TiXmlText("forward"));
			break;
	}
	playerelem->LinkEndChild(poselem);

	TiXmlElement* skillselem = new TiXmlElement("Skills");
	TiXmlElement* skill1elem = new TiXmlElement("KickPower");
	TiXmlElement* skill2elem = new TiXmlElement("RunSpeed");
	TiXmlElement* skill3elem = new TiXmlElement("BallControl");
	skill1elem->LinkEndChild(new TiXmlText(std::to_string(p.getSkills().KickPower)));
	skill2elem->LinkEndChild(new TiXmlText(std::to_string(p.getSkills().RunSpeed)));
	skill3elem->LinkEndChild(new TiXmlText(std::to_string(p.getSkills().BallControl)));
	skillselem->LinkEndChild(skill1elem);
	skillselem->LinkEndChild(skill2elem);
	skillselem->LinkEndChild(skill3elem);
	playerelem->LinkEndChild(skillselem);

	return playerelem;
}

void DataExchange::createMatchDataFile(const Match& m, const char* fn)
{
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
	TiXmlElement* matchelem = new TiXmlElement("Match");
	TiXmlElement* teamselem = new TiXmlElement("Teams");
	for(int i = 0; i < 2; i++) {
		std::shared_ptr<Team> t = m.getTeam(i);
		TiXmlElement* teamelem = createTeamElement(*t, false);
		teamselem->LinkEndChild(teamelem);
	}
	matchelem->LinkEndChild(teamselem);

	TiXmlElement* matchresultelem = new TiXmlElement("MatchResult");
	matchresultelem->SetAttribute("played", m.getResult().Played ? "1" : "0");
	TiXmlElement* homereselem = new TiXmlElement("Home");
	TiXmlElement* awayreselem = new TiXmlElement("Away");
	homereselem->LinkEndChild(new TiXmlText(std::to_string(m.getResult().HomeGoals)));
	awayreselem->LinkEndChild(new TiXmlText(std::to_string(m.getResult().AwayGoals)));
	matchresultelem->LinkEndChild(homereselem);
	matchresultelem->LinkEndChild(awayreselem);
	matchelem->LinkEndChild(matchresultelem);

	/* TODO: add team tactics */

	doc.LinkEndChild(decl);
	doc.LinkEndChild(matchelem);
	if(!doc.SaveFile(fn)) {
		throw std::runtime_error(std::string("Unable to save XML file ") + fn);
	}
}


void DataExchange::updateTeamDatabase(const char* fn, TeamDatabase& db)
{
	TiXmlDocument doc(fn);
	std::stringstream ss;
	ss << "Error parsing team database file " << fn;

	if(!doc.LoadFile())
		throw std::runtime_error(ss.str());

	TiXmlHandle handle(&doc);

	TiXmlElement* teamelem = handle.FirstChild("Teams").FirstChild("Team").ToElement();
	if(!teamelem)
		throw std::runtime_error(ss.str());

	for(; teamelem; teamelem = teamelem->NextSiblingElement()) {
		std::shared_ptr<Team> t = parseTeam(teamelem);
		db.insert(std::make_pair(t->getId(), t));
	}
}

void DataExchange::updatePlayerDatabase(const char* fn, PlayerDatabase& db)
{
	TiXmlDocument doc(fn);
	std::stringstream ss;
	ss << "Error parsing team database file " << fn;

	if(!doc.LoadFile())
		throw std::runtime_error(ss.str());

	TiXmlHandle handle(&doc);

	TiXmlElement* playerselem = handle.FirstChild("Players").ToElement();
	if(!playerselem)
		throw std::runtime_error(ss.str());

	for(TiXmlElement* pelem = playerselem->FirstChildElement(); pelem; pelem = pelem->NextSiblingElement()) {
		std::shared_ptr<Player> p = parsePlayer(pelem);
		db.insert(make_pair(p->getId(), p));
	}
}

void DataExchange::createTeamDatabase(const char* fn, const TeamDatabase& db)
{
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
	TiXmlElement* teamselem = new TiXmlElement("Teams");
	for(auto& tm : db) {
		TiXmlElement* teamelem = createTeamElement(*tm.second, true);
		teamselem->LinkEndChild(teamelem);
	}
	doc.LinkEndChild(decl);
	doc.LinkEndChild(teamselem);
	if(!doc.SaveFile(fn)) {
		throw std::runtime_error(std::string("Unable to save XML file ") + fn);
	}
}

void DataExchange::createPlayerDatabase(const char* fn, const PlayerDatabase& db)
{
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
	TiXmlElement* players = new TiXmlElement("Players");
	for(auto& pl : db) {
		TiXmlElement* playerelem = createPlayerElement(*pl.second);
		players->LinkEndChild(playerelem);
	}
	doc.LinkEndChild(decl);
	doc.LinkEndChild(players);
	if(!doc.SaveFile(fn)) {
		throw std::runtime_error(std::string("Unable to save XML file ") + fn);
	}
}


}
