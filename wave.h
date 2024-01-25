#pragma once

#include <iostream>
#include <vector>
#include <string>

//Source for this information from http://soundfile.sapp.org/doc/WaveFormat/

struct Wave {

	Wave() {
		//Generic Default Values

		//Riff Chunk
		chunk_id = "";
		chunk_size = "";
		format = "";

		//FMT sub-chunk
		subchunk1_id = "";
		subchunk1_size = 0;
		audio_format = 0;
		num_channels = 0;
		sample_rate = 0;
		byte_rate = 0;
		block_align = 0;
		bits_per_sample = 0;

		//Data sub-chunk
		subchunk2_id = "";
		subchunk2_size = 0;
		duration = 0;
		max_amplitude = 0;
		frequency = 0;

		//Extra
		number_of_samples = 0;
		sample_size = 0;
	}

	//Riff Chunk
	std::string chunk_id;
	std::string chunk_size;
	std::string format;

	//FMT sub-chunk
	std::string subchunk1_id;
	int subchunk1_size;
	short audio_format;
	short num_channels;
	int sample_rate;
	int byte_rate; 
	short block_align;
	short bits_per_sample;

	//Data sub-chunk
	std::string subchunk2_id; 
	int subchunk2_size;
	float duration; 
	int max_amplitude;
	double frequency;
	int number_of_samples;
	int sample_size;
};

