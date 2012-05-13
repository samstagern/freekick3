#include <iostream>
#include <stdexcept>

#include <SDL_image.h>
#include <SDL_ttf.h>
#include <GL/gl.h>

#include "Menu.h"
#include "common/SDL_utils.h"

#include "soccer/DataExchange.h"
#include "soccer/Player.h"
#include "soccer/Match.h"

namespace Soccer {

using namespace Common;

static const int screenWidth = 800;
static const int screenHeight = 600;

Menu::Menu()
	: mRunning(true),
	mPressedButton(std::string(""))
{
	mScreen = SDL_utils::initSDL(screenWidth, screenHeight);
	SDL_utils::setupOrthoScreen(screenWidth, screenHeight);

	mFont = TTF_OpenFont("share/DejaVuSans.ttf", 12);
	if(!mFont) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
		throw std::runtime_error("Loading font");
	}
	mBackground = std::shared_ptr<Texture>(new Texture("share/bg.png", 0, 0));
	mButtons.push_back(std::shared_ptr<Button>(new Button("Friendly", mFont,
					Rectangle(0.35f * screenWidth, 0.35f * screenHeight,
						0.30f * screenWidth, 0.15f * screenHeight))));
	mButtons.push_back(std::shared_ptr<Button>(new Button("Quit", mFont,
					Rectangle(0.35f * screenWidth, 0.65f * screenHeight,
						0.30f * screenWidth, 0.15f * screenHeight))));

	DataExchange::updatePlayerDatabase("share/teams/Players_1000.xml", mPlayers);
	DataExchange::updateTeamDatabase("share/teams/Teams.xml", mTeams);
	for(auto t : mTeams) {
		t.second->fetchPlayersFromDB(mPlayers);
	}
}

Menu::~Menu()
{
	if(mFont)
		TTF_CloseFont(mFont);
	TTF_Quit();
	SDL_Quit();
}

void Menu::run()
{
	drawScreen();
	while(mRunning) {
		if(handleEvents())
			drawScreen();
	}
}

void Menu::drawScreen()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// draw background
	glColor3f(0.5f, 0.5f, 0.5f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mBackground->getTexture());
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, screenHeight, 0.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(screenWidth, screenHeight, 0.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(screenWidth, 0.0f, 0.0f);
	glEnd();

	// draw buttons
	for(auto b : mButtons) {
		const Rectangle& r = b->getRectangle();
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor3f(0.85f, 0.75f, 0.50f);
		glVertex3f(r.x, screenHeight - r.y, 1.0f);
		glVertex3f(r.x + r.w, screenHeight - r.y, 0.0f);
		glColor3f(0.75, 0.50f, 0.45f);
		glVertex3f(r.x + r.w, screenHeight - r.y - r.h, 0.0f);
		glVertex3f(r.x, screenHeight - r.y - r.h, 0.0f);
		glEnd();

		float tw2 = b->getTexture()->getWidth() * 0.8f;
		float th2 = b->getTexture()->getHeight() * 0.8f;

		glColor3f(1.0f, 1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, b->getTexture()->getTexture());
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(r.x + r.w * 0.5f - tw2, screenHeight - r.y - r.h * 0.5f + th2, 1.1f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(r.x + r.w * 0.5f + tw2, screenHeight - r.y - r.h * 0.5f + th2, 1.1f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(r.x + r.w * 0.5f + tw2, screenHeight - r.y - r.h * 0.5f - th2, 1.1f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(r.x + r.w * 0.5f - tw2, screenHeight - r.y - r.h * 0.5f - th2, 1.1f);
		glEnd();

	}

	SDL_GL_SwapBuffers();
}

bool Menu::handleEvents()
{
	SDL_Event ev;
	int ret = SDL_WaitEvent(&ev);
	if(!ret) {
		fprintf(stderr, "Error in SDL_WaitEvent: %s\n", SDL_GetError());
		mRunning = false;
		return true;
	}
	switch(ev.type) {
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if(ev.button.button == SDL_BUTTON_LEFT)
				return recordMouseButton(ev.type == SDL_MOUSEBUTTONUP, ev.button.x, ev.button.y);

		case SDL_KEYDOWN:
			if(ev.key.keysym.sym == SDLK_ESCAPE)
				mRunning = false;
			return true;

		case SDL_QUIT:
			mRunning = false;
			return true;

		case SDL_VIDEORESIZE:
		case SDL_VIDEOEXPOSE:
			return true;

		default:
			return false;
	}

	return false;
}

bool Menu::recordMouseButton(bool up, int x, int y)
{
	for(auto b : mButtons) {
		if(b->clicked(x, y)) {
			if(!up) {
				mPressedButton = std::string(b->getText());
			} else {
				if(mPressedButton == b->getText()) {
					if(mPressedButton == "Quit") {
						mRunning = false;
						mPressedButton = std::string("");
						return true;
					}
					else if(mPressedButton == "Friendly") {
						mPressedButton = std::string("");
						auto it = mTeams.find(1);
						if(it == mTeams.end()) {
							std::cerr << "Could not find team with ID 1\n";
							return true;
						}
						std::shared_ptr<Team> t1 = it->second;
						it = mTeams.find(2);
						if(it == mTeams.end()) {
							std::cerr << "Could not find team with ID 2\n";
							return true;
						}
						std::shared_ptr<Team> t2 = it->second;
						Match m(t1, t2, TeamTactics(), TeamTactics());
						DataExchange::createMatchDataFile(m, "tmp/match.xml");
						return true;
					}
				}
				mPressedButton = std::string("");
			}
			break;
		}
	}
	return false;
}

Button::Button(const char* text, TTF_Font* font, const Rectangle& dim)
	: mText(std::string(text)),
	mRectangle(dim)
{
	SDL_Surface* textsurface;
	SDL_Color color = {255, 255, 255};
	textsurface = TTF_RenderUTF8_Blended(font, text, color);

	if(!textsurface) {
		fprintf(stderr, "Could not render text: %s\n",
				TTF_GetError());
		throw std::runtime_error("Rendering text");
	}
	else {
		mTextTexture = std::shared_ptr<Texture>(new Texture(textsurface));
		SDL_FreeSurface(textsurface);
	}
}

bool Button::clicked(int x, int y) const
{
	return mRectangle.pointWithin(x, y);
}

const std::string& Button::getText() const
{
	return mText;
}

const Rectangle& Button::getRectangle() const
{
	return mRectangle;
}

const Common::Texture* Button::getTexture() const
{
	return mTextTexture.get();
}

}

