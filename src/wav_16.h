#pragma once
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
using namespace std;

typedef struct Wav_16_data
{
	//wav header chunk
	uint32_t master_id;
	uint32_t master_size;
	uint32_t wave_identifier;
	
	//wav data
	uint32_t fmt_id;
	uint32_t fmt_size;
	uint16_t fmt_tag;
	uint16_t channels;
	uint32_t samples_per_sec;
	uint32_t bytes_per_sec;
	uint16_t block_align;
	uint16_t bits_per_sample;
	
	uint32_t data_id;
	uint32_t data_size;
	uint8_t* data;
}wav_16_data;

class wav_16
{
	private:
	bool is_empty = true;
	
	void clear_data();
	
	public:
	wav_16_data wav_data;
	wav_16(vector<uint8_t> memory_file);
	wav_16();
	~wav_16();
	
	uint8_t* pcm_data();
	uint8_t* get_sample(int offset);
	void load_file(vector<uint8_t> memory_file);
	void write_file(string path);
	void print_info();
	int total_samples();
	int get_window(int window_duration_ms);
};