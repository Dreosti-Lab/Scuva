// vkLog Class Source
#pragma once

// Include config defintions header
#include "config.h"

// Include standard libraries
#include <string>
#include <chrono>
#include <ctime>

// Include class library
#include "log.h"

// VK Logging Class Definitions
// ------------------------------

// Static private member
std::string Log::path_;
std::string Log::log_path_;

// Initialize log
void Log::Initialize(std::string path)
{
	// Set static path member
	path_ = path;
	log_path_ = path + std::string(LOG_PATH);

	// Open log file
	FILE* log_file = fopen(log_path_.c_str(), "w");
	if (log_file == NULL)
	{

		LOGD("Scuva Log failed to open: %s", log_path_.c_str());
	}

	// Add log header and date/time
	std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(tp);
	fprintf(log_file, "Scuva Log: %s\n", std::ctime(&t));

	// Close log file
	fclose(log_file);
}

// Add entry to log
void Log::Write(std::string msg)
{
	// Open log file
	FILE* log_file = fopen(log_path_.c_str(), "a");

	// Write message to log file
	fprintf(log_file, "%s\n", msg.c_str());

	// Close log file
	fclose(log_file);
}

// Initialize log
std::string Log::Path()
{
	return path_;
}

// FIN