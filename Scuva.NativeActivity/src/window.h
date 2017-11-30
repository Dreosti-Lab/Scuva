// Scuva Window Class Header
#pragma once

// Include config defintions header
#include "config.h"

// Include required type definitinons
#include <vector>

// Suva Window Data Structures
struct UI
{
	bool	touch = false;
	float	x_pos = 0.f;
	float	y_pos = 0.f;
	float	x_vel = 0.f;
	float	y_vel = 0.f;
	bool	up_key = false;
	bool	down_key = false;
	bool	left_key = false;
	bool	right_key = false;
};

// Platform agnostic window and user input class
// ---------------------------------------------
class Window
{

public:

	// Constructors
	Window();

	// Destructor
	~Window();

	// Public Members
	void*						winptr_;
	UI							ui_;
	bool						fullscreen_;
	int							x_;
	int							y_;
	int							width_;
	int							height_;
	bool						close_;
};


// FIN