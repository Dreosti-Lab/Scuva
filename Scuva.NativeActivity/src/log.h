// Scuva Log Class Header
#pragma once

// Include config defintions header
#include "config.h"

// Include standard libraries
#include <string>

// Scuva Logging Class Declarations
// --------------------------------
// Cross-platform utilities for logging, error reporting, and debugging

class Log
{
public:
																
	// Static public methods
	static void			Initialize(std::string path);		// Initialize logger
	static void			Write(std::string msg);				// Update log entry
	static std::string	Path();								// Get log path

private:
	
	// Static private member
	static std::string	path_;
	static std::string	log_path_;
};

// FIN