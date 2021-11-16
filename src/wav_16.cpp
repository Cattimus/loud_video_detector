#include "wav_16.h"

//read from memory as if you'd read from a file
static void mem_read(void* dest, void* src, uint32_t len, uint32_t* pos)
{
	memcpy(dest, src, len);
	*pos += len;
}

//fetch a specific number of bytes from an index
static string fetch_num(void* loc, int num_bytes)
{
	string to_return;
	
	for(int i = 0; i < num_bytes; i++)
	{
		char* temp = (char*)loc + i;
		to_return += *temp;
	}
	
	return to_return;
}

void wav_16::clear_data()
{
	//clear wav header from data
	memset(&wav_data, 0, 44);
	
	//free wav data
	free(wav_data.data);
	wav_data.data = NULL;
	
	is_empty = true;
}

//print wav file info
void wav_16::print_info()
{
	printf("Master id: %s\n", fetch_num(&wav_data.master_id, 4).c_str());
	printf("Master size: %d\n\n", wav_data.master_size);
	
	printf("Format id: %s\n", fetch_num(&wav_data.fmt_id, 4).c_str());
	printf("Format size: %d\n", wav_data.fmt_size);
	printf("Format code: %d\n", wav_data.fmt_tag);
	printf("Number of channels: %d\n", wav_data.channels);
	printf("Samples per second: %d\n", wav_data.samples_per_sec);
	printf("Bytes per second: %d\n", wav_data.bytes_per_sec);
	printf("Block allign: %d\n", wav_data.block_align);
	printf("Bits per sample: %d\n\n", wav_data.bits_per_sample);
	
	printf("Data id: %s\n", fetch_num(&wav_data.data_id, 4).c_str());
	printf("Data size: %d\n", wav_data.data_size);
}

//load wav file from memory
void wav_16::load_file(vector<uint8_t> memory_file)
{
	if(!is_empty)
	{
		clear_data();
	}
	
	uint32_t current_index = 0;
	
	//read master header
	mem_read(&(wav_data.master_id), memory_file.data() + current_index, 4, &current_index);
	mem_read(&(wav_data.master_size), memory_file.data() + current_index, 4, &current_index);
	mem_read(&(wav_data.wave_identifier), memory_file.data() + current_index, 4, &current_index);
	
	if(strncmp((const char*)&(wav_data.wave_identifier), "WAVE", 4) != 0)
	{
		fprintf(stderr, "loud_checker: input file is not a wav.\n");
		exit(1);
	}
	
	if(strncmp((const char*)&(wav_data.master_id), "RIFF", 4) != 0)
	{
		fprintf(stderr, "loud_checker: RIFX file format is not supported.\n");
		exit(1);
	}
	
	//read format chunk
	mem_read(&(wav_data.fmt_id), memory_file.data() + current_index, 4, &current_index);
	mem_read(&(wav_data.fmt_size), memory_file.data() + current_index, 4, &current_index);
	mem_read(&(wav_data.fmt_tag), memory_file.data() + current_index, 2, &current_index);
	mem_read(&(wav_data.channels), memory_file.data() + current_index, 2, &current_index);
	mem_read(&(wav_data.samples_per_sec), memory_file.data() + current_index, 4, &current_index);
	mem_read(&(wav_data.bytes_per_sec), memory_file.data() + current_index, 4, &current_index);
	mem_read(&(wav_data.block_align), memory_file.data() + current_index, 2, &current_index);
	mem_read(&(wav_data.bits_per_sample), memory_file.data() + current_index, 2, &current_index);
	
	//error out if not PCM wav
	if(wav_data.fmt_tag != 0x001)
	{
		string wav_type = "Decoding error detected";
		
		switch(wav_data.fmt_tag)
		{
			case 0x0003:
			wav_type = "IEEE float";
			break;
			
			case 0x0006:
			wav_type = "8-bit A-law";
			break;
			
			case 0x0007:
			wav_type = "8-bit MU-law";
			break;
			
			case 0xFFFE:
			wav_type = "Extended wav";
			break;
			
		}
		
		fprintf(stderr, "loud_checker: only wav PCM format is supported. Input is: %s\n", wav_type.c_str());
		exit(1);
	}
	
	//skip all non-data chunks
	uint32_t id;
	uint32_t size;
	mem_read(&id, memory_file.data() + current_index, 4, &current_index);
	mem_read(&size, memory_file.data() + current_index, 4, &current_index);
	
	while(strncmp((const char*)&id, "data", 4) != 0)
	{
		current_index += size;
		
		mem_read(&id, memory_file.data() + current_index, 4, &current_index);
		mem_read(&size, memory_file.data() + current_index, 4, &current_index);
	}
	
	//read data chunk
	wav_data.data_id = id;
	wav_data.data_size = size;
	wav_data.data = (uint8_t*)calloc(wav_data.data_size, 1);
	mem_read(wav_data.data, memory_file.data() + current_index, wav_data.data_size, &current_index);
}

wav_16::wav_16(vector<uint8_t> memory_file)
{
	load_file(memory_file);
}

wav_16::wav_16()
{
	wav_data.data = NULL;
}

wav_16::~wav_16()
{
	clear_data();
}

void wav_16::write_file(string path)
{
	FILE* output = fopen(path.c_str(), "wb");
	fwrite(&wav_data, 1, 44, output);
	fwrite(wav_data.data, 1, wav_data.data_size, output);
	fclose(output);
}

uint8_t* wav_16::pcm_data()
{
	return wav_data.data;
}

uint8_t* wav_16::get_sample(int offset)
{
	return (wav_data.data + (offset * (wav_data.bits_per_sample * wav_data.channels / 8)));
}

int wav_16::total_samples()
{
	return (wav_data.data_size / (wav_data.bits_per_sample * wav_data.channels / 8));
}

int wav_16::get_window(int ms)
{
	return (wav_data.samples_per_sec / (1000 / ms));
}
