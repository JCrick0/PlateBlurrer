// PlateBlurrer.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <memory>
#include <vector>
#include <filesystem>
#include <opencv2/core/utils/logger.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

//Colors for console output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_UNDERLINE "\033[4m"

//Center console output for better readability - also handles dynamic console widths for linux and windows (with a fallback to 80 columns if the width cannot be determined)
void printCentered(const std::string& text) {
	//Get console width (default to 80 if it cannot be determined)
	int consoleWidth;
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	} else {
		consoleWidth = 80;
	}
#else
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
		consoleWidth = w.ws_col;
	} else {
		consoleWidth = 80;
	}
#endif
	int padding = (consoleWidth - static_cast<int>(text.length())) / 2;
	if (padding > 0) {
		std::cout << std::string(padding, ' ');
	}
	std::cout << text << std::endl;
}

void blurLicensePlate(cv::Mat& image, const cv::Rect& plate);
