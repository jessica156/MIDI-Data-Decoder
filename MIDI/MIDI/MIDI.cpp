// MIDI.cpp : This program will read in MIDI data, convert the MIDI information, and output the converted information into a text file.
// Authors: Jessica Nguyen and David Sherrier

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <chrono>
#include <windows.h>
#include <vector>
#include <numeric>
#include <iterator>
#include <utility>
#include <array>
#include <thread>
#include <future>
#include "RtMidi.h"

struct keyInfo {
	std::string note{};
	int octave{}, velocity{};
	long double duration{};
	friend std::ostream& operator<<(std::ostream& os, keyInfo& k) {
			os << std::left << std::setw(26) << std::setfill(' ') << k.note 
			<< std::left << std::setw(26) << std::setfill(' ') << k.octave 
			<< std::left << std::setw(26) << std::setfill(' ') << k.velocity 
			<< std::left << std::setw(26) << std::setfill(' ') << k.duration
			<< std::endl;
			return os;
	}
};

int main()
{
	RtMidiIn  *midiin = nullptr;
	std::vector<unsigned char> message;
	std::vector<keyInfo> key_press_map{};
	int nBytes, i;
	unsigned int inputPort;
	long double totalTime;
	long double timeDifference = 0;
	long double stamp;
	std::string answer;
	std::string noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	int unique_id{};
	std::array<int, 3> midiData{};
	std::array<int, 3> midiDataPrev{};
	//If -1 then that key has not been initially pressed else hold ID of the note for when it was pressed
	int id_of_last_on[11][12]{ { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
							  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } };

	//Creating an output text file to output the converted MIDI data 
	std::ofstream outfile;
	outfile.open("midiData.txt");
	outfile << std::left << std::setw(26) << std::setfill(' ') << "Note Name"
		<< std::left << std::setw(26) << std::setfill(' ') << "Note Octave"
		<< std::left << std::setw(26) << std::setfill(' ') << "Velocity"
		<< std::left << std::setw(26) << std::setfill(' ') << "Duration (ms)"
		<< std::endl;
	
	//RtMidiIn constructor
	try 
	{
		midiin = new RtMidiIn();
	}
	catch (RtMidiError &error) 
	{
		error.printMessage();
		exit(EXIT_FAILURE);
	}
	
	//Check inputs
	input:
		unsigned int nPorts = midiin->getPortCount();

	if(nPorts == 0)
	{
		std::cout << "No MIDI input source connected. Please input a MIDI source and enter 'ok'.\n";
		std::cin >> answer;
		if (answer == "ok")
			goto input;
		else
			exit(EXIT_FAILURE);
	}
	else
	{
		if (nPorts == 1)
			std::cout << "There is " << nPorts << " MIDI input source available.\n";
		else
			std::cout << "There are " << nPorts << " MIDI input sources available.\n";
	}

	std::string portName;

	for (unsigned int i = 0; i < nPorts; i++) 
	{
		try 
		{
			portName = midiin->getPortName(i);
		}
		catch (RtMidiError &error) 
		{
			error.printMessage();
			delete midiin;
			exit(EXIT_FAILURE);
		}

		std::cout << "Input Port #" << i << ": " << portName << '\n';
	}
	
	//Prompt the user to input the port number they want to use
	std::cout << "Enter the port number to use: ";
	std::cin >> inputPort;
	midiin->openPort(inputPort);

	//Don't ignore sysex, timing, or active sensing messages
	midiin->ignoreTypes(false, false, false);
	
	auto response = std::async(std::launch::async, []() {
		std::string c{};
		while (c != "exit") {
			std::cin >> c;
		}
		return c;
	});

	auto span = std::chrono::milliseconds(5);
	std::cout << "Reading MIDI from port ... quit with 'exit'\n";
	
	//Periodically check input queue
	while (response.wait_for(span) == std::future_status::timeout)
	{
		stamp = midiin->getMessage(&message);
		nBytes = message.size();
		int indexCount = 0;

		for (i = 0; i < nBytes; ++i)
		{
			int value = static_cast<int>(message[i]);
			std::cout << "Byte " << i << " = " << value << ", ";
			midiData[i] = value;	// Adds message data to vector
		}

		if (midiData == midiDataPrev)
			continue;
			
		if (midiData[0] == 144) // New key is pressed
			{
				keyInfo k{};
				k.velocity = midiData[2];
				
				// Note name = noteName[value % 12], Note octave = (value / 12)
				k.note = noteNames[midiData[1] % 12];
				k.octave = (midiData[1] / 12);
				id_of_last_on[k.octave][midiData[1] % 12] = unique_id;

				key_press_map.push_back(k);
				++unique_id;

				// Add the time difference between the notes
				timeDifference = timeDifference + (stamp * 1000.0);
			}
		else	// A key is released
			{
				// Note name = noteName[value % 12], Note octave = (value / 12)
				key_press_map[id_of_last_on[midiData[1] / 12][midiData[1] % 12]].duration = (stamp * 1000.0);

				// Print out the info to the text file
				outfile << key_press_map[id_of_last_on[midiData[1] / 12][midiData[1] % 12]];
			}

		if (nBytes > 0)
			std::cout << "Stamp = " << stamp << std::endl;
		
		midiDataPrev = midiData;

		//Sleep for 10 milliseconds ... platform-dependent
		Sleep(10);
	} 
	
	totalTime = std::accumulate(key_press_map.begin(), key_press_map.end(), 0.0, [](long double sum, keyInfo& k) {return sum + k.duration;});
	totalTime += timeDifference;

	outfile << "\nTotal time: " << totalTime << " ms";

	//Clean up
	delete midiin; 

	//Close outfile
	outfile.close();

	return 0;
}